#include "FirewallService.h"

#include <cstdio>
#include <cwchar>
#include <functional>
#include <set>
#include <vector>

#include <netfw.h>
#include <windows.h>

#include "HttpUtil.h"
#include "Json.h"
#include "Util.h"

namespace FirewallService {

static const char* kIpRangesUrl = "https://ip-ranges.amazonaws.com/ip-ranges.json";
static const char* kRulePrefix  = "DbdBlockRule";

std::string RulePrefixFor(const std::string& platform)
{
    return std::string(kRulePrefix) + platform + "_";
}

// Minimal RAII wrapper around a COM interface pointer.
template <typename T>
class ComPtr
{
public:
    ComPtr() = default;
    ~ComPtr() { if (p_) p_->Release(); }
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    ComPtr(ComPtr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
    ComPtr& operator=(ComPtr&& o) noexcept
    {
        if (this != &o)
        {
            if (p_) p_->Release();
            p_ = o.p_;
            o.p_ = nullptr;
        }
        return *this;
    }
    T** operator&() { return &p_; }
    T*  get() const { return p_; }
    T*  operator->() const { return p_; }
    explicit operator bool() const { return p_ != nullptr; }

private:
    T* p_ = nullptr;
};

static std::wstring ToWide(const std::string& s)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w;
    if (len > 0)
    {
        w.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &w[0], len);
    }
    return w;
}

static BSTR MakeBstr(const std::string& s)
{
    std::wstring w = ToWide(s);
    return SysAllocString(w.c_str());
}

static std::string HrText(HRESULT hr)
{
    char buf[32];
    sprintf_s(buf, "0x%08X", static_cast<unsigned>(hr));
    return buf;
}

static std::string RuleName(const std::string& platform, const std::string& regionId, bool inbound)
{
    return std::string(kRulePrefix) + platform + "_" + regionId + (inbound ? "_IN" : "_OUT");
}

// Collects the IPv4 prefixes for all AWS regions except `selected` from the
// official ip-ranges.json endpoint.
static bool FetchBlockedIpPrefixes(const std::string& selectedId, const std::vector<Region>& all,
                                   std::vector<std::string>& ips, std::string& error)
{
    std::string json;
    if (!HttpUtil::GetText(kIpRangesUrl, json, error))
        return false;

    std::set<std::string> blocked; // region ids to block
    for (const auto& r : all)
    {
        if (r.id != selectedId)
            blocked.insert(r.id);
    }

    ips.clear();
    js::Parser parser(json);

    std::string region, ip;
    std::string::size_type prefixesPos = 0;
    const bool prefixesFound = parser.findKey("prefixes", prefixesPos);
    // findKey left the parser positioned at the start of the "prefixes" value.
    if (prefixesFound)
    {
        parser.forEachObjectInArray([&]() {
            for (;;)
            {
                if (parser.atObjectEnd())
                {
                    parser.eatObjectEnd();
                    break;
                }
                std::string key;
                if (!parser.nextMemberKey(key))
                    return false;
                std::string value;
                if (parser.parseString(value))
                {
                    if (key == "region")
                        region = value;
                    else if (key == "ip_prefix")
                        ip = value;
                }
                else
                {
                    parser.skipValue();
                }
            }
            if (!ip.empty() && blocked.count(region) > 0 && ip.find('.') != std::string::npos)
                ips.push_back(ip);
            region.clear();
            ip.clear();
            return true;
        });
    }

    if (ips.empty())
        error = "No IP ranges could be collected for the selected region set.";
    else
        error.clear();
    return !ips.empty();
}

static HRESULT EnumerateRules(INetFwRules* rules, const std::function<void(INetFwRule*, const std::wstring&)>& visit)
{
    if (!rules)
        return E_POINTER;

    HRESULT hr = S_OK;
    ComPtr<IUnknown> enumerator;
    hr = rules->get__NewEnum(&enumerator);
    if (FAILED(hr))
        return hr;

    ComPtr<IEnumVARIANT> variantEnum;
    hr = enumerator->QueryInterface(__uuidof(IEnumVARIANT), reinterpret_cast<void**>(&variantEnum));
    if (FAILED(hr))
        return hr;

    for (;;)
    {
        VARIANT v;
        VariantInit(&v);
        ULONG fetched = 0;
        HRESULT next = variantEnum->Next(1, &v, &fetched);
        if (next != S_OK || fetched == 0)
            break;

        if (v.vt == VT_DISPATCH && v.pdispVal)
        {
            ComPtr<INetFwRule> rule;
            if (SUCCEEDED(v.pdispVal->QueryInterface(__uuidof(INetFwRule), reinterpret_cast<void**>(&rule))))
            {
                BSTR name = nullptr;
                if (SUCCEEDED(rule->get_Name(&name)))
                {
                    std::wstring wname = name ? name : L"";
                    SysFreeString(name);
                    visit(rule.get(), wname);
                }
            }
        }
        VariantClear(&v);
    }
    return S_OK;
}

Result CreateRules(const Region& selected, const std::vector<Region>& all,
                   const std::string& exePath, const std::string& platform)
{
    std::vector<std::string> ips;
    std::string error;
    if (!FetchBlockedIpPrefixes(selected.id, all, ips, error))
    {
        return {false, error.empty() ? "No IP ranges could be collected for the selected region set." : error};
    }

    std::string remoteCsv;
    for (const auto& ip : ips)
    {
        if (!remoteCsv.empty())
            remoteCsv += ",";
        remoteCsv += ip;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // Only rewind initialization we performed ourselves (S_OK). S_FALSE means
    // the thread was already initialized elsewhere, RPC_E_CHANGED_MODE an MTA.
    const bool shouldUninit = (hr == S_OK);
    if (hr == RPC_E_CHANGED_MODE)
    {
        hr = S_OK;
    }
    if (FAILED(hr))
        return {false, "COM initialization failed (" + HrText(hr) + ")."};

    Result result{true, ""};
    do
    {
        ComPtr<INetFwPolicy2> policy;
        hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                              __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&policy));
        if (FAILED(hr))
        {
            result = {false, "Could not access the Windows Firewall service (" + HrText(hr) + ")."};
            break;
        }

        ComPtr<INetFwRules> rules;
        hr = policy->get_Rules(&rules);
        if (FAILED(hr))
        {
            result = {false, "Could not enumerate firewall rules (" + HrText(hr) + ")."};
            break;
        }

        for (int inbound = 1; inbound >= 0; inbound--)
        {
            ComPtr<INetFwRule> rule;
            hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER,
                                  __uuidof(INetFwRule), reinterpret_cast<void**>(&rule));
            if (FAILED(hr))
            {
                result = {false, "Could not create a firewall rule object (" + HrText(hr) + ")."};
                break;
            }

            std::string name = RuleName(platform, selected.id, inbound != 0);
            std::string program = exePath;
            BSTR bName = MakeBstr(name);
            BSTR bProg = MakeBstr(program);
            BSTR bRemote = MakeBstr(remoteCsv);
            BSTR bInterfaces = SysAllocString(L"All");
            BSTR bDesc = MakeBstr("Dead by Daylight server region blocker");

            hr = rule->put_Name(bName);
            if (SUCCEEDED(hr)) hr = rule->put_Description(bDesc);
            if (SUCCEEDED(hr)) hr = rule->put_Direction(inbound ? NET_FW_RULE_DIR_IN : NET_FW_RULE_DIR_OUT);
            if (SUCCEEDED(hr)) hr = rule->put_Action(NET_FW_ACTION_BLOCK);
            if (SUCCEEDED(hr)) hr = rule->put_Enabled(VARIANT_TRUE);
            if (SUCCEEDED(hr)) hr = rule->put_ApplicationName(bProg);
            if (SUCCEEDED(hr)) hr = rule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
            if (SUCCEEDED(hr)) hr = rule->put_InterfaceTypes(bInterfaces);
            if (SUCCEEDED(hr)) hr = rule->put_RemoteAddresses(bRemote);
            if (SUCCEEDED(hr)) hr = rules->Add(rule.get());

            SysFreeString(bName);
            SysFreeString(bProg);
            SysFreeString(bRemote);
            SysFreeString(bInterfaces);
            SysFreeString(bDesc);

            if (FAILED(hr))
            {
                result = {false, "Failed to add the firewall rule \"" + name + "\" (" + HrText(hr) + ")."};
                break;
            }
        }
        if (FAILED(hr))
            break;
    } while (false);

    if (shouldUninit)
        CoUninitialize();

    return result;
}

Result RemoveRulesFor(const std::string& platform)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninit = (hr == S_OK);
    if (hr == RPC_E_CHANGED_MODE)
    {
        hr = S_OK;
    }
    if (FAILED(hr))
        return {false, "COM initialization failed (" + HrText(hr) + ")."};

    Result result{true, ""};
    do
    {
        ComPtr<INetFwPolicy2> policy;
        hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                              __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&policy));
        if (FAILED(hr))
        {
            result = {false, "Could not access the Windows Firewall service (" + HrText(hr) + ")."};
            break;
        }

        ComPtr<INetFwRules> rules;
        hr = policy->get_Rules(&rules);
        if (FAILED(hr))
        {
            result = {false, "Could not enumerate firewall rules (" + HrText(hr) + ")."};
            break;
        }

        std::string prefix = std::string(kRulePrefix) + platform + "_";
        std::wstring wPrefix = ToWide(prefix);

        std::vector<std::wstring> toRemove;
        EnumerateRules(rules.get(), [&](INetFwRule*, const std::wstring& name) {
            if (name.size() >= wPrefix.size() &&
                _wcsnicmp(name.c_str(), wPrefix.c_str(), wPrefix.size()) == 0)
            {
                toRemove.push_back(name);
            }
        });

        for (const auto& name : toRemove)
        {
            BSTR bName = SysAllocString(name.c_str());
            rules->Remove(bName);
            SysFreeString(bName);
        }

        // Verify nothing remains for this platform.
        bool anyLeft = false;
        EnumerateRules(rules.get(), [&](INetFwRule*, const std::wstring& name) {
            if (name.size() >= wPrefix.size() &&
                _wcsnicmp(name.c_str(), wPrefix.c_str(), wPrefix.size()) == 0)
            {
                anyLeft = true;
            }
        });

        if (anyLeft)
            result = {false, "Rules not found or could not be removed."};
    } while (false);

    if (shouldUninit)
        CoUninitialize();

    return result;
}

void GetAppliedRule(const std::string& platform, bool& found, std::string& regionId)
{
    found = false;
    regionId.clear();

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool shouldUninit = (hr == S_OK);
    if (hr == RPC_E_CHANGED_MODE)
    {
        hr = S_OK;
    }
    if (FAILED(hr))
        return;

    ComPtr<INetFwPolicy2> policy;
    if (SUCCEEDED(CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                                   __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&policy))))
    {
        ComPtr<INetFwRules> rules;
        if (SUCCEEDED(policy->get_Rules(&rules)))
        {
            std::string prefix = std::string(kRulePrefix) + platform + "_";
            std::wstring wPrefix = ToWide(prefix);
            const std::string inSuffix = "_IN";
            const std::string outSuffix = "_OUT";

            EnumerateRules(rules.get(), [&](INetFwRule*, const std::wstring& name) {
                if (found)
                    return;
                std::wstring w = name;
                if (w.size() < wPrefix.size())
                    return;
                std::string sub;
                {
                    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
                    if (len > 0)
                    {
                        sub.resize(len);
                        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), &sub[0], len, nullptr, nullptr);
                    }
                }
                if (_wcsnicmp(w.c_str(), wPrefix.c_str(), wPrefix.size()) != 0)
                    return;
                found = true; // some rule matches the platform pattern

                std::string rest = sub.substr(prefix.size());
                if (util::endsWithIgnoreCase(rest, inSuffix))
                    rest = rest.substr(0, rest.size() - inSuffix.size());
                else if (util::endsWithIgnoreCase(rest, outSuffix))
                    rest = rest.substr(0, rest.size() - outSuffix.size());
                if (!rest.empty())
                    regionId = rest;
            });
        }
    }

    if (shouldUninit)
        CoUninitialize();
}

} // namespace FirewallService
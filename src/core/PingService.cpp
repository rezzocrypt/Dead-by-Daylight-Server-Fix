#include "PingService.h"

#include <atomic>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>   // GetAddrInfoW / FreeAddrInfoW
#include <iphlpapi.h>   // Icmp* import library
#include <icmpapi.h>    // IcmpCreateFile / IcmpSendEcho / IcmpCloseHandle

namespace PingService {

static std::mutex                      g_mutex;
static std::map<std::string, Result>   g_results;
static std::atomic<int>                g_pending{0};
static std::atomic<bool>               g_active{false};

bool IsCycleActive() { return g_active.load(); }

const Result* Find(const std::string& id)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    const auto it = g_results.find(id);
    return it == g_results.end() ? nullptr : &it->second;
}

static void PingRegion(const std::string& id, const std::string& host)
{
    Result result;
    // Resolve the hostname to an IPv4 address.
    std::wstring whost;
    {
        int len = MultiByteToWideChar(CP_ACP, 0, host.c_str(), static_cast<int>(host.size()), nullptr, 0);
        whost.resize(len);
        MultiByteToWideChar(CP_ACP, 0, host.c_str(), static_cast<int>(host.size()), &whost[0], len);
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    {
        ADDRINFOW hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        ADDRINFOW* res = nullptr;
        if (GetAddrInfoW(whost.c_str(), nullptr, &hints, &res) == 0 && res)
        {
            if (res->ai_addrlen >= sizeof(sockaddr_in))
                addr = *(reinterpret_cast<sockaddr_in*>(res->ai_addr));
            FreeAddrInfoW(res);
        }
        else
        {
            addr.sin_addr.s_addr = INADDR_NONE;
        }
    }

    if (addr.sin_addr.s_addr == INADDR_NONE)
    {
        // Unable to resolve -> treat as failure.
        std::lock_guard<std::mutex> lock(g_mutex);
        g_results[id] = {false, 0};
    }
    else
    {
        HANDLE handle = IcmpCreateFile();
        if (handle == INVALID_HANDLE_VALUE)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_results[id] = {false, 0};
        }
        else
        {
            const char* payload = "dbd region ping";
            char replyBuffer[2048] = {};
            DWORD replySize = static_cast<DWORD>(sizeof(replyBuffer));

            Result r;
            DWORD rc = IcmpSendEcho(handle, addr.sin_addr.s_addr, const_cast<char*>(payload),
                                    static_cast<WORD>(strlen(payload)), nullptr,
                                    replyBuffer, replySize, 3000);
            if (rc > 0)
            {
                ICMP_ECHO_REPLY* echo = reinterpret_cast<ICMP_ECHO_REPLY*>(replyBuffer);
                bool ok = (echo->Status == 0 /* IP_SUCCESS */);
                r = {ok, ok ? static_cast<long>(echo->RoundTripTime) : 0};
            }
            else
            {
                r = {false, 0};
            }
            IcmpCloseHandle(handle);

            {
                std::lock_guard<std::mutex> lock(g_mutex);
                g_results[id] = r;
            }
        }
    }

    if (g_pending.fetch_sub(1) == 1)
    {
        g_active.store(false);
    }
}

void StartCycle(const std::vector<Region>& regions)
{
    if (g_active.load())
        return;
    if (regions.empty())
        return;

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_results.clear();
    }
    g_active.store(true);
    g_pending.store(static_cast<int>(regions.size()));

    for (const auto& r : regions)
    {
        std::thread t(PingRegion, r.id, r.host);
        t.detach();
    }
}

} // namespace PingService
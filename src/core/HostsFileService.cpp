#include "HostsFileService.h"

#include <windows.h>

#include "DnsUtil.h"
#include "Region.h"
#include "RegionCatalog.h"
#include "Util.h"

namespace HostsFileService {

static const std::string HostsPath = "C:\\Windows\\System32\\drivers\\etc\\hosts";

static bool IsOwnedLine(const std::string& line)
{
    std::string t = util::trim(line);
    if (t == "#asia" || t == "#america" || t == "#europe")
        return true;
    return util::containsIgnoreCase(t, "gamelift.") && util::containsIgnoreCase(t, "amazonaws.com");
}

static void BuildBlock(const Region& selected, std::vector<std::string>& block)
{
    static const char* groups[] = {"#asia", "#america", "#europe"};
    for (const char* group : groups)
    {
        block.push_back(group);
        for (const auto& region : RegionCatalog::All())
        {
            if (region.hostsGroup != group)
                continue;
            bool commented = (region.id == selected.id);
            std::string prefix = commented ? "#0.0.0.0" : "0.0.0.0";
            block.push_back(prefix + " " + region.host + " # " + region.name + " " + region.id);
        }
    }
}

Result Apply(const Region& selected)
{
    try
    {
        std::vector<std::string> lines;
        if (util::fileExists(HostsPath))
        {
            bool ok = false;
            auto all = util::readAllLines(HostsPath, ok);
            if (ok)
            {
                for (const auto& line : all)
                {
                    if (!IsOwnedLine(line))
                        lines.push_back(line);
                }
            }
        }
        BuildBlock(selected, lines);
        if (!util::writeAllLines(HostsPath, lines))
        {
            return {false, "You do not have permission to modify the hosts file. Run the application as administrator."};
        }
        DnsUtil::Flush();
        return {true, ""};
    }
    catch (...)
    {
        return {false, "An error occurred while modifying the hosts file."};
    }
}

Result Clear()
{
    try
    {
        if (!util::fileExists(HostsPath))
            return {true, ""};

        bool ok = false;
        auto all = util::readAllLines(HostsPath, ok);
        if (!ok)
            return {false, "Could not read the hosts file."};

        std::vector<std::string> cleaned;
        for (const auto& line : all)
        {
            if (!IsOwnedLine(line))
                cleaned.push_back(line);
        }

        if (!util::writeAllLines(HostsPath, cleaned))
        {
            return {false, "You do not have permission to modify the hosts file. Run the application as administrator."};
        }
        DnsUtil::Flush();
        return {true, ""};
    }
    catch (...)
    {
        return {false, "An error occurred while modifying the hosts file."};
    }
}

std::string GetAppliedRegionId()
{
    try
    {
        if (!util::fileExists(HostsPath))
            return "";

        bool ok = false;
        auto lines = util::readAllLines(HostsPath, ok);
        if (!ok)
            return "";

        std::vector<std::string> commented;
        for (const auto& line : lines)
        {
            std::string t = util::trim(line);
            if (!t.empty() && t[0] == '#')
                commented.push_back(t);
        }

        for (const auto& region : RegionCatalog::All())
        {
            for (const auto& c : commented)
            {
                if (util::containsIgnoreCase(c, region.host))
                    return region.id;
            }
        }
        return "";
    }
    catch (...)
    {
        return "";
    }
}

} // namespace HostsFileService
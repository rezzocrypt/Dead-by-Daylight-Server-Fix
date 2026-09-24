#pragma once
#include <string>
#include <vector>

#include "Region.h"

// Windows Firewall rules via the INetFwPolicy2 COM API (mirror of FirewallService.cs,
// which used PowerShell's New-NetFirewallRule / Remove-NetFirewallRule).
namespace FirewallService {

struct Result
{
    bool        ok = false;
    std::string message;
};

// Blocks every region except `selected` for the given game executable.
// Rule names follow the pattern DbdBlockRule<platform>_<regionId>_IN / _OUT.
Result CreateRules(const Region& selected, const std::vector<Region>& all,
                   const std::string& exePath, const std::string& platform);

// Removes all rules whose name starts with DbdBlockRule<platform>_.
Result RemoveRulesFor(const std::string& platform);

// Looks up existing rules: returns found=true when any rule for the platform
// exists; regionId is filled from the first matching rule name when parseable.
void   GetAppliedRule(const std::string& platform, bool& found, std::string& regionId);

std::string RulePrefixFor(const std::string& platform);

} // namespace FirewallService
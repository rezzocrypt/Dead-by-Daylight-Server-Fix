#pragma once
#include <string>

#include "Region.h"

// Hosts file editing (mirror of HostsFileService.cs).
namespace HostsFileService {

struct Result
{
    bool        ok = false;
    std::string message;
};

// Applies the block for the selected region: every region except the selected
// one gets an active "0.0.0.0 <host>" line; the selected region is commented out.
Result Apply(const Region& selected);

// Removes every line owned by this application from the hosts file.
Result Clear();

// Returns the id of the applied region (the one whose host line is commented out),
// or an empty string when nothing is applied.
std::string GetAppliedRegionId();

} // namespace HostsFileService
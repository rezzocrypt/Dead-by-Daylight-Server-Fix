#pragma once
#include <string>
#include <vector>

#include "Region.h"

// Latency measurement to the GameLift endpoints (IcmpSendEcho, worker threads).
namespace PingService {

struct Result
{
    bool ok = false;    // ICMP reply received
    long rttMs = 0;     // round-trip time in milliseconds (valid when ok == true)
};

// Starts a fresh measurement cycle for all regions. No-op while a cycle runs.
void StartCycle(const std::vector<Region>& regions);

bool IsCycleActive();
// Latest result for a region id, or nullptr while it is not yet measured.
const Result* Find(const std::string& id);

} // namespace PingService
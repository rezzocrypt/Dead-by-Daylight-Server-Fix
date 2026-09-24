#pragma once
#include <string>

// A single GameLift region catalogue entry (mirror of the .NET GameliftRegion record).
struct Region
{
    std::string id;         // AWS region id, e.g. "eu-central-1"
    std::string name;       // display name, e.g. "Europe (Frankfurt)"
    std::string host;       // resolved endpoint (gamelift.<id>.amazonaws.com unless overridden)
    std::string hostsGroup; // section header used in the hosts file ("asia" | "america" | "europe")
};
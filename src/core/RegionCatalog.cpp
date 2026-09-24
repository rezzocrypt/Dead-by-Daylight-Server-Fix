#include "RegionCatalog.h"

#include <string>
#include <utility>
#include <vector>

namespace RegionCatalog {

static std::vector<Region> g_regions;

const std::vector<Region>& All()
{
    return g_regions;
}

const Region* Find(const std::string& id)
{
    for (const auto& r : g_regions)
    {
        if (r.id == id)
            return &r;
    }
    return nullptr;
}

namespace {
struct RegionDef
{
    const char* id;
    const char* name;
    const char* group;
};
} // namespace

// Table extracted from the original regions.json; the GameLift host is always
// gamelift.<id>.amazonaws.com.
static const RegionDef kRegionDefs[] = {
    // Asia Pacific
    {"ap-south-1", "Asia Pacific (Mumbai)", "asia"},
    {"ap-east-1", "Asia Pacific (Hong Kong)", "asia"},
    {"ap-northeast-1", "Asia Pacific (Tokyo)", "asia"},
    {"ap-northeast-2", "Asia Pacific (Seoul)", "asia"},
    {"ap-southeast-1", "Asia Pacific (Singapore)", "asia"},
    {"ap-southeast-2", "Asia Pacific (Sydney)", "asia"},
    // Americas
    {"ca-central-1", "Canada (Central)", "america"},
    {"us-east-1", "US East (N. Virginia)", "america"},
    {"us-east-2", "US East (Ohio)", "america"},
    {"us-west-1", "US West (N. California)", "america"},
    {"us-west-2", "US West (Oregon)", "america"},
    {"sa-east-1", "South America (Sao Paulo)", "america"},
    // Europe
    {"eu-central-1", "Europe (Frankfurt)", "europe"},
    {"eu-west-1", "Europe (Ireland)", "europe"},
    {"eu-west-2", "Europe (London)", "europe"},
};

void Initialize()
{
    g_regions.clear();
    g_regions.reserve(sizeof(kRegionDefs) / sizeof(kRegionDefs[0]));
    for (const auto& d : kRegionDefs)
    {
        Region r;
        r.id = d.id;
        r.name = d.name;
        r.host = std::string("gamelift.") + d.id + ".amazonaws.com";
        r.hostsGroup = d.group;
        g_regions.push_back(std::move(r));
    }
}

} // namespace RegionCatalog
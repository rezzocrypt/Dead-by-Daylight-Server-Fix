#pragma once
#include <string>
#include <vector>

#include "Region.h"

// Region catalogue. The list is compiled into the binary (mirror of
// RegionCatalog.cs). Order is preserved from the original regions.json;
// this is also the hosts-file order.
namespace RegionCatalog {

const std::vector<Region>& All();
// Returns a pointer to a region by id, or nullptr.
const Region*              Find(const std::string& id);

// Fills the catalogue from the embedded table. Not thread-safe; call once at
// startup.
void Initialize();

} // namespace RegionCatalog
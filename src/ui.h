#pragma once
#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "core/Region.h"

namespace ui {

struct OpResultMsg
{
    bool        ok = false;
    std::string message;
};

// Mutable application state, owned exclusively by the UI thread (worker threads
// only write the fields guarded by opDone / PingService).
struct AppState
{
    std::vector<Region> regions;
    std::string         selectedRegionId;
    std::string         appliedZoneId;

    std::string platform = "STEAM"; // STEAM | EGS | MS
    std::string exePath;
    std::string pathLabel = "Path to DeadByDaylight binary file";

    bool busy = false;

    std::thread       opThread; // joined by UI thread once opDone flips
    std::atomic<bool> opDone{false};
    OpResultMsg       opResult; // read by UI thread only after join

    bool firstFrame = true;

    // ping clock
    bool   pingNeverStarted = true;
    bool   pingWasActive = false;
    double pingElapsed = 0.0;
};

// Loads the regions catalogue (fatal on failure) - call once at startup.
bool Initialize(AppState& app, std::string& fatalMessage);

// Applies the dark colour scheme to the current ImGui context.
void ApplyDefaultStyle();

// Renders the whole UI once per frame.
void BuildUI(AppState& app);

// Starts the background "Create Rules" / "Remove Rules" operations.
void StartCreateRules(AppState& app);
void StartRemoveRules(AppState& app);

// Called by the main loop; finishes background operations, shows results,
// refreshes the applied zone and re-pings.
void PollOperation(AppState& app);

// Keeps the ping cycle alive (15 s interval, auto-select of the best row).
void TickPings(AppState& app, double deltaSeconds);

extern const char* kVersion;
extern const char* kGithubUrl;

void ShowInfo(bool isError, const std::string& message);

} // namespace ui
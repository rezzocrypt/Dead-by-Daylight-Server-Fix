#include "ui.h"

#include <cstdio>
#include <string>

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <imgui.h>

#include "core/DnsUtil.h"
#include "core/FirewallService.h"
#include "core/HostsFileService.h"
#include "core/PingService.h"
#include "core/RegionCatalog.h"
#include "core/Util.h"

// ---- shared with main.cpp ----
extern HWND     g_hwnd;
extern ImFont*  g_fontTitle;
extern ImFont*  g_fontBase;

namespace ui {

const char* kVersion   = "v1.2.0";
const char* kGithubUrl = "https://github.com/rezzocrypt/Dead-by-Daylight-Server-Fix";

static const ImVec4 rgb(int r, int g, int b, float a = 1.0f)
{
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

// Palette ported from the original WinForms theme (Themes.cs).
static const ImVec4 C_Background      = rgb(17, 21, 28);
static const ImVec4 C_Surface         = rgb(26, 33, 42);
static const ImVec4 C_SurfaceAlt      = rgb(34, 44, 56);
static const ImVec4 C_Field           = rgb(11, 14, 19);
static const ImVec4 C_Border          = rgb(42, 52, 66);
static const ImVec4 C_Text            = rgb(232, 236, 241);
static const ImVec4 C_TextMuted       = rgb(152, 162, 179);
static const ImVec4 C_Accent          = rgb(61, 126, 255);
static const ImVec4 C_AccentHover     = rgb(86, 140, 255);
static const ImVec4 C_AccentPressed   = rgb(43, 103, 215);
static const ImVec4 C_Success         = rgb(46, 184, 114);
static const ImVec4 C_Warning         = rgb(245, 166, 35);
static const ImVec4 C_Danger          = rgb(229, 72, 77);
static const ImVec4 C_Link            = rgb(127, 176, 255);
static const ImVec4 C_Selection       = rgb(31, 58, 103);
static const ImVec4 C_SelectionHover  = rgb(45, 72, 122);

namespace {

std::string exeNameForPlatform(const std::string& platform)
{
    if (platform == "EGS") return "DeadByDaylight-EGS-Shipping.exe";
    if (platform == "MS")  return "DeadByDaylight-WinGDK-Shipping.exe";
    return "DeadByDaylight-Win64-Shipping.exe";
}

std::string DetectPlatform(const std::string& filePath)
{
    const char* known[] = {
        "DeadByDaylight-Win64-Shipping.exe",
        "DeadByDaylight-EGS-Shipping.exe",
        "DeadByDaylight-WinGDK-Shipping.exe",
    };
    const char* platform[] = {"STEAM", "EGS", "MS"};

    size_t slash = filePath.find_last_of("\\/");
    std::string name = (slash == std::string::npos) ? filePath : filePath.substr(slash + 1);
    for (int i = 0; i < 3; i++)
    {
        if (util::endsWithIgnoreCase(name, known[i]))
            return platform[i];
    }
    return "";
}

void OpenGithub()
{
    ShellExecuteW(nullptr, L"open", L"https://github.com/rezzocrypt/Dead-by-Daylight-Server-Fix",
                  nullptr, nullptr, SW_SHOWNORMAL);
}

void BrowseForExecutable(AppState& app)
{
    char file[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select game executable";
    ofn.lpstrFilter = "Dead by Daylight executables\0DeadByDaylight-*.exe\0All files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn))
        return;

    std::string platform = DetectPlatform(file);
    if (platform.empty())
    {
        ShowInfo(true, "Unrecognized executable file");
        return;
    }

    app.platform = platform;
    app.exePath = file;
    app.pathLabel = "Path to " + exeNameForPlatform(platform);
}

void RefreshAppliedZone(AppState& app)
{
    bool found = false;
    std::string ruleRegion;
    FirewallService::GetAppliedRule(app.platform, found, ruleRegion);

    std::string zoneId = HostsFileService::GetAppliedRegionId();
    if (zoneId.empty() && found)
        zoneId = ruleRegion;

    app.appliedZoneId = (zoneId.empty() || RegionCatalog::Find(zoneId) == nullptr) ? "" : zoneId;
}

void DrawServerTable(AppState& app)
{
    const ImVec2 tableSize(712.0f, 372.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));
    if (ImGui::BeginTable("servers", 3,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV |
                              ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoBordersInBody,
                          tableSize))
    {
        ImGui::TableSetupColumn("Zone", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Host", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Ping", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        for (size_t idx = 0; idx < app.regions.size(); idx++)
        {
            const Region& r = app.regions[idx];
            const bool isApplied = (r.id == app.appliedZoneId);
            const bool isSelected = (r.id == app.selectedRegionId);
            const PingService::Result* ping = PingService::Find(r.id);

            ImGui::TableNextRow(0, 24.0f);
            ImGui::TableSetColumnIndex(0);

            char idBuf[32];
            sprintf_s(idBuf, "##row%zu", idx);
            if (ImGui::Selectable(idBuf, isSelected, ImGuiSelectableFlags_SpanAllColumns))
                app.selectedRegionId = r.id;

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, isApplied ? C_Success : C_Text);
            if (isApplied)
            {
                ImGui::TextUnformatted("\u2713 ");
                ImGui::SameLine(0.0f, 0.0f);
            }
            ImGui::TextUnformatted(r.name.c_str());
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(1);
            ImGui::PushStyleColor(ImGuiCol_Text, isApplied ? C_Success : C_Text);
            ImGui::TextUnformatted(r.host.c_str());
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(2);
            if (ping != nullptr && ping->ok)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ping->rttMs < 100 ? C_Success
                                                    : ping->rttMs < 200 ? C_Warning : C_Danger);
                ImGui::Text("%ld ms", ping->rttMs);
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ping == nullptr ? C_TextMuted : C_Danger);
                ImGui::TextUnformatted(ping == nullptr ? "ping" : "Error");
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

} // namespace

bool Initialize(AppState& app, std::string& fatalMessage)
{
    RegionCatalog::Initialize();
    if (RegionCatalog::All().empty())
    {
        fatalMessage = "The region list is empty.";
        return false;
    }
    app.regions = RegionCatalog::All();
    return true;
}

void ApplyDefaultStyle()
{
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 0.0f;
    st.ChildRounding = 4.0f;
    st.PopupRounding = 4.0f;
    st.FrameRounding = 4.0f;
    st.WindowBorderSize = 0.0f;
    st.WindowPadding = ImVec2(10, 10);
    st.FramePadding = ImVec2(10, 5);
    st.ItemSpacing = ImVec2(8, 6);

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = C_Background;
    c[ImGuiCol_ChildBg] = C_Field;
    c[ImGuiCol_PopupBg] = C_Surface;
    c[ImGuiCol_Border] = C_Border;
    c[ImGuiCol_FrameBg] = C_Field;
    c[ImGuiCol_FrameBgHovered] = C_Surface;
    c[ImGuiCol_FrameBgActive] = C_SurfaceAlt;
    c[ImGuiCol_Text] = C_Text;
    c[ImGuiCol_TextDisabled] = C_TextMuted;
    c[ImGuiCol_Button] = C_Surface;
    c[ImGuiCol_ButtonHovered] = C_SurfaceAlt;
    c[ImGuiCol_ButtonActive] = C_AccentPressed;
    c[ImGuiCol_Header] = C_Selection;
    c[ImGuiCol_HeaderHovered] = C_SelectionHover;
    c[ImGuiCol_HeaderActive] = C_AccentPressed;
    c[ImGuiCol_TableHeaderBg] = C_Surface;
    c[ImGuiCol_TableBorderStrong] = C_Border;
    c[ImGuiCol_TableBorderLight] = rgb(36, 45, 58);
    c[ImGuiCol_TableRowBg] = C_Background;
    c[ImGuiCol_TableRowBgAlt] = rgb(21, 27, 35);
    c[ImGuiCol_CheckMark] = C_Accent;
    c[ImGuiCol_Separator] = C_Border;
    c[ImGuiCol_ScrollbarBg] = C_Field;
    c[ImGuiCol_ScrollbarGrab] = C_Border;
    c[ImGuiCol_ScrollbarGrabHovered] = C_SurfaceAlt;
    c[ImGuiCol_ScrollbarGrabActive] = C_Accent;
    c[ImGuiCol_TitleBg] = C_Background;
    c[ImGuiCol_TitleBgActive] = C_Background;
    c[ImGuiCol_TitleBgCollapsed] = C_Background;
    c[ImGuiCol_TextSelectedBg] = C_Selection;
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
}

void ShowInfo(bool isError, const std::string& message)
{
    MessageBoxA(g_hwnd, message.c_str(), "Dead by Daylight Region Selector",
                MB_OK | (isError ? MB_ICONERROR : MB_ICONINFORMATION));
}

void TickPings(AppState& app, double deltaSeconds)
{
    const bool active = PingService::IsCycleActive();

    if (!active && app.pingNeverStarted)
    {
        PingService::StartCycle(app.regions);
        app.pingNeverStarted = false;
        app.pingWasActive = true;
        app.pingElapsed = 0.0;
        return;
    }

    if (app.pingWasActive && !active)
    {
        // Cycle completed -> pick the row with the lowest latency.
        long best = 0x7FFFFFFF;
        std::string bestId;
        for (const auto& r : app.regions)
        {
            const PingService::Result* pr = PingService::Find(r.id);
            if (pr && pr->ok && pr->rttMs >= 0 && pr->rttMs < best)
            {
                best = pr->rttMs;
                bestId = r.id;
            }
        }
        if (!bestId.empty())
            app.selectedRegionId = bestId;
        app.pingWasActive = false;
        return;
    }

    if (active)
    {
        app.pingWasActive = true;
        app.pingElapsed = 0.0;
        return;
    }

    app.pingElapsed += deltaSeconds;
    if (app.pingElapsed >= 15.0)
    {
        PingService::StartCycle(app.regions);
        app.pingWasActive = true;
        app.pingElapsed = 0.0;
    }
}

void PollOperation(AppState& app)
{
    if (!app.opDone.load())
        return;

    if (app.opThread.joinable())
        app.opThread.join();
    app.opDone.store(false);
    app.busy = false;

    if (app.opResult.ok)
        ShowInfo(false, app.opResult.message);
    else
        ShowInfo(true, app.opResult.message);

    RefreshAppliedZone(app);
    PingService::StartCycle(app.regions);
}

void StartCreateRules(AppState& app)
{
    if (app.busy)
        return;

    const Region* selected = RegionCatalog::Find(app.selectedRegionId);
    if (selected == nullptr)
    {
        ShowInfo(true, "Please select a region");
        return;
    }
    if (app.exePath.empty())
    {
        ShowInfo(true, "Please specify the path to the executable file");
        return;
    }

    Region selectedCopy = *selected;
    std::string exe = app.exePath;
    std::string platform = app.platform;
    std::vector<Region> snapshot = app.regions;

    app.busy = true;
    app.opDone.store(false);
    app.opResult = {false, ""};

    app.opThread = std::thread([&app, selectedCopy, exe, platform, snapshot]() {
        auto hosts = HostsFileService::Apply(selectedCopy);
        OpResultMsg result;
        if (!hosts.ok)
        {
            result = {false, hosts.message};
        }
        else
        {
            auto fw = FirewallService::CreateRules(selectedCopy, snapshot, exe, platform);
            if (fw.ok)
            {
                DnsUtil::Flush();
                result = {true, "Rules have been successfully added"};
            }
            else
            {
                result = {false, fw.message};
            }
        }
        app.opResult = result;
        app.opDone.store(true);
    });
}

void StartRemoveRules(AppState& app)
{
    if (app.busy)
        return;

    if (app.selectedRegionId.empty())
    {
        ShowInfo(true, "Please select the region to be deleted");
        return;
    }

    std::string platform = app.platform;

    app.busy = true;
    app.opDone.store(false);
    app.opResult = {false, ""};

    app.opThread = std::thread([&app, platform]() {
        auto hosts = HostsFileService::Clear();
        OpResultMsg result;
        if (!hosts.ok)
        {
            result = {false, hosts.message};
        }
        else
        {
            auto fw = FirewallService::RemoveRulesFor(platform);
            if (fw.ok)
            {
                DnsUtil::Flush();
                result = {true, "Rules have been successfully removed"};
            }
            else
            {
                result = {false, fw.message};
            }
        }
        app.opResult = result;
        app.opDone.store(true);
    });
}

void BuildUI(AppState& app)
{
    if (app.firstFrame)
    {
        app.firstFrame = false;
        RefreshAppliedZone(app);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(760, 622));
    ImGui::Begin("##main", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);

    // Title
    ImGui::SetCursorPos(ImVec2(12, 7));
    if (g_fontTitle)
        ImGui::PushFont(g_fontTitle);
    ImGui::TextUnformatted("Dead by Daylight");
    if (g_fontTitle)
        ImGui::PopFont();

    // Subtitle (14 px spacing under the larger title)
    ImGui::SetCursorPos(ImVec2(24, 46));
    ImGui::PushStyleColor(ImGuiCol_Text, C_TextMuted);
    ImGui::TextUnformatted("region selector");
    ImGui::PopStyleColor();

    ImGui::BeginDisabled(app.busy);

    // Server table
    ImGui::SetCursorPos(ImVec2(24, 84));
    DrawServerTable(app);

    // Path to game executable
    ImGui::SetCursorPos(ImVec2(24, 470));
    ImGui::PushStyleColor(ImGuiCol_Text, C_TextMuted);
    ImGui::TextUnformatted(app.pathLabel.c_str());
    ImGui::PopStyleColor();

    char pathBuf[1024] = {};
    if (app.exePath.size() < sizeof(pathBuf))
        memcpy(pathBuf, app.exePath.c_str(), app.exePath.size());

    ImGui::SetCursorPos(ImVec2(24, 490));
    ImGui::SetNextItemWidth(550.0f);
    ImGui::InputText("##path", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_ReadOnly);
    if (app.exePath != pathBuf)
        app.exePath = pathBuf;

    ImGui::SetCursorPos(ImVec2(584, 490));
    if (ImGui::Button("Browse", ImVec2(152, 26)))
        BrowseForExecutable(app);

    ImGui::EndDisabled();

    if (app.busy)
    {
        ImGui::SetCursorPos(ImVec2(24, 522));
        ImGui::PushStyleColor(ImGuiCol_Text, C_TextMuted);
        ImGui::TextUnformatted("Working in progress...");
        ImGui::PopStyleColor();
    }

    ImGui::BeginDisabled(app.busy);

    // Remove Rules (danger)
    ImGui::SetCursorPos(ImVec2(422, 548));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, C_Background);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C_Surface);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, C_SurfaceAlt);
    ImGui::PushStyleColor(ImGuiCol_Border, C_Danger);
    ImGui::PushStyleColor(ImGuiCol_Text, C_Danger);
    if (ImGui::Button("Remove Rules", ImVec2(152, 39)))
        StartRemoveRules(app);
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);

    // Create Rules (primary)
    ImGui::SetCursorPos(ImVec2(584, 548));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, C_Accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C_AccentHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, C_AccentPressed);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    if (ImGui::Button("Create Rules", ImVec2(152, 39)))
        StartCreateRules(app);
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();

    ImGui::EndDisabled();

    // Status footer
    {
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 lineA(winPos.x, winPos.y + 600.0f);
        ImVec2 lineB(winPos.x + 760.0f, winPos.y + 600.0f);
        ImGui::GetWindowDrawList()->AddLine(lineA, lineB, ImGui::GetColorU32(C_Border), 1.0f);

        // GitHub link
        ImGui::SetCursorPos(ImVec2(10, 601));
        ImGui::InvisibleButton("##github", ImVec2(730, 18));
        const bool linkHovered = ImGui::IsItemHovered();
        const bool linkClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        if (linkHovered)
        {
            ImGui::SetTooltip("Open repository in browser");
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, linkHovered ? C_Accent : C_Link);
        // The InvisibleButton consumed the cursor row; draw text right after it.
        ImGui::SetCursorPos(ImVec2(10, 601));
        ImGui::TextUnformatted(kGithubUrl);
        ImGui::PopStyleColor();
        if (linkClicked)
            OpenGithub();

        // Version (right-aligned)
        ImVec2 vsz = ImGui::CalcTextSize(kVersion);
        ImGui::SetCursorPos(ImVec2(760.0f - 10.0f - vsz.x, 601.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, C_TextMuted);
        ImGui::TextUnformatted(kVersion);
        ImGui::PopStyleColor();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace ui
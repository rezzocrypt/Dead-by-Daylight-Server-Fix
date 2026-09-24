#include <d3d11.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <windowsx.h>

#include <algorithm>
#include <chrono>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

// The Win32 backend intentionally leaves this declaration out of the header
// (see the "#if 0" in imgui_impl_win32.h) - forward-declare it ourselves.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "core/Util.h"
#include "ui.h"

// Forward declarations
static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Data
static ID3D11Device*           g_pd3dDevice = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*         g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

HWND    g_hwnd = nullptr;
ImFont* g_fontTitle = nullptr;
ImFont* g_fontBase = nullptr;

// Main code
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    ui::AppState app;
    {
        std::string fatal;
        if (!ui::Initialize(app, fatal))
        {
            MessageBoxA(nullptr, fatal.c_str(), "Dead by Daylight Region Selector",
                        MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    constexpr int clientWidth = 760;
    constexpr int clientHeight = 622;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.lpszClassName = L"DeadByDaylightRegionSelector";
    RegisterClassExW(&wc);

    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    RECT rect = {0, 0, clientWidth, clientHeight};
    AdjustWindowRect(&rect, windowStyle, FALSE);

    int winW = rect.right - rect.left;
    int winH = rect.bottom - rect.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
    int y = std::max(0, (GetSystemMetrics(SM_CYSCREEN) - winH) / 2);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Dead by Daylight Region Selector",
                                windowStyle, x, y, winW, winH, nullptr, nullptr, hInstance, nullptr);
    g_hwnd = hwnd;

    if (!hwnd)
    {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        MessageBoxA(nullptr, "Failed to create a Direct3D 11 device.", "Dead by Daylight Region Selector",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // do not write imgui.ini

    ui::ApplyDefaultStyle();

    // Fonts (Segoe UI, 100% DPI). Fall back to the built-in font when missing.
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
    builder.AddChar(0x2713); // check mark ✓ used for the applied zone
    ImVector<ImWchar> glyphRanges;
    builder.BuildRanges(&glyphRanges);

    g_fontBase = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f, nullptr,
                                              glyphRanges.Data);
    g_fontTitle = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeuib.ttf", 30.0f, nullptr,
                                               glyphRanges.Data);
    if (g_fontBase == nullptr)
        g_fontBase = io.Fonts->AddFontDefault();
    if (g_fontTitle == nullptr)
        g_fontTitle = g_fontBase;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    bool running = true;
    auto lastUpdate = std::chrono::steady_clock::now();

    while (running)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
        if (!running)
            break;

        ui::PollOperation(app);

        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - lastUpdate).count();
        lastUpdate = now;
        ui::TickPings(app, dt);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ui::BuildUI(app);

        ImGui::Render();
        const float clearColor[4] = {17.0f / 255.0f, 21.0f / 255.0f, 28.0f / 255.0f, 1.0f};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Win32 message handler (forwarded to ImGui first)
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// DX11 device + swapchain
static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

static void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // fallback: WARP software rasterizer
    {
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}
cmake_minimum_required(VERSION 3.20)
project(DeadByDaylightServerFix LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "This application is Windows-only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Dear ImGui (downloaded on first configure) ----
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD) # we populate manually and build our own target
endif()
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG        v1.92.9
  GIT_SHALLOW    TRUE)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp)

add_library(dearimgui STATIC ${IMGUI_SOURCES})
target_include_directories(dearimgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends)
set_target_properties(dearimgui PROPERTIES CXX_STANDARD 17)

# ---- Application ----
add_executable(dbd_server_fix WIN32
  app.rc
  src/main.cpp
  src/ui.cpp
  src/core/Util.cpp
  src/core/Json.cpp
  src/core/RegionCatalog.cpp
  src/core/HostsFileService.cpp
  src/core/DnsUtil.cpp
  src/core/HttpUtil.cpp
  src/core/PingService.cpp
  src/core/FirewallService.cpp)

target_include_directories(dbd_server_fix PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core)

target_compile_definitions(dbd_server_fix PRIVATE
  _WIN32_WINNT=0x0601
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  UNICODE
  _UNICODE)

target_compile_options(dbd_server_fix PRIVATE /W3 /permissive-)

# The application manifest (requireAdministrator + DPI-aware) is already a
# resource in app.rc, so tell the linker not to embed its own one.
target_link_options(dbd_server_fix PRIVATE /MANIFEST:NO)

target_link_libraries(dbd_server_fix PRIVATE
  dearimgui
  d3d11 dxgi d3dcompiler
  ole32 oleaut32 uuid
  user32 gdi32 shell32 comdlg32
  iphlpapi ws2_32 wininet)
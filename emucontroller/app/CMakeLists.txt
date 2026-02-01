cmake_minimum_required(VERSION 3.16)

# Fetch ImGui
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.6
)
FetchContent_MakeAvailable(imgui)

# ImGui library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# EmuController GUI Application
add_executable(EmuController WIN32
    src/main.cpp
    src/ipc_client.cpp
)

target_include_directories(EmuController PRIVATE
    src
)

target_link_libraries(EmuController PRIVATE
    imgui
    d3d11
    dxgi
)

target_compile_definitions(EmuController PRIVATE
    UNICODE
    _UNICODE
)

# Set subsystem to Windows (GUI app)
set_target_properties(EmuController PROPERTIES
    WIN32_EXECUTABLE TRUE
)

// EmuController - OpenVR Controller Emulator GUI

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <windows.h>
#include <tchar.h>
#include <cmath>

#include "input_state.h"
#include "ipc_client.h"

// DirectX 11 globals
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Input state
static emu::ControllerInputState g_leftController;
static emu::ControllerInputState g_rightController;
static emu::KeyMapping g_keyMapping;

// HMD state
static emu::HMDPose g_hmdPose;
static emu::HMDKeyMapping g_hmdKeyMapping;
static emu::HMDSettings g_hmdSettings;
static bool g_mouseLookEnabled = false;
static POINT g_lastMousePos = { 0, 0 };
static bool g_wasRightButtonDown = false;

// IPC Client
static emu::IPCClient g_ipcClient;

// Update input state from keyboard
void UpdateInput(HWND hwnd) {
    (void)hwnd;

    // Reset stick positions (they return to center)
    g_leftController.stick_x = 0.0f;
    g_leftController.stick_y = 0.0f;
    g_rightController.stick_x = 0.0f;
    g_rightController.stick_y = 0.0f;

    // Left stick - WASD
    if (GetAsyncKeyState(g_keyMapping.left_stick_up) & 0x8000)
        g_leftController.stick_y = 1.0f;
    if (GetAsyncKeyState(g_keyMapping.left_stick_down) & 0x8000)
        g_leftController.stick_y = -1.0f;
    if (GetAsyncKeyState(g_keyMapping.left_stick_left) & 0x8000)
        g_leftController.stick_x = -1.0f;
    if (GetAsyncKeyState(g_keyMapping.left_stick_right) & 0x8000)
        g_leftController.stick_x = 1.0f;

    // Normalize diagonal movement (left)
    float len = sqrtf(g_leftController.stick_x * g_leftController.stick_x +
                      g_leftController.stick_y * g_leftController.stick_y);
    if (len > 1.0f) {
        g_leftController.stick_x /= len;
        g_leftController.stick_y /= len;
    }

    // Right stick - Arrow Keys
    if (GetAsyncKeyState(g_keyMapping.right_stick_up) & 0x8000)
        g_rightController.stick_y = 1.0f;
    if (GetAsyncKeyState(g_keyMapping.right_stick_down) & 0x8000)
        g_rightController.stick_y = -1.0f;
    if (GetAsyncKeyState(g_keyMapping.right_stick_left) & 0x8000)
        g_rightController.stick_x = -1.0f;
    if (GetAsyncKeyState(g_keyMapping.right_stick_right) & 0x8000)
        g_rightController.stick_x = 1.0f;

    // Normalize diagonal movement (right)
    len = sqrtf(g_rightController.stick_x * g_rightController.stick_x +
                g_rightController.stick_y * g_rightController.stick_y);
    if (len > 1.0f) {
        g_rightController.stick_x /= len;
        g_rightController.stick_y /= len;
    }

    // Left controller buttons
    g_leftController.SetButton(emu::BTN_A, GetAsyncKeyState(g_keyMapping.left_a) & 0x8000);
    g_leftController.SetButton(emu::BTN_B, GetAsyncKeyState(g_keyMapping.left_b) & 0x8000);
    g_leftController.SetButton(emu::BTN_X, GetAsyncKeyState(g_keyMapping.left_x) & 0x8000);
    g_leftController.SetButton(emu::BTN_Y, GetAsyncKeyState(g_keyMapping.left_y) & 0x8000);
    g_leftController.SetButton(emu::BTN_L3, GetAsyncKeyState(g_keyMapping.left_stick_click) & 0x8000);
    g_leftController.SetButton(emu::BTN_LB, GetAsyncKeyState(g_keyMapping.left_bumper) & 0x8000);

    // Left trigger/grip
    g_leftController.trigger = (GetAsyncKeyState(g_keyMapping.left_trigger) & 0x8000) ? 1.0f : 0.0f;
    g_leftController.grip = (GetAsyncKeyState(g_keyMapping.left_grip) & 0x8000) ? 1.0f : 0.0f;

    // Right controller buttons
    g_rightController.SetButton(emu::BTN_A, GetAsyncKeyState(g_keyMapping.right_a) & 0x8000);
    g_rightController.SetButton(emu::BTN_B, GetAsyncKeyState(g_keyMapping.right_b) & 0x8000);
    g_rightController.SetButton(emu::BTN_X, GetAsyncKeyState(g_keyMapping.right_x) & 0x8000);
    g_rightController.SetButton(emu::BTN_Y, GetAsyncKeyState(g_keyMapping.right_y) & 0x8000);
    g_rightController.SetButton(emu::BTN_R3, GetAsyncKeyState(g_keyMapping.right_stick_click) & 0x8000);
    g_rightController.SetButton(emu::BTN_RB, GetAsyncKeyState(g_keyMapping.right_bumper) & 0x8000);

    // Right trigger/grip
    g_rightController.trigger = (GetAsyncKeyState(g_keyMapping.right_trigger) & 0x8000) ? 1.0f : 0.0f;
    g_rightController.grip = (GetAsyncKeyState(g_keyMapping.right_grip) & 0x8000) ? 1.0f : 0.0f;

    // D-Pad (numpad)
    bool dpadUp = GetAsyncKeyState(g_keyMapping.dpad_up) & 0x8000;
    bool dpadDown = GetAsyncKeyState(g_keyMapping.dpad_down) & 0x8000;
    bool dpadLeft = GetAsyncKeyState(g_keyMapping.dpad_left) & 0x8000;
    bool dpadRight = GetAsyncKeyState(g_keyMapping.dpad_right) & 0x8000;

    g_leftController.SetButton(emu::BTN_DPAD_UP, dpadUp);
    g_leftController.SetButton(emu::BTN_DPAD_DOWN, dpadDown);
    g_leftController.SetButton(emu::BTN_DPAD_LEFT, dpadLeft);
    g_leftController.SetButton(emu::BTN_DPAD_RIGHT, dpadRight);

    // System buttons
    g_leftController.SetButton(emu::BTN_START, GetAsyncKeyState(g_keyMapping.system_button) & 0x8000);
    g_leftController.SetButton(emu::BTN_BACK, GetAsyncKeyState(g_keyMapping.menu_button) & 0x8000);
}

// Update HMD pose from keyboard/mouse
void UpdateHMDInput(HWND hwnd, float deltaTime) {
    // Toggle mouse look with right mouse button
    bool rightButtonDown = (GetAsyncKeyState(g_hmdKeyMapping.mouse_look_toggle) & 0x8000) != 0;
    if (rightButtonDown && !g_wasRightButtonDown) {
        g_mouseLookEnabled = !g_mouseLookEnabled;
        if (g_mouseLookEnabled) {
            // Store current mouse position when enabling
            GetCursorPos(&g_lastMousePos);
            // Hide cursor
            ShowCursor(FALSE);
        } else {
            // Show cursor when disabling
            ShowCursor(TRUE);
        }
    }
    g_wasRightButtonDown = rightButtonDown;

    // Mouse look (when enabled)
    if (g_mouseLookEnabled) {
        POINT currentPos;
        GetCursorPos(&currentPos);

        int deltaX = currentPos.x - g_lastMousePos.x;
        int deltaY = currentPos.y - g_lastMousePos.y;

        if (deltaX != 0 || deltaY != 0) {
            // Update rotation
            g_hmdPose.yaw -= deltaX * g_hmdSettings.mouse_sensitivity;
            g_hmdPose.pitch -= deltaY * g_hmdSettings.mouse_sensitivity;

            // Clamp pitch to prevent flipping
            if (g_hmdPose.pitch < g_hmdSettings.min_pitch)
                g_hmdPose.pitch = g_hmdSettings.min_pitch;
            if (g_hmdPose.pitch > g_hmdSettings.max_pitch)
                g_hmdPose.pitch = g_hmdSettings.max_pitch;

            // Keep yaw in [-PI, PI] range
            while (g_hmdPose.yaw > 3.14159f) g_hmdPose.yaw -= 6.28318f;
            while (g_hmdPose.yaw < -3.14159f) g_hmdPose.yaw += 6.28318f;

            // Reset cursor to center to allow continuous rotation
            SetCursorPos(g_lastMousePos.x, g_lastMousePos.y);
        }
    }

    // Keyboard movement
    float moveSpeed = g_hmdSettings.move_speed * deltaTime;

    // Speed modifiers
    if (GetAsyncKeyState(g_hmdKeyMapping.speed_fast) & 0x8000)
        moveSpeed *= g_hmdSettings.fast_multiplier;
    if (GetAsyncKeyState(g_hmdKeyMapping.speed_slow) & 0x8000)
        moveSpeed *= g_hmdSettings.slow_multiplier;

    // Calculate movement direction based on yaw
    float sinYaw = sinf(g_hmdPose.yaw);
    float cosYaw = cosf(g_hmdPose.yaw);

    // Forward/backward (Z axis in OpenVR)
    if (GetAsyncKeyState(g_hmdKeyMapping.move_forward) & 0x8000) {
        g_hmdPose.position[0] -= sinYaw * moveSpeed;
        g_hmdPose.position[2] -= cosYaw * moveSpeed;
    }
    if (GetAsyncKeyState(g_hmdKeyMapping.move_backward) & 0x8000) {
        g_hmdPose.position[0] += sinYaw * moveSpeed;
        g_hmdPose.position[2] += cosYaw * moveSpeed;
    }

    // Left/right strafe (X axis)
    if (GetAsyncKeyState(g_hmdKeyMapping.move_left) & 0x8000) {
        g_hmdPose.position[0] -= cosYaw * moveSpeed;
        g_hmdPose.position[2] += sinYaw * moveSpeed;
    }
    if (GetAsyncKeyState(g_hmdKeyMapping.move_right) & 0x8000) {
        g_hmdPose.position[0] += cosYaw * moveSpeed;
        g_hmdPose.position[2] -= sinYaw * moveSpeed;
    }

    // Up/down (Y axis)
    if (GetAsyncKeyState(g_hmdKeyMapping.move_up) & 0x8000)
        g_hmdPose.position[1] += moveSpeed;
    if (GetAsyncKeyState(g_hmdKeyMapping.move_down) & 0x8000)
        g_hmdPose.position[1] -= moveSpeed;

    // Reset pose
    if (GetAsyncKeyState(g_hmdKeyMapping.reset_pose) & 0x8000) {
        g_hmdPose.Reset();
    }
}

// Draw a button indicator
void DrawButton(const char* label, bool pressed, float width = 40.0f) {
    ImVec4 color = pressed ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    ImGui::Button(label, ImVec2(width, 30));
    ImGui::PopStyleColor(3);
}

// Draw joystick visualization
void DrawJoystick(const char* label, float x, float y, bool clicked) {
    ImGui::Text("%s", label);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float radius = 40.0f;
    float inner_radius = 8.0f;

    // Background circle
    draw_list->AddCircleFilled(ImVec2(p.x + radius, p.y + radius), radius,
        IM_COL32(50, 50, 50, 255));
    draw_list->AddCircle(ImVec2(p.x + radius, p.y + radius), radius,
        IM_COL32(100, 100, 100, 255));

    // Stick position
    float stick_x = p.x + radius + x * (radius - inner_radius);
    float stick_y = p.y + radius - y * (radius - inner_radius); // Invert Y for display
    ImU32 stick_color = clicked ? IM_COL32(50, 200, 50, 255) : IM_COL32(150, 150, 150, 255);
    draw_list->AddCircleFilled(ImVec2(stick_x, stick_y), inner_radius, stick_color);

    ImGui::Dummy(ImVec2(radius * 2, radius * 2));
    ImGui::Text("X:%.2f Y:%.2f", x, y);
}

// Draw trigger/grip bar
void DrawAnalogBar(const char* label, float value) {
    ImGui::Text("%s", label);
    ImGui::ProgressBar(value, ImVec2(100, 20));
}

// Draw D-Pad
void DrawDPad(bool up, bool down, bool left, bool right) {
    ImGui::Text("D-Pad");
    ImGui::Dummy(ImVec2(30, 0)); ImGui::SameLine();
    DrawButton("^", up, 30); ImGui::SameLine();
    ImGui::Dummy(ImVec2(30, 0));

    DrawButton("<", left, 30); ImGui::SameLine();
    ImGui::Dummy(ImVec2(30, 0)); ImGui::SameLine();
    DrawButton(">", right, 30);

    ImGui::Dummy(ImVec2(30, 0)); ImGui::SameLine();
    DrawButton("v", down, 30);
}

// Draw controller panel
void DrawControllerPanel(const char* name, const emu::ControllerInputState& state, bool isLeft) {
    ImGui::BeginChild(name, ImVec2(250, 400), true);
    ImGui::Text("%s", name);
    ImGui::Separator();

    // Joystick
    DrawJoystick("Stick", state.stick_x, state.stick_y,
        state.IsButtonPressed(isLeft ? emu::BTN_L3 : emu::BTN_R3));
    ImGui::Separator();

    // Face buttons
    ImGui::Text("Buttons");
    DrawButton("Y", state.IsButtonPressed(emu::BTN_Y)); ImGui::SameLine();
    DrawButton("X", state.IsButtonPressed(emu::BTN_X));
    DrawButton("B", state.IsButtonPressed(emu::BTN_B)); ImGui::SameLine();
    DrawButton("A", state.IsButtonPressed(emu::BTN_A));
    ImGui::Separator();

    // Bumper
    DrawButton(isLeft ? "LB" : "RB",
        state.IsButtonPressed(isLeft ? emu::BTN_LB : emu::BTN_RB), 60);
    ImGui::Separator();

    // Trigger and Grip
    DrawAnalogBar("Trigger", state.trigger);
    DrawAnalogBar("Grip", state.grip);
    ImGui::Separator();

    // D-Pad (left controller only)
    if (isLeft) {
        DrawDPad(
            state.IsButtonPressed(emu::BTN_DPAD_UP),
            state.IsButtonPressed(emu::BTN_DPAD_DOWN),
            state.IsButtonPressed(emu::BTN_DPAD_LEFT),
            state.IsButtonPressed(emu::BTN_DPAD_RIGHT)
        );
        ImGui::Separator();

        // System buttons
        ImGui::Text("System");
        DrawButton("Start", state.IsButtonPressed(emu::BTN_START), 60); ImGui::SameLine();
        DrawButton("Back", state.IsButtonPressed(emu::BTN_BACK), 60);
    }

    ImGui::EndChild();
}

// Draw HMD control panel
void DrawHMDPanel() {
    ImGui::BeginChild("HMD", ImVec2(520, 280), true);
    ImGui::Text("HMD Emulator");
    ImGui::Separator();

    // Mouse look status
    if (g_mouseLookEnabled) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Mouse Look: ENABLED (Right-click to toggle)");
    } else {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Mouse Look: DISABLED (Right-click to toggle)");
    }
    ImGui::Separator();

    // Position display
    ImGui::Text("Position (meters):");
    ImGui::Text("  X: %+.3f  Y: %+.3f  Z: %+.3f",
        g_hmdPose.position[0], g_hmdPose.position[1], g_hmdPose.position[2]);

    // Rotation display (convert to degrees for readability)
    float yawDeg = g_hmdPose.yaw * 57.2958f;
    float pitchDeg = g_hmdPose.pitch * 57.2958f;
    float rollDeg = g_hmdPose.roll * 57.2958f;
    ImGui::Text("Rotation (degrees):");
    ImGui::Text("  Yaw: %+.1f  Pitch: %+.1f  Roll: %+.1f", yawDeg, pitchDeg, rollDeg);
    ImGui::Separator();

    // Visual representation - top-down view
    ImGui::Text("Top-Down View:");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float viewSize = 120.0f;
    float centerX = p.x + viewSize;
    float centerY = p.y + viewSize / 2;

    // Background
    draw_list->AddRectFilled(
        ImVec2(p.x, p.y),
        ImVec2(p.x + viewSize * 2, p.y + viewSize),
        IM_COL32(30, 30, 40, 255));

    // Grid
    draw_list->AddLine(ImVec2(centerX, p.y), ImVec2(centerX, p.y + viewSize), IM_COL32(60, 60, 60, 255));
    draw_list->AddLine(ImVec2(p.x, centerY), ImVec2(p.x + viewSize * 2, centerY), IM_COL32(60, 60, 60, 255));

    // Scale: 1 meter = 20 pixels
    float scale = 20.0f;
    float hmdX = centerX + g_hmdPose.position[0] * scale;
    float hmdY = centerY - g_hmdPose.position[2] * scale;  // Z is forward, Y-up in screen

    // Clamp to view bounds
    hmdX = fmaxf(p.x + 10, fminf(p.x + viewSize * 2 - 10, hmdX));
    hmdY = fmaxf(p.y + 10, fminf(p.y + viewSize - 10, hmdY));

    // Draw HMD as triangle pointing in view direction
    float triSize = 12.0f;
    float dirX = -sinf(g_hmdPose.yaw);
    float dirY = -cosf(g_hmdPose.yaw);

    ImVec2 tip(hmdX + dirX * triSize, hmdY + dirY * triSize);
    ImVec2 left(hmdX + dirY * triSize * 0.5f, hmdY - dirX * triSize * 0.5f);
    ImVec2 right(hmdX - dirY * triSize * 0.5f, hmdY + dirX * triSize * 0.5f);

    draw_list->AddTriangleFilled(tip, left, right, IM_COL32(100, 200, 100, 255));
    draw_list->AddTriangle(tip, left, right, IM_COL32(150, 255, 150, 255));

    // Origin marker
    draw_list->AddCircle(ImVec2(centerX, centerY), 4, IM_COL32(100, 100, 200, 255));

    ImGui::Dummy(ImVec2(viewSize * 2, viewSize));

    // Side view (pitch visualization)
    ImGui::SameLine();
    ImGui::Text("  Side View:");
    ImVec2 p2 = ImGui::GetCursorScreenPos();
    p2.x += 10;

    // Background
    draw_list->AddRectFilled(
        ImVec2(p2.x, p.y),
        ImVec2(p2.x + 100, p.y + viewSize),
        IM_COL32(30, 30, 40, 255));

    // Ground line
    float groundY = p.y + viewSize - 20;
    draw_list->AddLine(ImVec2(p2.x, groundY), ImVec2(p2.x + 100, groundY), IM_COL32(80, 60, 40, 255), 2.0f);

    // Height indicator
    float heightY = groundY - g_hmdPose.position[1] * scale;
    heightY = fmaxf(p.y + 10, fminf(groundY - 5, heightY));
    float sideX = p2.x + 50;

    // Draw head circle with pitch direction
    draw_list->AddCircleFilled(ImVec2(sideX, heightY), 8, IM_COL32(100, 200, 100, 255));
    float lookDirX = cosf(g_hmdPose.pitch) * 15;
    float lookDirY = -sinf(g_hmdPose.pitch) * 15;
    draw_list->AddLine(
        ImVec2(sideX, heightY),
        ImVec2(sideX + lookDirX, heightY + lookDirY),
        IM_COL32(255, 255, 100, 255), 2.0f);

    ImGui::Separator();

    // Key mapping info
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD: Move | Q/E: Down/Up | R: Reset");
    ImGui::BulletText("Shift: Fast | Ctrl: Slow");
    ImGui::BulletText("Right-Click: Toggle Mouse Look");

    ImGui::EndChild();
}

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance,
        nullptr, nullptr, nullptr, nullptr, L"EmuController", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"EmuController - OpenVR Emulator",
        WS_OVERLAPPEDWINDOW, 100, 100, 560, 750, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.15f, 1.0f);

    // Start IPC connection thread
    g_ipcClient.StartConnectionThread();

    // Delta time tracking
    LARGE_INTEGER frequency, lastTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);

    // Main loop
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Calculate delta time
        QueryPerformanceCounter(&currentTime);
        float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) /
                          static_cast<float>(frequency.QuadPart);
        lastTime = currentTime;

        // Update input state
        UpdateInput(hwnd);
        UpdateHMDInput(hwnd, deltaTime);

        // Send input to driver via IPC
        if (g_ipcClient.IsConnected()) {
            g_ipcClient.SendInputState(g_leftController, g_rightController);
            // TODO: Send HMD pose when IPC is extended
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Main window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("EmuController", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // Header
        ImGui::Text("EmuController v1.1 - OpenVR Emulator (HMD + Controllers)");
        ImGui::Separator();

        // Connection status
        if (g_ipcClient.IsConnected()) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Status: Connected to SteamVR Driver");
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "Status: Waiting for SteamVR Driver...");
        }
        ImGui::Separator();

        // HMD panel
        DrawHMDPanel();
        ImGui::Separator();

        // Controller panels side by side
        DrawControllerPanel("Left Controller", g_leftController, true);
        ImGui::SameLine();
        DrawControllerPanel("Right Controller", g_rightController, false);

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w, clear_color.y * clear_color.w,
            clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    if (g_mouseLookEnabled) {
        ShowCursor(TRUE);  // Restore cursor visibility
    }

    g_ipcClient.StopConnectionThread();
    g_ipcClient.Disconnect();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// DirectX 11 helper functions
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
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
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

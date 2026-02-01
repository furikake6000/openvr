#pragma once

#include <cstdint>

namespace emu {

// Button bit flags
enum ButtonFlags : uint16_t {
    BTN_A      = 1 << 0,
    BTN_B      = 1 << 1,
    BTN_X      = 1 << 2,
    BTN_Y      = 1 << 3,
    BTN_LB     = 1 << 4,   // Left Bumper
    BTN_RB     = 1 << 5,   // Right Bumper
    BTN_BACK   = 1 << 6,
    BTN_START  = 1 << 7,
    BTN_L3     = 1 << 8,   // Left Stick Click
    BTN_R3     = 1 << 9,   // Right Stick Click
    BTN_DPAD_UP    = 1 << 10,
    BTN_DPAD_DOWN  = 1 << 11,
    BTN_DPAD_LEFT  = 1 << 12,
    BTN_DPAD_RIGHT = 1 << 13,
};

// Controller input state (single controller)
struct ControllerInputState {
    uint16_t buttons = 0;       // Button bit field

    float stick_x = 0.0f;       // Joystick X (-1.0 to +1.0)
    float stick_y = 0.0f;       // Joystick Y (-1.0 to +1.0)

    float trigger = 0.0f;       // Trigger (0.0 to 1.0)
    float grip = 0.0f;          // Grip (0.0 to 1.0)

    // Helper methods
    bool IsButtonPressed(ButtonFlags btn) const { return (buttons & btn) != 0; }
    void SetButton(ButtonFlags btn, bool pressed) {
        if (pressed) buttons |= btn;
        else buttons &= ~btn;
    }
};

// Default key mappings (Virtual Key codes)
struct KeyMapping {
    // Left controller - WASD
    int left_stick_up    = 'W';
    int left_stick_down  = 'S';
    int left_stick_left  = 'A';
    int left_stick_right = 'D';
    int left_trigger     = 0x01;  // VK_LBUTTON (Left Click)
    int left_grip        = 0xA0;  // VK_LSHIFT (Left Shift)
    int left_a           = 'Q';
    int left_b           = 'E';
    int left_x           = '1';
    int left_y           = '2';
    int left_stick_click = 0x20;  // VK_SPACE
    int left_bumper      = 'R';

    // Right controller - Arrow Keys
    int right_stick_up   = 0x26;  // VK_UP
    int right_stick_down = 0x28;  // VK_DOWN
    int right_stick_left = 0x25;  // VK_LEFT
    int right_stick_right= 0x27;  // VK_RIGHT
    int right_trigger    = 0x02;  // VK_RBUTTON (Right Click)
    int right_grip       = 0xA1;  // VK_RSHIFT (Right Shift)
    int right_a          = 'J';
    int right_b          = 'K';
    int right_x          = 'U';
    int right_y          = 'I';
    int right_stick_click = 'L';
    int right_bumper     = 'O';

    // D-Pad (numpad)
    int dpad_up          = 0x68;  // VK_NUMPAD8
    int dpad_down        = 0x62;  // VK_NUMPAD2
    int dpad_left        = 0x64;  // VK_NUMPAD4
    int dpad_right       = 0x66;  // VK_NUMPAD6

    // System
    int system_button    = 0x1B;  // VK_ESCAPE
    int menu_button      = 0x09;  // VK_TAB
};

} // namespace emu

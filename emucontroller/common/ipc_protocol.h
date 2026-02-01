#pragma once

#include <cstdint>

namespace emu {

// Named Pipe name
constexpr const char* PIPE_NAME = "\\\\.\\pipe\\EmuControllerPipe";

// Message types
enum MessageType : uint16_t {
    MSG_INPUT_STATE     = 0x0001,  // Input state update (GUI -> Driver)
    MSG_CONNECT         = 0x0002,  // Connection request (GUI -> Driver)
    MSG_DISCONNECT      = 0x0003,  // Disconnection notification
    MSG_STATUS_REQUEST  = 0x0010,  // Status request (GUI -> Driver)
    MSG_STATUS_RESPONSE = 0x0011,  // Status response (Driver -> GUI)
};

// Message header
#pragma pack(push, 1)
struct MessageHeader {
    uint16_t type;    // MessageType
    uint16_t length;  // Payload length (excluding header)
};

// Input state for a single controller (sent from GUI to Driver)
struct ControllerInputPacket {
    uint16_t buttons;       // Button bit field (see ButtonFlags in input_state.h)

    int16_t stick_x;        // Joystick X (-32768 to +32767, maps to -1.0 to +1.0)
    int16_t stick_y;        // Joystick Y (-32768 to +32767, maps to -1.0 to +1.0)

    uint8_t trigger;        // Trigger value (0-255, maps to 0.0 to 1.0)
    uint8_t grip;           // Grip value (0-255, maps to 0.0 to 1.0)

    uint8_t reserved[2];    // Padding for alignment
};

// Full input state message (both controllers)
struct InputStateMessage {
    MessageHeader header;
    ControllerInputPacket left;
    ControllerInputPacket right;
    uint64_t timestamp;     // Timestamp in milliseconds
};

// Status response from driver
struct StatusResponse {
    MessageHeader header;
    uint8_t driver_active;      // 1 if driver is active
    uint8_t left_connected;     // 1 if left controller is connected
    uint8_t right_connected;    // 1 if right controller is connected
    uint8_t reserved;
};
#pragma pack(pop)

// Helper functions for conversion
inline float StickToFloat(int16_t value) {
    return static_cast<float>(value) / 32767.0f;
}

inline int16_t FloatToStick(float value) {
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    return static_cast<int16_t>(value * 32767.0f);
}

inline float TriggerToFloat(uint8_t value) {
    return static_cast<float>(value) / 255.0f;
}

inline uint8_t FloatToTrigger(float value) {
    if (value > 1.0f) value = 1.0f;
    if (value < 0.0f) value = 0.0f;
    return static_cast<uint8_t>(value * 255.0f);
}

} // namespace emu

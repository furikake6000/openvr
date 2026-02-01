#pragma once

#include <openvr_driver.h>
#include <string>
#include <thread>
#include <atomic>
#include <array>
#include <mutex>
#include "input_state.h"

namespace emu {

// Input component indices
enum EmuInputComponent {
    Component_A_Click,
    Component_A_Touch,
    Component_B_Click,
    Component_B_Touch,
    Component_X_Click,
    Component_X_Touch,
    Component_Y_Click,
    Component_Y_Touch,
    Component_Trigger_Value,
    Component_Trigger_Click,
    Component_Grip_Value,
    Component_Grip_Click,
    Component_Joystick_X,
    Component_Joystick_Y,
    Component_Joystick_Click,
    Component_Joystick_Touch,
    Component_Bumper_Click,
    Component_DPad_Up,
    Component_DPad_Down,
    Component_DPad_Left,
    Component_DPad_Right,
    Component_System_Click,
    Component_Menu_Click,
    Component_Haptic,
    Component_MAX
};

class EmuControllerDriver : public vr::ITrackedDeviceServerDriver {
public:
    EmuControllerDriver(vr::ETrackedControllerRole role);

    // ITrackedDeviceServerDriver interface
    vr::EVRInitError Activate(uint32_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override;
    void* GetComponent(const char* pchComponentNameAndVersion) override;
    void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
    vr::DriverPose_t GetPose() override;

    // Custom methods
    const std::string& GetSerialNumber() const { return serial_number_; }
    void UpdateInputState(const ControllerInputState& state);
    void RunFrame();
    void ProcessEvent(const vr::VREvent_t& event);

private:
    void PoseUpdateThread();

    std::atomic<bool> is_active_{false};
    vr::TrackedDeviceIndex_t device_index_ = vr::k_unTrackedDeviceIndexInvalid;
    vr::ETrackedControllerRole role_;

    std::string serial_number_;
    std::string model_number_ = "EmuController";

    std::array<vr::VRInputComponentHandle_t, Component_MAX> input_handles_{};

    std::mutex input_mutex_;
    ControllerInputState current_state_;

    std::thread pose_thread_;
};

} // namespace emu

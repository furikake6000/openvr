#include "controller_device_driver.h"
#include "driverlog.h"
#include "vrmath.h"

namespace emu {

EmuControllerDriver::EmuControllerDriver(vr::ETrackedControllerRole role)
    : role_(role) {
    // Generate serial number based on role
    serial_number_ = (role == vr::TrackedControllerRole_LeftHand)
        ? "EMU_LEFT_CONTROLLER"
        : "EMU_RIGHT_CONTROLLER";

    DriverLog("EmuControllerDriver created: %s\n", serial_number_.c_str());
}

vr::EVRInitError EmuControllerDriver::Activate(uint32_t unObjectId) {
    is_active_ = true;
    device_index_ = unObjectId;

    vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(device_index_);

    // Set device properties
    vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, model_number_.c_str());
    vr::VRProperties()->SetStringProperty(container, vr::Prop_SerialNumber_String, serial_number_.c_str());
    vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32, role_);

    // Set input profile path
    vr::VRProperties()->SetStringProperty(container, vr::Prop_InputProfilePath_String,
        "{emucontroller}/input/emucontroller_profile.json");

    // Create input components
    // Face buttons
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/a/click", &input_handles_[Component_A_Click]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/a/touch", &input_handles_[Component_A_Touch]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/b/click", &input_handles_[Component_B_Click]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/b/touch", &input_handles_[Component_B_Touch]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/x/click", &input_handles_[Component_X_Click]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/x/touch", &input_handles_[Component_X_Touch]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/y/click", &input_handles_[Component_Y_Click]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/y/touch", &input_handles_[Component_Y_Touch]);

    // Trigger
    vr::VRDriverInput()->CreateScalarComponent(container, "/input/trigger/value", &input_handles_[Component_Trigger_Value],
        vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/trigger/click", &input_handles_[Component_Trigger_Click]);

    // Grip
    vr::VRDriverInput()->CreateScalarComponent(container, "/input/grip/value", &input_handles_[Component_Grip_Value],
        vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/grip/click", &input_handles_[Component_Grip_Click]);

    // Joystick
    vr::VRDriverInput()->CreateScalarComponent(container, "/input/joystick/x", &input_handles_[Component_Joystick_X],
        vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent(container, "/input/joystick/y", &input_handles_[Component_Joystick_Y],
        vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/joystick/click", &input_handles_[Component_Joystick_Click]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/joystick/touch", &input_handles_[Component_Joystick_Touch]);

    // Bumper
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/bumper/click", &input_handles_[Component_Bumper_Click]);

    // D-Pad (for left controller)
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/dpad/up", &input_handles_[Component_DPad_Up]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/dpad/down", &input_handles_[Component_DPad_Down]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/dpad/left", &input_handles_[Component_DPad_Left]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/dpad/right", &input_handles_[Component_DPad_Right]);

    // System buttons
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/system/click", &input_handles_[Component_System_Click]);
    vr::VRDriverInput()->CreateBooleanComponent(container, "/input/application_menu/click", &input_handles_[Component_Menu_Click]);

    // Haptic output
    vr::VRDriverInput()->CreateHapticComponent(container, "/output/haptic", &input_handles_[Component_Haptic]);

    // Start pose update thread
    pose_thread_ = std::thread(&EmuControllerDriver::PoseUpdateThread, this);

    DriverLog("EmuControllerDriver activated: %s (index: %d)\n", serial_number_.c_str(), device_index_);
    return vr::VRInitError_None;
}

void EmuControllerDriver::Deactivate() {
    if (is_active_.exchange(false)) {
        pose_thread_.join();
    }
    device_index_ = vr::k_unTrackedDeviceIndexInvalid;
    DriverLog("EmuControllerDriver deactivated: %s\n", serial_number_.c_str());
}

void EmuControllerDriver::EnterStandby() {
    DriverLog("EmuControllerDriver enter standby: %s\n", serial_number_.c_str());
}

void* EmuControllerDriver::GetComponent(const char* pchComponentNameAndVersion) {
    return nullptr;
}

void EmuControllerDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
    if (unResponseBufferSize >= 1)
        pchResponseBuffer[0] = 0;
}

vr::DriverPose_t EmuControllerDriver::GetPose() {
    vr::DriverPose_t pose = {};
    pose.qWorldFromDriverRotation.w = 1.0;
    pose.qDriverFromHeadRotation.w = 1.0;

    // Get HMD pose
    vr::TrackedDevicePose_t hmd_pose{};
    vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0.f, &hmd_pose, 1);

    if (hmd_pose.bPoseIsValid) {
        const vr::HmdVector3_t hmd_position = HmdVector3_From34Matrix(hmd_pose.mDeviceToAbsoluteTracking);
        const vr::HmdQuaternion_t hmd_orientation = HmdQuaternion_FromMatrix(hmd_pose.mDeviceToAbsoluteTracking);

        // Pitch controller 90 degrees
        const vr::HmdQuaternion_t offset_orientation = HmdQuaternion_FromEulerAngles(0.f, static_cast<float>(DEG_TO_RAD(90.0)), 0.f);
        pose.qRotation = hmd_orientation * offset_orientation;

        // Position offset based on hand
        const vr::HmdVector3_t offset_position = {
            role_ == vr::TrackedControllerRole_LeftHand ? -0.2f : 0.2f,
            -0.1f,
            -0.4f
        };

        const vr::HmdVector3_t position = hmd_position + (offset_position * hmd_orientation);
        pose.vecPosition[0] = position.v[0];
        pose.vecPosition[1] = position.v[1];
        pose.vecPosition[2] = position.v[2];
    } else {
        // Default position if HMD not available
        pose.qRotation.w = 1.0;
        pose.vecPosition[0] = role_ == vr::TrackedControllerRole_LeftHand ? -0.2 : 0.2;
        pose.vecPosition[1] = 1.0;
        pose.vecPosition[2] = -0.5;
    }

    pose.poseIsValid = true;
    pose.deviceIsConnected = true;
    pose.result = vr::TrackingResult_Running_OK;

    return pose;
}

void EmuControllerDriver::PoseUpdateThread() {
    while (is_active_.load()) {
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(device_index_, GetPose(), sizeof(vr::DriverPose_t));
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void EmuControllerDriver::UpdateInputState(const ControllerInputState& state) {
    std::lock_guard<std::mutex> lock(input_mutex_);
    current_state_ = state;
}

void EmuControllerDriver::RunFrame() {
    ControllerInputState state;
    {
        std::lock_guard<std::mutex> lock(input_mutex_);
        state = current_state_;
    }

    // Update boolean components (buttons)
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_A_Click], state.IsButtonPressed(BTN_A), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_A_Touch], state.IsButtonPressed(BTN_A), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_B_Click], state.IsButtonPressed(BTN_B), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_B_Touch], state.IsButtonPressed(BTN_B), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_X_Click], state.IsButtonPressed(BTN_X), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_X_Touch], state.IsButtonPressed(BTN_X), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_Y_Click], state.IsButtonPressed(BTN_Y), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_Y_Touch], state.IsButtonPressed(BTN_Y), 0);

    // Trigger
    vr::VRDriverInput()->UpdateScalarComponent(input_handles_[Component_Trigger_Value], state.trigger, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_Trigger_Click], state.trigger > 0.9f, 0);

    // Grip
    vr::VRDriverInput()->UpdateScalarComponent(input_handles_[Component_Grip_Value], state.grip, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_Grip_Click], state.grip > 0.9f, 0);

    // Joystick
    vr::VRDriverInput()->UpdateScalarComponent(input_handles_[Component_Joystick_X], state.stick_x, 0);
    vr::VRDriverInput()->UpdateScalarComponent(input_handles_[Component_Joystick_Y], state.stick_y, 0);
    bool stick_pressed = (role_ == vr::TrackedControllerRole_LeftHand)
        ? state.IsButtonPressed(BTN_L3)
        : state.IsButtonPressed(BTN_R3);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_Joystick_Click], stick_pressed, 0);
    bool stick_touched = (state.stick_x != 0.0f || state.stick_y != 0.0f || stick_pressed);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_Joystick_Touch], stick_touched, 0);

    // Bumper
    bool bumper_pressed = (role_ == vr::TrackedControllerRole_LeftHand)
        ? state.IsButtonPressed(BTN_LB)
        : state.IsButtonPressed(BTN_RB);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_Bumper_Click], bumper_pressed, 0);

    // D-Pad
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_DPad_Up], state.IsButtonPressed(BTN_DPAD_UP), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_DPad_Down], state.IsButtonPressed(BTN_DPAD_DOWN), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_DPad_Left], state.IsButtonPressed(BTN_DPAD_LEFT), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_DPad_Right], state.IsButtonPressed(BTN_DPAD_RIGHT), 0);

    // System buttons
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_System_Click], state.IsButtonPressed(BTN_START), 0);
    vr::VRDriverInput()->UpdateBooleanComponent(input_handles_[Component_Menu_Click], state.IsButtonPressed(BTN_BACK), 0);
}

void EmuControllerDriver::ProcessEvent(const vr::VREvent_t& event) {
    if (event.eventType == vr::VREvent_Input_HapticVibration) {
        if (event.data.hapticVibration.componentHandle == input_handles_[Component_Haptic]) {
            // Log haptic event (could be sent to GUI in the future)
            DriverLog("Haptic: %s hand - duration: %.2f, freq: %.2f, amp: %.2f\n",
                role_ == vr::TrackedControllerRole_LeftHand ? "Left" : "Right",
                event.data.hapticVibration.fDurationSeconds,
                event.data.hapticVibration.fFrequency,
                event.data.hapticVibration.fAmplitude);
        }
    }
}

} // namespace emu

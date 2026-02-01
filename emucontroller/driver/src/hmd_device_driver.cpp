#include "hmd_device_driver.h"
#include "driverlog.h"
#include <cstring>

namespace emu {

EmuHMDDriver::EmuHMDDriver() {
    // Load display configuration from settings
    display_config_.window_x = vr::VRSettings()->GetInt32("emucontroller_hmd", "window_x");
    display_config_.window_y = vr::VRSettings()->GetInt32("emucontroller_hmd", "window_y");
    display_config_.window_width = vr::VRSettings()->GetInt32("emucontroller_hmd", "window_width");
    display_config_.window_height = vr::VRSettings()->GetInt32("emucontroller_hmd", "window_height");
    display_config_.render_width = vr::VRSettings()->GetInt32("emucontroller_hmd", "render_width");
    display_config_.render_height = vr::VRSettings()->GetInt32("emucontroller_hmd", "render_height");

    // Use defaults if settings not found (0 means not set)
    if (display_config_.window_width == 0) display_config_.window_width = 1920;
    if (display_config_.window_height == 0) display_config_.window_height = 1080;
    if (display_config_.render_width == 0) display_config_.render_width = 1512;
    if (display_config_.render_height == 0) display_config_.render_height = 1680;

    // Create display component
    display_component_ = std::make_unique<EmuHMDDisplayComponent>(display_config_);

    DriverLog("EmuHMD: Created with window %dx%d at (%d,%d), render %dx%d",
        display_config_.window_width, display_config_.window_height,
        display_config_.window_x, display_config_.window_y,
        display_config_.render_width, display_config_.render_height);
}

vr::EVRInitError EmuHMDDriver::Activate(uint32_t unObjectId) {
    device_index_ = unObjectId;
    is_active_ = true;

    vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(device_index_);

    // Basic properties
    vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, model_number_.c_str());
    vr::VRProperties()->SetStringProperty(container, vr::Prop_SerialNumber_String, serial_number_.c_str());
    vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String, "EmuController");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_TrackingSystemName_String, "emucontroller");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_RenderModelName_String, "generic_hmd");

    // Display properties
    vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserIpdMeters_Float,
        vr::VRSettings()->GetFloat(vr::k_pch_SteamVR_Section, vr::k_pch_SteamVR_IPD_Float));
    vr::VRProperties()->SetFloatProperty(container, vr::Prop_DisplayFrequency_Float, 60.0f);
    vr::VRProperties()->SetFloatProperty(container, vr::Prop_SecondsFromVsyncToPhotons_Float, 0.011f);
    vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserHeadToEyeDepthMeters_Float, 0.0f);

    // Avoid "not fullscreen" warnings
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_IsOnDesktop_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_DisplayDebugMode_Bool, true);

    // Will submit poses (not head pose provider)
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_WillDriftInYaw_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_DeviceIsWireless_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_DeviceIsCharging_Bool, false);
    vr::VRProperties()->SetFloatProperty(container, vr::Prop_DeviceBatteryPercentage_Float, 1.0f);

    // Start pose update thread
    pose_thread_ = std::thread(&EmuHMDDriver::PoseUpdateThread, this);

    DriverLog("EmuHMD: Activated with device index %d", device_index_);
    return vr::VRInitError_None;
}

void EmuHMDDriver::Deactivate() {
    if (is_active_.exchange(false)) {
        if (pose_thread_.joinable()) {
            pose_thread_.join();
        }
    }
    device_index_ = vr::k_unTrackedDeviceIndexInvalid;
    DriverLog("EmuHMD: Deactivated");
}

void EmuHMDDriver::EnterStandby() {
    DriverLog("EmuHMD: Entering standby");
}

void* EmuHMDDriver::GetComponent(const char* pchComponentNameAndVersion) {
    if (strcmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version) == 0) {
        return display_component_.get();
    }
    return nullptr;
}

void EmuHMDDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
    if (unResponseBufferSize > 0) {
        pchResponseBuffer[0] = '\0';
    }
}

vr::DriverPose_t EmuHMDDriver::GetPose() {
    vr::DriverPose_t pose = {};

    // Initialize quaternions to identity
    pose.qWorldFromDriverRotation.w = 1.0f;
    pose.qWorldFromDriverRotation.x = 0.0f;
    pose.qWorldFromDriverRotation.y = 0.0f;
    pose.qWorldFromDriverRotation.z = 0.0f;

    pose.qDriverFromHeadRotation.w = 1.0f;
    pose.qDriverFromHeadRotation.x = 0.0f;
    pose.qDriverFromHeadRotation.y = 0.0f;
    pose.qDriverFromHeadRotation.z = 0.0f;

    {
        std::lock_guard<std::mutex> lock(pose_mutex_);
        // Position
        pose.vecPosition[0] = current_pose_.position[0];
        pose.vecPosition[1] = current_pose_.position[1];
        pose.vecPosition[2] = current_pose_.position[2];

        // Rotation (quaternion from IPC)
        pose.qRotation.w = current_pose_.quaternion[0];
        pose.qRotation.x = current_pose_.quaternion[1];
        pose.qRotation.y = current_pose_.quaternion[2];
        pose.qRotation.z = current_pose_.quaternion[3];
    }

    pose.poseIsValid = true;
    pose.deviceIsConnected = true;
    pose.result = vr::TrackingResult_Running_OK;
    pose.shouldApplyHeadModel = false;  // We provide the full pose

    return pose;
}

void EmuHMDDriver::UpdatePose(const HMDPoseData& pose) {
    std::lock_guard<std::mutex> lock(pose_mutex_);
    current_pose_ = pose;
}

void EmuHMDDriver::RunFrame() {
    // Called each frame, can be used for per-frame updates
}

void EmuHMDDriver::ProcessEvent(const vr::VREvent_t& event) {
    (void)event;
}

void EmuHMDDriver::PoseUpdateThread() {
    while (is_active_) {
        if (device_index_ != vr::k_unTrackedDeviceIndexInvalid) {
            vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
                device_index_, GetPose(), sizeof(vr::DriverPose_t));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

} // namespace emu

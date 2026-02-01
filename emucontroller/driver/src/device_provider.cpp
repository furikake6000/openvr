#include "device_provider.h"
#include "driverlog.h"

namespace emu {

vr::EVRInitError EmuDeviceProvider::Init(vr::IVRDriverContext* pDriverContext) {
    VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

    InitDriverLog(vr::VRDriverLog());
    DriverLog("EmuController Driver initializing...\n");

    // Start IPC server
    ipc_server_ = std::make_unique<IPCServer>();
    if (!ipc_server_->Start()) {
        DriverLog("Failed to start IPC server!\n");
        return vr::VRInitError_Driver_Failed;
    }

    // Create HMD
    hmd_ = std::make_unique<EmuHMDDriver>();
    if (!vr::VRServerDriverHost()->TrackedDeviceAdded(
            hmd_->GetSerialNumber().c_str(),
            vr::TrackedDeviceClass_HMD,
            hmd_.get())) {
        DriverLog("Failed to add HMD!\n");
        return vr::VRInitError_Driver_Unknown;
    }
    DriverLog("HMD added successfully\n");

    // Create left controller
    left_controller_ = std::make_unique<EmuControllerDriver>(vr::TrackedControllerRole_LeftHand);
    if (!vr::VRServerDriverHost()->TrackedDeviceAdded(
            left_controller_->GetSerialNumber().c_str(),
            vr::TrackedDeviceClass_Controller,
            left_controller_.get())) {
        DriverLog("Failed to add left controller!\n");
        return vr::VRInitError_Driver_Unknown;
    }

    // Create right controller
    right_controller_ = std::make_unique<EmuControllerDriver>(vr::TrackedControllerRole_RightHand);
    if (!vr::VRServerDriverHost()->TrackedDeviceAdded(
            right_controller_->GetSerialNumber().c_str(),
            vr::TrackedDeviceClass_Controller,
            right_controller_.get())) {
        DriverLog("Failed to add right controller!\n");
        return vr::VRInitError_Driver_Unknown;
    }

    DriverLog("EmuController Driver initialized successfully!\n");
    return vr::VRInitError_None;
}

void EmuDeviceProvider::Cleanup() {
    DriverLog("EmuController Driver cleaning up...\n");

    if (ipc_server_) {
        ipc_server_->Stop();
        ipc_server_.reset();
    }

    hmd_.reset();
    left_controller_.reset();
    right_controller_.reset();

    CleanupDriverLog();
}

const char* const* EmuDeviceProvider::GetInterfaceVersions() {
    return vr::k_InterfaceVersions;
}

void EmuDeviceProvider::RunFrame() {
    // Get input from IPC
    if (ipc_server_ && ipc_server_->IsClientConnected()) {
        // Get controller input
        ControllerInputState left_state, right_state;
        if (ipc_server_->GetInputState(left_state, right_state)) {
            if (left_controller_) {
                left_controller_->UpdateInputState(left_state);
            }
            if (right_controller_) {
                right_controller_->UpdateInputState(right_state);
            }
        }

        // Get HMD pose
        HMDPoseData hmd_pose;
        if (ipc_server_->GetHMDPose(hmd_pose)) {
            if (hmd_) {
                hmd_->UpdatePose(hmd_pose);
            }
        }
    }

    // Run frame for each device
    if (hmd_) {
        hmd_->RunFrame();
    }
    if (left_controller_) {
        left_controller_->RunFrame();
    }
    if (right_controller_) {
        right_controller_->RunFrame();
    }

    // Process events
    vr::VREvent_t event{};
    while (vr::VRServerDriverHost()->PollNextEvent(&event, sizeof(vr::VREvent_t))) {
        if (hmd_) {
            hmd_->ProcessEvent(event);
        }
        if (left_controller_) {
            left_controller_->ProcessEvent(event);
        }
        if (right_controller_) {
            right_controller_->ProcessEvent(event);
        }
    }
}

bool EmuDeviceProvider::ShouldBlockStandbyMode() {
    return false;
}

void EmuDeviceProvider::EnterStandby() {
    DriverLog("EmuController Driver entering standby\n");
}

void EmuDeviceProvider::LeaveStandby() {
    DriverLog("EmuController Driver leaving standby\n");
}

} // namespace emu

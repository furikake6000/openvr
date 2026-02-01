#pragma once

#include <openvr_driver.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include "hmd_display_component.h"

namespace emu {

// HMD pose data from IPC
struct HMDPoseData {
    float position[3] = { 0.0f, 1.6f, 0.0f };   // x, y, z in meters
    float quaternion[4] = { 1.0f, 0.0f, 0.0f, 0.0f };  // w, x, y, z
};

class EmuHMDDriver : public vr::ITrackedDeviceServerDriver {
public:
    EmuHMDDriver();

    // ITrackedDeviceServerDriver interface
    vr::EVRInitError Activate(uint32_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override;
    void* GetComponent(const char* pchComponentNameAndVersion) override;
    void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
    vr::DriverPose_t GetPose() override;

    // Custom methods
    const std::string& GetSerialNumber() const { return serial_number_; }
    void UpdatePose(const HMDPoseData& pose);
    void RunFrame();
    void ProcessEvent(const vr::VREvent_t& event);

private:
    void PoseUpdateThread();

    std::atomic<bool> is_active_{false};
    vr::TrackedDeviceIndex_t device_index_ = vr::k_unTrackedDeviceIndexInvalid;

    std::string serial_number_ = "EMU-HMD-001";
    std::string model_number_ = "EmuHMD";

    std::unique_ptr<EmuHMDDisplayComponent> display_component_;
    HMDDisplayConfig display_config_;

    std::mutex pose_mutex_;
    HMDPoseData current_pose_;

    std::thread pose_thread_;
};

} // namespace emu

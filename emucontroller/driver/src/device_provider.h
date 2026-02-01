#pragma once

#include <openvr_driver.h>
#include <memory>
#include "controller_device_driver.h"
#include "hmd_device_driver.h"
#include "ipc_server.h"

namespace emu {

class EmuDeviceProvider : public vr::IServerTrackedDeviceProvider {
public:
    // IServerTrackedDeviceProvider interface
    vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext) override;
    void Cleanup() override;
    const char* const* GetInterfaceVersions() override;
    void RunFrame() override;
    bool ShouldBlockStandbyMode() override;
    void EnterStandby() override;
    void LeaveStandby() override;

private:
    std::unique_ptr<EmuHMDDriver> hmd_;
    std::unique_ptr<EmuControllerDriver> left_controller_;
    std::unique_ptr<EmuControllerDriver> right_controller_;
    std::unique_ptr<IPCServer> ipc_server_;
};

} // namespace emu

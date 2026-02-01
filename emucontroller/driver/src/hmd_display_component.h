#pragma once

#include <openvr_driver.h>
#include <cstdint>

namespace emu {

// HMD display configuration
struct HMDDisplayConfig {
    int32_t window_x = 100;
    int32_t window_y = 100;
    int32_t window_width = 1920;
    int32_t window_height = 1080;
    int32_t render_width = 1512;
    int32_t render_height = 1680;
};

// Display component for HMD - implements IVRDisplayComponent
class EmuHMDDisplayComponent : public vr::IVRDisplayComponent {
public:
    explicit EmuHMDDisplayComponent(const HMDDisplayConfig& config);

    // IVRDisplayComponent interface
    void GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) override;
    bool IsDisplayOnDesktop() override;
    bool IsDisplayRealDisplay() override;
    void GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) override;
    void GetEyeOutputViewport(vr::EVREye eEye, uint32_t* pnX, uint32_t* pnY,
                              uint32_t* pnWidth, uint32_t* pnHeight) override;
    void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight,
                          float* pfTop, float* pfBottom) override;
    vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU, float fV) override;
    bool ComputeInverseDistortion(vr::HmdVector2_t* pResult, vr::EVREye eEye,
                                   uint32_t unChannel, float fU, float fV) override;

private:
    HMDDisplayConfig config_;
};

} // namespace emu

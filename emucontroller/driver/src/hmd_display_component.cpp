#include "hmd_display_component.h"

namespace emu {

EmuHMDDisplayComponent::EmuHMDDisplayComponent(const HMDDisplayConfig& config)
    : config_(config) {
}

void EmuHMDDisplayComponent::GetWindowBounds(int32_t* pnX, int32_t* pnY,
                                              uint32_t* pnWidth, uint32_t* pnHeight) {
    *pnX = config_.window_x;
    *pnY = config_.window_y;
    *pnWidth = static_cast<uint32_t>(config_.window_width);
    *pnHeight = static_cast<uint32_t>(config_.window_height);
}

bool EmuHMDDisplayComponent::IsDisplayOnDesktop() {
    // Display on desktop window (not direct mode)
    return true;
}

bool EmuHMDDisplayComponent::IsDisplayRealDisplay() {
    // Not a real physical display
    return false;
}

void EmuHMDDisplayComponent::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) {
    *pnWidth = static_cast<uint32_t>(config_.render_width);
    *pnHeight = static_cast<uint32_t>(config_.render_height);
}

void EmuHMDDisplayComponent::GetEyeOutputViewport(vr::EVREye eEye, uint32_t* pnX, uint32_t* pnY,
                                                   uint32_t* pnWidth, uint32_t* pnHeight) {
    *pnY = 0;
    *pnWidth = static_cast<uint32_t>(config_.window_width / 2);
    *pnHeight = static_cast<uint32_t>(config_.window_height);

    if (eEye == vr::Eye_Left) {
        *pnX = 0;  // Left eye on left half
    } else {
        *pnX = static_cast<uint32_t>(config_.window_width / 2);  // Right eye on right half
    }
}

void EmuHMDDisplayComponent::GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight,
                                               float* pfTop, float* pfBottom) {
    (void)eEye;
    // Standard 90 degree FOV (tan(45) = 1.0)
    *pfLeft = -1.0f;
    *pfRight = 1.0f;
    *pfTop = -1.0f;
    *pfBottom = 1.0f;
}

vr::DistortionCoordinates_t EmuHMDDisplayComponent::ComputeDistortion(vr::EVREye eEye,
                                                                       float fU, float fV) {
    (void)eEye;
    // No distortion for emulator
    vr::DistortionCoordinates_t coords{};
    coords.rfRed[0] = fU;
    coords.rfRed[1] = fV;
    coords.rfGreen[0] = fU;
    coords.rfGreen[1] = fV;
    coords.rfBlue[0] = fU;
    coords.rfBlue[1] = fV;
    return coords;
}

bool EmuHMDDisplayComponent::ComputeInverseDistortion(vr::HmdVector2_t* pResult, vr::EVREye eEye,
                                                       uint32_t unChannel, float fU, float fV) {
    (void)eEye;
    (void)unChannel;
    // Return false to let SteamVR compute from ComputeDistortion
    if (pResult) {
        pResult->v[0] = fU;
        pResult->v[1] = fV;
    }
    return false;
}

} // namespace emu

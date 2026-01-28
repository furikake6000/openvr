# HMD実装テンプレート

## hmd_device.h

```cpp
#pragma once
#include "openvr_driver.h"
#include <string>
#include <thread>
#include <atomic>

struct DisplayConfig
{
    int32_t windowX = 0, windowY = 0;
    uint32_t windowW = 2880, windowH = 1600;
    uint32_t renderW = 1440, renderH = 1600;
    float refreshRate = 90.0f;
    float ipd = 0.063f;
};

class HMDDevice : public vr::ITrackedDeviceServerDriver, public vr::IVRDisplayComponent
{
public:
    HMDDevice(const std::string &serial, const DisplayConfig &cfg);
    ~HMDDevice();

    // ITrackedDeviceServerDriver
    vr::EVRInitError Activate(uint32_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override;
    void *GetComponent(const char *pchComponentNameAndVersion) override;
    void DebugRequest(const char *, char *buf, uint32_t size) override;
    vr::DriverPose_t GetPose() override;

    // IVRDisplayComponent
    void GetWindowBounds(int32_t *x, int32_t *y, uint32_t *w, uint32_t *h) override;
    bool IsDisplayOnDesktop() override;
    bool IsDisplayRealDisplay() override;
    void GetRecommendedRenderTargetSize(uint32_t *w, uint32_t *h) override;
    void GetEyeOutputViewport(vr::EVREye eye, uint32_t *x, uint32_t *y,
                              uint32_t *w, uint32_t *h) override;
    void GetProjectionRaw(vr::EVREye eye, float *l, float *r, float *t, float *b) override;
    vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eye, float u, float v) override;
    bool ComputeInverseDistortion(vr::HmdVector2_t *, vr::EVREye, uint32_t, float, float) override;

    const std::string &GetSerial() const { return m_serial; }
    void UpdatePose(const vr::DriverPose_t &pose);

private:
    void PoseThread();

    std::string m_serial;
    DisplayConfig m_cfg;
    uint32_t m_index = vr::k_unTrackedDeviceIndexInvalid;
    vr::DriverPose_t m_pose = {};
    std::thread m_thread;
    std::atomic<bool> m_active{false};
};
```

## hmd_device.cpp

```cpp
#include "hmd_device.h"
#include <cstring>
#include <chrono>

HMDDevice::HMDDevice(const std::string &serial, const DisplayConfig &cfg)
    : m_serial(serial), m_cfg(cfg)
{
    memset(&m_pose, 0, sizeof(m_pose));
    m_pose.qWorldFromDriverRotation.w = 1.0f;
    m_pose.qDriverFromHeadRotation.w = 1.0f;
    m_pose.qRotation.w = 1.0f;
    m_pose.vecPosition[1] = 1.6f;  // 目の高さ
}

HMDDevice::~HMDDevice()
{
    if (m_active.exchange(false) && m_thread.joinable())
        m_thread.join();
}

vr::EVRInitError HMDDevice::Activate(uint32_t unObjectId)
{
    m_index = unObjectId;
    m_active = true;

    auto c = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

    vr::VRProperties()->SetStringProperty(c, vr::Prop_ModelNumber_String, "MyHMD v1");
    vr::VRProperties()->SetFloatProperty(c, vr::Prop_UserIpdMeters_Float, m_cfg.ipd);
    vr::VRProperties()->SetFloatProperty(c, vr::Prop_DisplayFrequency_Float, m_cfg.refreshRate);
    vr::VRProperties()->SetFloatProperty(c, vr::Prop_SecondsFromVsyncToPhotons_Float,
        1.0f / m_cfg.refreshRate);
    vr::VRProperties()->SetBoolProperty(c, vr::Prop_IsOnDesktop_Bool, false);

    m_thread = std::thread(&HMDDevice::PoseThread, this);
    return vr::VRInitError_None;
}

void HMDDevice::Deactivate()
{
    if (m_active.exchange(false) && m_thread.joinable())
        m_thread.join();
    m_index = vr::k_unTrackedDeviceIndexInvalid;
}

void HMDDevice::EnterStandby() {}

void *HMDDevice::GetComponent(const char *name)
{
    if (strcmp(name, vr::IVRDisplayComponent_Version) == 0)
        return static_cast<vr::IVRDisplayComponent*>(this);
    return nullptr;
}

void HMDDevice::DebugRequest(const char *, char *buf, uint32_t size)
{
    if (size >= 1) buf[0] = 0;
}

vr::DriverPose_t HMDDevice::GetPose() { return m_pose; }

// IVRDisplayComponent
void HMDDevice::GetWindowBounds(int32_t *x, int32_t *y, uint32_t *w, uint32_t *h)
{
    *x = m_cfg.windowX; *y = m_cfg.windowY;
    *w = m_cfg.windowW; *h = m_cfg.windowH;
}

bool HMDDevice::IsDisplayOnDesktop() { return false; }
bool HMDDevice::IsDisplayRealDisplay() { return true; }

void HMDDevice::GetRecommendedRenderTargetSize(uint32_t *w, uint32_t *h)
{
    *w = m_cfg.renderW; *h = m_cfg.renderH;
}

void HMDDevice::GetEyeOutputViewport(vr::EVREye eye, uint32_t *x, uint32_t *y,
                                      uint32_t *w, uint32_t *h)
{
    *y = 0;
    *w = m_cfg.windowW / 2;
    *h = m_cfg.windowH;
    *x = (eye == vr::Eye_Left) ? 0 : m_cfg.windowW / 2;
}

void HMDDevice::GetProjectionRaw(vr::EVREye, float *l, float *r, float *t, float *b)
{
    // 90度FOV
    *l = -1.0f; *r = 1.0f;
    *t = -1.0f; *b = 1.0f;
}

vr::DistortionCoordinates_t HMDDevice::ComputeDistortion(vr::EVREye, float u, float v)
{
    vr::DistortionCoordinates_t c = {};
    c.rfRed[0] = c.rfGreen[0] = c.rfBlue[0] = u;
    c.rfRed[1] = c.rfGreen[1] = c.rfBlue[1] = v;
    return c;
}

bool HMDDevice::ComputeInverseDistortion(vr::HmdVector2_t *, vr::EVREye,
                                          uint32_t, float, float)
{
    return false;
}

void HMDDevice::UpdatePose(const vr::DriverPose_t &pose) { m_pose = pose; }

void HMDDevice::PoseThread()
{
    while (m_active)
    {
        m_pose.poseIsValid = true;
        m_pose.deviceIsConnected = true;
        m_pose.result = vr::TrackingResult_Running_OK;

        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
            m_index, GetPose(), sizeof(vr::DriverPose_t));
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
```

## 使用例

```cpp
// DeviceProvider::Init内
DisplayConfig cfg;
cfg.windowW = 2880; cfg.windowH = 1600;
cfg.renderW = 1440; cfg.renderH = 1600;
cfg.refreshRate = 90.0f;
cfg.ipd = 0.063f;

auto hmd = std::make_unique<HMDDevice>("MyHMD_001", cfg);
vr::VRServerDriverHost()->TrackedDeviceAdded(
    hmd->GetSerial().c_str(),
    vr::TrackedDeviceClass_HMD,
    hmd.get());
```

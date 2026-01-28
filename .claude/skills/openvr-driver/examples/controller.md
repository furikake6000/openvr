# コントローラー実装テンプレート

## controller_device.h

```cpp
#pragma once
#include "openvr_driver.h"
#include <string>
#include <thread>
#include <atomic>
#include <array>

class ControllerDevice : public vr::ITrackedDeviceServerDriver
{
public:
    ControllerDevice(const std::string &serial, vr::ETrackedControllerRole role);
    ~ControllerDevice();

    // ITrackedDeviceServerDriver
    vr::EVRInitError Activate(uint32_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override;
    void *GetComponent(const char *pchComponentNameAndVersion) override;
    void DebugRequest(const char *pchRequest, char *pchResponseBuffer,
                      uint32_t unResponseBufferSize) override;
    vr::DriverPose_t GetPose() override;

    const std::string &GetSerial() const { return m_serial; }
    void RunFrame();
    void ProcessEvent(const vr::VREvent_t &event);

    // 入力更新
    void UpdateTrigger(float value, bool clicked);
    void UpdateJoystick(float x, float y, bool clicked, bool touched);
    void UpdateButtons(bool aClick, bool aTouch, bool bClick, bool bTouch);
    void UpdatePose(const vr::DriverPose_t &pose);

private:
    void PoseThread();

    std::string m_serial;
    vr::ETrackedControllerRole m_role;
    uint32_t m_index = vr::k_unTrackedDeviceIndexInvalid;

    enum Handle { kTriggerVal, kTriggerClick, kJoyX, kJoyY, kJoyClick,
                  kJoyTouch, kAClick, kATouch, kBClick, kBTouch, kHaptic, kCount };
    std::array<vr::VRInputComponentHandle_t, kCount> m_handles = {};

    vr::DriverPose_t m_pose = {};
    std::thread m_thread;
    std::atomic<bool> m_active{false};
};
```

## controller_device.cpp

```cpp
#include "controller_device.h"
#include <cstring>
#include <chrono>

ControllerDevice::ControllerDevice(const std::string &serial,
                                   vr::ETrackedControllerRole role)
    : m_serial(serial), m_role(role)
{
    memset(&m_pose, 0, sizeof(m_pose));
    m_pose.qWorldFromDriverRotation.w = 1.0f;
    m_pose.qDriverFromHeadRotation.w = 1.0f;
    m_pose.qRotation.w = 1.0f;
}

ControllerDevice::~ControllerDevice()
{
    if (m_active.exchange(false) && m_thread.joinable())
        m_thread.join();
}

vr::EVRInitError ControllerDevice::Activate(uint32_t unObjectId)
{
    m_index = unObjectId;
    m_active = true;

    auto c = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

    // プロパティ
    vr::VRProperties()->SetStringProperty(c, vr::Prop_ControllerType_String, "mycontroller");
    vr::VRProperties()->SetStringProperty(c, vr::Prop_ModelNumber_String, "MyController v1");
    vr::VRProperties()->SetInt32Property(c, vr::Prop_ControllerRoleHint_Int32, m_role);
    vr::VRProperties()->SetStringProperty(c, vr::Prop_InputProfilePath_String,
        "{mydriver}/input/mycontroller_profile.json");

    // 入力コンポーネント
    vr::VRDriverInput()->CreateScalarComponent(c, "/input/trigger/value",
        &m_handles[kTriggerVal], vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedOneSided);
    vr::VRDriverInput()->CreateBooleanComponent(c, "/input/trigger/click",
        &m_handles[kTriggerClick]);

    vr::VRDriverInput()->CreateScalarComponent(c, "/input/joystick/x",
        &m_handles[kJoyX], vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent(c, "/input/joystick/y",
        &m_handles[kJoyY], vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateBooleanComponent(c, "/input/joystick/click",
        &m_handles[kJoyClick]);
    vr::VRDriverInput()->CreateBooleanComponent(c, "/input/joystick/touch",
        &m_handles[kJoyTouch]);

    vr::VRDriverInput()->CreateBooleanComponent(c, "/input/a/click", &m_handles[kAClick]);
    vr::VRDriverInput()->CreateBooleanComponent(c, "/input/a/touch", &m_handles[kATouch]);
    vr::VRDriverInput()->CreateBooleanComponent(c, "/input/b/click", &m_handles[kBClick]);
    vr::VRDriverInput()->CreateBooleanComponent(c, "/input/b/touch", &m_handles[kBTouch]);

    vr::VRDriverInput()->CreateHapticComponent(c, "/output/haptic", &m_handles[kHaptic]);

    m_thread = std::thread(&ControllerDevice::PoseThread, this);
    return vr::VRInitError_None;
}

void ControllerDevice::Deactivate()
{
    if (m_active.exchange(false) && m_thread.joinable())
        m_thread.join();
    m_index = vr::k_unTrackedDeviceIndexInvalid;
}

void ControllerDevice::EnterStandby() {}

void *ControllerDevice::GetComponent(const char *) { return nullptr; }

void ControllerDevice::DebugRequest(const char *, char *buf, uint32_t size)
{
    if (size >= 1) buf[0] = 0;
}

vr::DriverPose_t ControllerDevice::GetPose() { return m_pose; }

void ControllerDevice::RunFrame() {}

void ControllerDevice::ProcessEvent(const vr::VREvent_t &e)
{
    if (e.eventType == vr::VREvent_Input_HapticVibration &&
        e.data.hapticVibration.componentHandle == m_handles[kHaptic])
    {
        // ハプティック処理
        float dur = e.data.hapticVibration.fDurationSeconds;
        float freq = e.data.hapticVibration.fFrequency;
        float amp = e.data.hapticVibration.fAmplitude;
        // ハードウェアに送信
    }
}

void ControllerDevice::UpdateTrigger(float value, bool clicked)
{
    vr::VRDriverInput()->UpdateScalarComponent(m_handles[kTriggerVal], value, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(m_handles[kTriggerClick], clicked, 0);
}

void ControllerDevice::UpdateJoystick(float x, float y, bool clicked, bool touched)
{
    vr::VRDriverInput()->UpdateScalarComponent(m_handles[kJoyX], x, 0);
    vr::VRDriverInput()->UpdateScalarComponent(m_handles[kJoyY], y, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(m_handles[kJoyClick], clicked, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(m_handles[kJoyTouch], touched, 0);
}

void ControllerDevice::UpdateButtons(bool aClick, bool aTouch, bool bClick, bool bTouch)
{
    vr::VRDriverInput()->UpdateBooleanComponent(m_handles[kAClick], aClick, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(m_handles[kATouch], aTouch, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(m_handles[kBClick], bClick, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(m_handles[kBTouch], bTouch, 0);
}

void ControllerDevice::UpdatePose(const vr::DriverPose_t &pose) { m_pose = pose; }

void ControllerDevice::PoseThread()
{
    while (m_active)
    {
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
            m_index, GetPose(), sizeof(vr::DriverPose_t));
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
```

## 入力プロファイル (mycontroller_profile.json)

```json
{
  "jsonid": "input_profile",
  "controller_type": "mycontroller",
  "input_bindingui_mode": "controller_handed",
  "input_source": {
    "/input/trigger": { "type": "trigger", "click": true, "value": true, "order": 1 },
    "/input/joystick": { "type": "joystick", "click": true, "touch": true, "order": 2 },
    "/input/a": { "type": "button", "click": true, "touch": true, "order": 3 },
    "/input/b": { "type": "button", "click": true, "touch": true, "order": 4 },
    "/output/haptic": { "type": "vibration", "order": 5 }
  }
}
```

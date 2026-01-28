# トラッカー実装テンプレート

## tracker_device.h

```cpp
#pragma once
#include "openvr_driver.h"
#include <string>
#include <thread>
#include <atomic>

class TrackerDevice : public vr::ITrackedDeviceServerDriver
{
public:
    TrackerDevice(const std::string &serial);
    ~TrackerDevice();

    vr::EVRInitError Activate(uint32_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override;
    void *GetComponent(const char *pchComponentNameAndVersion) override;
    void DebugRequest(const char *, char *buf, uint32_t size) override;
    vr::DriverPose_t GetPose() override;

    const std::string &GetSerial() const { return m_serial; }
    void UpdatePose(const vr::DriverPose_t &pose);

private:
    void PoseThread();

    std::string m_serial;
    uint32_t m_index = vr::k_unTrackedDeviceIndexInvalid;
    vr::DriverPose_t m_pose = {};
    std::thread m_thread;
    std::atomic<bool> m_active{false};
};
```

## tracker_device.cpp

```cpp
#include "tracker_device.h"
#include <cstring>
#include <chrono>

TrackerDevice::TrackerDevice(const std::string &serial)
    : m_serial(serial)
{
    memset(&m_pose, 0, sizeof(m_pose));
    m_pose.qWorldFromDriverRotation.w = 1.0f;
    m_pose.qDriverFromHeadRotation.w = 1.0f;
    m_pose.qRotation.w = 1.0f;
}

TrackerDevice::~TrackerDevice()
{
    if (m_active.exchange(false) && m_thread.joinable())
        m_thread.join();
}

vr::EVRInitError TrackerDevice::Activate(uint32_t unObjectId)
{
    m_index = unObjectId;
    m_active = true;

    auto c = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

    vr::VRProperties()->SetStringProperty(c, vr::Prop_ModelNumber_String, "MyTracker v1");
    vr::VRProperties()->SetStringProperty(c, vr::Prop_ManufacturerName_String, "MyCompany");

    // 入力プロファイル（トラッカーにボタン等がある場合）
    vr::VRProperties()->SetStringProperty(c, vr::Prop_InputProfilePath_String,
        "{mydriver}/input/mytracker_profile.json");

    m_thread = std::thread(&TrackerDevice::PoseThread, this);
    return vr::VRInitError_None;
}

void TrackerDevice::Deactivate()
{
    if (m_active.exchange(false) && m_thread.joinable())
        m_thread.join();
    m_index = vr::k_unTrackedDeviceIndexInvalid;
}

void TrackerDevice::EnterStandby() {}

void *TrackerDevice::GetComponent(const char *) { return nullptr; }

void TrackerDevice::DebugRequest(const char *, char *buf, uint32_t size)
{
    if (size >= 1) buf[0] = 0;
}

vr::DriverPose_t TrackerDevice::GetPose() { return m_pose; }

void TrackerDevice::UpdatePose(const vr::DriverPose_t &pose) { m_pose = pose; }

void TrackerDevice::PoseThread()
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

## 入力プロファイル (mytracker_profile.json)

```json
{
  "jsonid": "input_profile",
  "controller_type": "mytracker",
  "device_class": "TrackedDeviceClass_GenericTracker",
  "input_bindingui_mode": "single_device",
  "input_source": {}
}
```

## 使用例: 複数トラッカー

```cpp
// DeviceProvider::Init内
const int NUM_TRACKERS = 3;
for (int i = 0; i < NUM_TRACKERS; i++)
{
    std::string serial = "MyTracker_" + std::to_string(i);
    auto tracker = std::make_unique<TrackerDevice>(serial);

    if (vr::VRServerDriverHost()->TrackedDeviceAdded(
            tracker->GetSerial().c_str(),
            vr::TrackedDeviceClass_GenericTracker,
            tracker.get()))
    {
        m_trackers.push_back(std::move(tracker));
    }
}
```

## トラッカーロールについて

トラッカーのボディ部位（腰、足など）への割り当ては**ユーザーがSteamVR設定で行う**。
ドライバー側でロールを設定する必要はない。

ユーザー設定可能なロール:
- 腰 (Waist)
- 左足/右足 (LeftFoot/RightFoot)
- 左膝/右膝 (LeftKnee/RightKnee)
- 胸 (Chest)
- 左肘/右肘 (LeftElbow/RightElbow)
- 左肩/右肩 (LeftShoulder/RightShoulder)

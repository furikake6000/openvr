# 共通ユーティリティ

## HmdDriverFactory（エントリポイント）

```cpp
#include "device_provider.h"

#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec(dllexport)
#elif defined(__GNUC__) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static MyDeviceProvider g_provider;

HMD_DLL_EXPORT void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode)
{
    if (strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName) == 0)
        return &g_provider;
    if (pReturnCode) *pReturnCode = vr::VRInitError_Init_InterfaceNotFound;
    return nullptr;
}
```

## DeviceProvider（基本テンプレート）

```cpp
#pragma once
#include "openvr_driver.h"
#include <memory>
#include <vector>

class MyDeviceProvider : public vr::IServerTrackedDeviceProvider
{
public:
    vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext) override
    {
        VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
        // デバイス作成・追加
        return vr::VRInitError_None;
    }

    void Cleanup() override
    {
        m_devices.clear();
        VR_CLEANUP_SERVER_DRIVER_CONTEXT();
    }

    const char *const *GetInterfaceVersions() override
    {
        return vr::k_InterfaceVersions;
    }

    void RunFrame() override
    {
        vr::VREvent_t event;
        while (vr::VRServerDriverHost()->PollNextEvent(&event, sizeof(event)))
        {
            for (auto &dev : m_devices)
                dev->ProcessEvent(event);
        }
        for (auto &dev : m_devices)
            dev->RunFrame();
    }

    bool ShouldBlockStandbyMode() override { return false; }
    void EnterStandby() override {}
    void LeaveStandby() override {}

private:
    std::vector<std::unique_ptr<MyDevice>> m_devices;
};
```

## クォータニオン変換

```cpp
// 行列からクォータニオン
vr::HmdQuaternion_t MatrixToQuat(const vr::HmdMatrix34_t &m)
{
    vr::HmdQuaternion_t q = {};
    q.w = sqrt(fmax(0, 1 + m.m[0][0] + m.m[1][1] + m.m[2][2])) / 2;
    q.x = sqrt(fmax(0, 1 + m.m[0][0] - m.m[1][1] - m.m[2][2])) / 2;
    q.y = sqrt(fmax(0, 1 - m.m[0][0] + m.m[1][1] - m.m[2][2])) / 2;
    q.z = sqrt(fmax(0, 1 - m.m[0][0] - m.m[1][1] + m.m[2][2])) / 2;
    q.x = copysign(q.x, m.m[2][1] - m.m[1][2]);
    q.y = copysign(q.y, m.m[0][2] - m.m[2][0]);
    q.z = copysign(q.z, m.m[1][0] - m.m[0][1]);
    return q;
}

// オイラー角からクォータニオン (ZYX順)
vr::HmdQuaternion_t EulerToQuat(float pitch, float yaw, float roll)
{
    float cy = cos(yaw * 0.5f), sy = sin(yaw * 0.5f);
    float cp = cos(pitch * 0.5f), sp = sin(pitch * 0.5f);
    float cr = cos(roll * 0.5f), sr = sin(roll * 0.5f);

    vr::HmdQuaternion_t q = {};
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return q;
}

// 単位クォータニオン
vr::HmdQuaternion_t IdentityQuat()
{
    return {1.0, 0.0, 0.0, 0.0};  // w, x, y, z
}
```

## ポーズ作成ヘルパー

```cpp
vr::DriverPose_t CreatePose(double x, double y, double z,
                            const vr::HmdQuaternion_t &rot)
{
    vr::DriverPose_t pose = {};

    pose.qWorldFromDriverRotation.w = 1.0f;
    pose.qDriverFromHeadRotation.w = 1.0f;

    pose.vecPosition[0] = x;
    pose.vecPosition[1] = y;
    pose.vecPosition[2] = z;
    pose.qRotation = rot;

    pose.poseIsValid = true;
    pose.deviceIsConnected = true;
    pose.result = vr::TrackingResult_Running_OK;

    return pose;
}
```

## HMDポーズ取得

```cpp
vr::DriverPose_t GetHMDPose()
{
    vr::TrackedDevicePose_t hmdPose{};
    vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0.f, &hmdPose, 1);

    vr::DriverPose_t pose = {};
    pose.qWorldFromDriverRotation.w = 1.0f;
    pose.qDriverFromHeadRotation.w = 1.0f;

    if (hmdPose.bPoseIsValid)
    {
        auto &m = hmdPose.mDeviceToAbsoluteTracking;
        pose.vecPosition[0] = m.m[0][3];
        pose.vecPosition[1] = m.m[1][3];
        pose.vecPosition[2] = m.m[2][3];
        pose.qRotation = MatrixToQuat(m);
        pose.poseIsValid = true;
        pose.deviceIsConnected = true;
        pose.result = vr::TrackingResult_Running_OK;
    }

    return pose;
}
```

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(mydriver)

set(CMAKE_CXX_STANDARD 17)

set(OPENVR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/openvr)

if(WIN32)
    add_definitions(-D_WIN32)
    set(ARCH win64)
elseif(UNIX)
    add_definitions(-DLINUX -DPOSIX)
    set(ARCH linux64)
endif()

find_library(OPENVR_LIB openvr_api
    PATHS ${OPENVR_DIR}/lib/${ARCH})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/mydriver/bin/${ARCH})

add_library(driver_mydriver SHARED
    src/hmd_driver_factory.cpp
    src/device_provider.cpp
    src/my_device.cpp
)

target_include_directories(driver_mydriver PRIVATE ${OPENVR_DIR}/headers)
target_link_libraries(driver_mydriver PRIVATE ${OPENVR_LIB})

# リソースをコピー
add_custom_command(TARGET driver_mydriver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/mydriver ${CMAKE_BINARY_DIR}/mydriver)
```

## SteamVRへの登録

```bash
# Windows
"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrpathreg.exe" adddriver "C:\path\to\mydriver"

# Linux
~/.steam/steam/steamapps/common/SteamVR/bin/linux64/vrpathreg adddriver "/path/to/mydriver"

# 登録解除
vrpathreg removedriver "/path/to/mydriver"
```

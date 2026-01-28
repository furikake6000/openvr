# OpenVR ドライバーインターフェース

## 目次
- IServerTrackedDeviceProvider
- ITrackedDeviceServerDriver
- IVRDisplayComponent（HMD用）
- IVRServerDriverHost

---

## IServerTrackedDeviceProvider

ドライバーのメインプロバイダー。グローバルシングルトンとして存在する必要があります。

```cpp
class MyDeviceProvider : public vr::IServerTrackedDeviceProvider
{
public:
    // 初期化（リソース確保、デバイス追加）
    vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext) override
    {
        VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
        // デバイスを追加
        vr::VRServerDriverHost()->TrackedDeviceAdded(
            "SerialNumber", vr::TrackedDeviceClass_Controller, &m_device);
        return vr::VRInitError_None;
    }

    void Cleanup() override
    {
        VR_CLEANUP_SERVER_DRIVER_CONTEXT();
    }

    const char *const *GetInterfaceVersions() override
    {
        return vr::k_InterfaceVersions;
    }

    void RunFrame() override
    {
        // 毎フレーム呼ばれる。イベントポーリング推奨
        vr::VREvent_t event;
        while (vr::VRServerDriverHost()->PollNextEvent(&event, sizeof(event)))
        {
            // イベント処理
        }
    }

    bool ShouldBlockStandbyMode() override { return false; } // 非推奨
    void EnterStandby() override { }
    void LeaveStandby() override { }
};
```

---

## ITrackedDeviceServerDriver

個々のデバイスを表現するインターフェース。

```cpp
class MyDevice : public vr::ITrackedDeviceServerDriver
{
public:
    vr::EVRInitError Activate(uint32_t unObjectId) override
    {
        m_deviceIndex = unObjectId;

        // プロパティコンテナを取得
        vr::PropertyContainerHandle_t container =
            vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

        // プロパティ設定
        vr::VRProperties()->SetStringProperty(container,
            vr::Prop_ModelNumber_String, "MyModel");

        // 入力コンポーネント作成
        vr::VRDriverInput()->CreateBooleanComponent(container,
            "/input/trigger/click", &m_triggerClick);

        return vr::VRInitError_None;
    }

    void Deactivate() override
    {
        m_deviceIndex = vr::k_unTrackedDeviceIndexInvalid;
    }

    void EnterStandby() override { }

    void *GetComponent(const char *pchComponentNameAndVersion) override
    {
        // HMDの場合はIVRDisplayComponentを返す
        return nullptr;
    }

    void DebugRequest(const char *pchRequest, char *pchResponseBuffer,
                      uint32_t unResponseBufferSize) override
    {
        if (unResponseBufferSize >= 1) pchResponseBuffer[0] = 0;
    }

    vr::DriverPose_t GetPose() override
    {
        return m_pose;
    }
};
```

---

## IVRDisplayComponent（HMD専用）

HMDデバイスは `GetComponent()` で返す必要があります。

```cpp
void *MyHMD::GetComponent(const char *pchComponentNameAndVersion)
{
    if (strcmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version) == 0)
        return static_cast<vr::IVRDisplayComponent*>(this);
    return nullptr;
}

class MyDisplayComponent : public vr::IVRDisplayComponent
{
public:
    void GetWindowBounds(int32_t *pnX, int32_t *pnY,
                         uint32_t *pnWidth, uint32_t *pnHeight) override
    {
        *pnX = 0; *pnY = 0;
        *pnWidth = 2880; *pnHeight = 1600;
    }

    bool IsDisplayOnDesktop() override { return false; }
    bool IsDisplayRealDisplay() override { return true; }

    void GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight) override
    {
        *pnWidth = 1440; *pnHeight = 1600;
    }

    void GetEyeOutputViewport(vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY,
                              uint32_t *pnWidth, uint32_t *pnHeight) override
    {
        *pnY = 0;
        *pnWidth = 2880 / 2;
        *pnHeight = 1600;
        *pnX = (eEye == vr::Eye_Left) ? 0 : 2880 / 2;
    }

    void GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight,
                          float *pfTop, float *pfBottom) override
    {
        // 90度FOV: tan(45°) = 1.0
        *pfLeft = -1.0f; *pfRight = 1.0f;
        *pfTop = -1.0f; *pfBottom = 1.0f;
    }

    vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye,
                                                   float fU, float fV) override
    {
        vr::DistortionCoordinates_t coords = {};
        coords.rfRed[0] = coords.rfGreen[0] = coords.rfBlue[0] = fU;
        coords.rfRed[1] = coords.rfGreen[1] = coords.rfBlue[1] = fV;
        return coords;
    }

    bool ComputeInverseDistortion(vr::HmdVector2_t *pResult, vr::EVREye eEye,
                                   uint32_t unChannel, float fU, float fV) override
    {
        return false;  // SteamVRに推定させる
    }
};
```

---

## IVRServerDriverHost

ランタイムへの通知に使用。`vr::VRServerDriverHost()` で取得。

### 主要メソッド

```cpp
// デバイス追加
bool TrackedDeviceAdded(const char *pchDeviceSerialNumber,
                        ETrackedDeviceClass eDeviceClass,
                        ITrackedDeviceServerDriver *pDriver);

// ポーズ更新
void TrackedDevicePoseUpdated(uint32_t unWhichDevice,
                              const DriverPose_t &newPose,
                              uint32_t unPoseStructSize);

// イベントポーリング
bool PollNextEvent(VREvent_t *pEvent, uint32_t uncbVREvent);

// 他デバイスのポーズ取得
void GetRawTrackedDevicePoses(float fPredictedSecondsFromNow,
                              TrackedDevicePose_t *pTrackedDevicePoseArray,
                              uint32_t unTrackedDevicePoseArrayCount);

// ランタイム終了確認
bool IsExiting();
```

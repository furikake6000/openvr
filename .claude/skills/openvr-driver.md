# OpenVR ドライバー開発スキル

このスキルは、OpenVR/SteamVR向けのドライバー（HMD、コントローラー、トラッカー）を新規開発する際のガイドおよびコード生成支援を行います。

## 概要

OpenVRは、VRハードウェアとSteamVR/OpenVRアプリケーション間のインターフェースを提供するAPIです。ドライバーはデバイス非依存の設計となっており、OpenVR Driver APIに準拠すれば、アプリケーション側での個別サポートなしにSteamVRゲームで動作します。

## 重要な参照ファイル

- **ドキュメント**: `docs/Driver_API_Documentation.md`
- **ヘッダーファイル**: `headers/openvr_driver.h`
- **サンプルドライバー**: `samples/drivers/drivers/` 配下
  - `barebones/` - 最小限の実装
  - `simplehmd/` - シンプルなHMD
  - `simplecontroller/` - シンプルなコントローラー
  - `simpletrackers/` - トラッカー
  - `handskeletonsimulation/` - スケレタル入力
  - `tutorial/` - 詳細なチュートリアル（推奨）

## ドライバーフォルダ構造

```
<driver_name>/
├── bin/
│   ├── win64/
│   │   └── driver_<driver_name>.dll    # バイナリ（必須の命名規則）
│   └── linux64/
│       └── driver_<driver_name>.so
├── resources/
│   ├── icons/
│   │   └── *.png, *.gif                # デバイスアイコン
│   ├── input/
│   │   ├── <device>_profile.json       # 入力プロファイル
│   │   └── default_bindings/           # デフォルトバインディング
│   ├── settings/
│   │   └── default.vrsettings          # デフォルト設定
│   ├── localization/
│   │   └── localization.json           # ローカライズ
│   └── driver.vrresources              # リソース定義
└── driver.vrdrivermanifest             # ドライバーマニフェスト（必須）
```

## デバイスクラス (ETrackedDeviceClass)

| クラス | 説明 | 用途 |
|--------|------|------|
| `TrackedDeviceClass_HMD` | ヘッドマウントディスプレイ | VRヘッドセット |
| `TrackedDeviceClass_Controller` | コントローラー | ハンドコントローラー |
| `TrackedDeviceClass_GenericTracker` | 汎用トラッカー | フルボディトラッキング等 |
| `TrackedDeviceClass_TrackingReference` | トラッキング参照点 | ベースステーション、カメラ |
| `TrackedDeviceClass_DisplayRedirect` | ディスプレイリダイレクト | ワイヤレス伝送等 |

## 必須実装インターフェース

### 1. HmdDriverFactory（エントリポイント）

```cpp
extern "C" __declspec(dllexport)
void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode)
{
    if (0 == strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName))
    {
        return &g_deviceProvider;
    }

    if (pReturnCode)
        *pReturnCode = vr::VRInitError_Init_InterfaceNotFound;

    return nullptr;
}
```

### 2. IServerTrackedDeviceProvider

ドライバーのメインプロバイダー。グローバルシングルトンとして存在する必要があります。

```cpp
class MyDeviceProvider : public vr::IServerTrackedDeviceProvider
{
public:
    // 初期化（リソース確保、デバイス追加）
    vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext) override
    {
        VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

        // デバイスをランタイムに追加
        vr::VRServerDriverHost()->TrackedDeviceAdded(
            "MyDevice_SerialNumber",           // ユニークなシリアル番号
            vr::TrackedDeviceClass_Controller, // デバイスクラス
            &m_myDevice                        // ITrackedDeviceServerDriverのポインタ
        );

        return vr::VRInitError_None;
    }

    void Cleanup() override { /* リソース解放 */ }
    const char *const *GetInterfaceVersions() override { return vr::k_InterfaceVersions; }
    void RunFrame() override { /* 毎フレーム呼ばれる。イベントポーリング推奨 */ }
    bool ShouldBlockStandbyMode() override { return false; } // 非推奨
    void EnterStandby() override { /* スタンバイモード移行時 */ }
    void LeaveStandby() override { /* スタンバイモード復帰時 */ }
};
```

### 3. ITrackedDeviceServerDriver

個々のデバイスを表現するインターフェース。

```cpp
class MyDevice : public vr::ITrackedDeviceServerDriver
{
public:
    // デバイスがアクティベートされた時
    vr::EVRInitError Activate(uint32_t unObjectId) override
    {
        m_deviceIndex = unObjectId;

        // プロパティコンテナを取得
        vr::PropertyContainerHandle_t container =
            vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

        // プロパティを設定
        vr::VRProperties()->SetStringProperty(container,
            vr::Prop_ModelNumber_String, "MyModel");
        vr::VRProperties()->SetStringProperty(container,
            vr::Prop_InputProfilePath_String,
            "{mydriver}/input/mydevice_profile.json");

        // 入力コンポーネントを作成
        vr::VRDriverInput()->CreateBooleanComponent(container,
            "/input/trigger/click", &m_triggerClick);
        vr::VRDriverInput()->CreateScalarComponent(container,
            "/input/trigger/value", &m_triggerValue,
            vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);

        return vr::VRInitError_None;
    }

    void Deactivate() override { m_deviceIndex = vr::k_unTrackedDeviceIndexInvalid; }
    void EnterStandby() override { }
    void *GetComponent(const char *pchComponentNameAndVersion) override { return nullptr; }
    void DebugRequest(const char *, char *pchResponseBuffer, uint32_t unResponseBufferSize) override
    {
        if (unResponseBufferSize >= 1) pchResponseBuffer[0] = 0;
    }
    vr::DriverPose_t GetPose() override { return m_pose; }

private:
    uint32_t m_deviceIndex = vr::k_unTrackedDeviceIndexInvalid;
    vr::VRInputComponentHandle_t m_triggerClick;
    vr::VRInputComponentHandle_t m_triggerValue;
    vr::DriverPose_t m_pose;
};
```

## ポーズ (DriverPose_t)

ポーズはデバイスの位置・回転・トラッキング状態を表します。

```cpp
vr::DriverPose_t GetPose()
{
    vr::DriverPose_t pose = {0};

    // 必須: クォータニオンは有効な値（w=1で単位クォータニオン）
    pose.qWorldFromDriverRotation.w = 1.0f;
    pose.qDriverFromHeadRotation.w = 1.0f;
    pose.qRotation.w = 1.0f;

    // 位置 (メートル単位、右手座標系: +Y=上, +X=右, -Z=前方)
    pose.vecPosition[0] = x;
    pose.vecPosition[1] = y;
    pose.vecPosition[2] = z;

    // 回転 (クォータニオン)
    pose.qRotation = myQuaternion;

    // 速度・角速度（オプション）
    pose.vecVelocity[0] = vx;
    pose.vecVelocity[1] = vy;
    pose.vecVelocity[2] = vz;
    pose.vecAngularVelocity[0] = wx;
    pose.vecAngularVelocity[1] = wy;
    pose.vecAngularVelocity[2] = wz;

    // 状態
    pose.poseIsValid = true;
    pose.deviceIsConnected = true;
    pose.result = vr::TrackingResult_Running_OK;

    return pose;
}
```

### ポーズの提出

```cpp
// 別スレッドから定期的に呼び出す（5ms間隔など）
vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
    m_deviceIndex,
    GetPose(),
    sizeof(vr::DriverPose_t)
);
```

## 入力システム

### 入力プロファイル JSON (resources/input/<device>_profile.json)

```json
{
  "jsonid": "input_profile",
  "controller_type": "mycontroller",
  "input_bindingui_mode": "controller_handed",
  "input_source": {
    "/input/trigger": {
      "type": "trigger",
      "click": true,
      "value": true,
      "binding_image_point": [250, 60],
      "order": 1
    },
    "/input/joystick": {
      "type": "joystick",
      "click": true,
      "touch": true,
      "binding_image_point": [60, 60],
      "order": 2
    },
    "/input/a": {
      "type": "button",
      "click": true,
      "touch": true,
      "binding_image_point": [80, 60],
      "order": 3
    },
    "/output/haptic": {
      "type": "vibration",
      "binding_image_point": [300, 150],
      "order": 4
    }
  },
  "default_bindings": [
    {
      "app_key": "steam.app.546560",
      "binding_url": "default_bindings/steam.app.546560_mycontroller.json"
    }
  ]
}
```

### 入力タイプと作成方法

| タイプ | 作成関数 | 更新関数 |
|--------|----------|----------|
| Boolean | `CreateBooleanComponent` | `UpdateBooleanComponent` |
| Scalar | `CreateScalarComponent` | `UpdateScalarComponent` |
| Haptic | `CreateHapticComponent` | イベント受信で処理 |
| Skeleton | `CreateSkeletonComponent` | `UpdateSkeletonComponent` |

### 入力コンポーネントの作成と更新

```cpp
// 作成（Activate内で）
vr::VRDriverInput()->CreateBooleanComponent(container, "/input/a/click", &m_aClick);
vr::VRDriverInput()->CreateBooleanComponent(container, "/input/a/touch", &m_aTouch);
vr::VRDriverInput()->CreateScalarComponent(container, "/input/trigger/value", &m_triggerValue,
    vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
vr::VRDriverInput()->CreateScalarComponent(container, "/input/joystick/x", &m_joystickX,
    vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);

// 更新（データ受信時）
vr::VRDriverInput()->UpdateBooleanComponent(m_aClick, isClicked, 0.0);
vr::VRDriverInput()->UpdateScalarComponent(m_triggerValue, triggerValue, 0.0);
```

## イベント処理

```cpp
void MyDeviceProvider::RunFrame()
{
    vr::VREvent_t event;
    while (vr::VRServerDriverHost()->PollNextEvent(&event, sizeof(event)))
    {
        switch (event.eventType)
        {
        case vr::VREvent_Input_HapticVibration:
            // ハプティックフィードバック
            if (event.data.hapticVibration.componentHandle == m_hapticHandle)
            {
                float duration = event.data.hapticVibration.fDurationSeconds;
                float frequency = event.data.hapticVibration.fFrequency;
                float amplitude = event.data.hapticVibration.fAmplitude;
                // ハードウェアに振動を送信
            }
            break;
        }
    }
}
```

## プロパティ設定

### 必須プロパティ（コントローラー）

```cpp
vr::PropertyContainerHandle_t container =
    vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

// 必須
vr::VRProperties()->SetStringProperty(container, vr::Prop_ControllerType_String, "mycontroller");
vr::VRProperties()->SetStringProperty(container, vr::Prop_InputProfilePath_String,
    "{mydriver}/input/mycontroller_profile.json");

// 推奨
vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32,
    vr::TrackedControllerRole_RightHand);  // または TrackedControllerRole_LeftHand
vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, "MyController v1.0");
vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String, "MyCompany");

// バッテリー表示を有効にする場合
vr::VRProperties()->SetBoolProperty(container, vr::Prop_DeviceProvidesBatteryStatus_Bool, true);
vr::VRProperties()->SetFloatProperty(container, vr::Prop_DeviceBatteryPercentage_Float, 0.8f);
vr::VRProperties()->SetBoolProperty(container, vr::Prop_DeviceIsCharging_Bool, false);
```

### HMD固有のプロパティ

```cpp
vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserIpdMeters_Float, 0.063f);
vr::VRProperties()->SetFloatProperty(container, vr::Prop_DisplayFrequency_Float, 90.0f);
vr::VRProperties()->SetFloatProperty(container, vr::Prop_SecondsFromVsyncToPhotons_Float, 0.011f);
vr::VRProperties()->SetBoolProperty(container, vr::Prop_IsOnDesktop_Bool, false);
```

## HMDのディスプレイコンポーネント (IVRDisplayComponent)

HMDデバイスには `GetComponent` で `IVRDisplayComponent` を返す必要があります。

```cpp
void *MyHMD::GetComponent(const char *pchComponentNameAndVersion)
{
    if (strcmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version) == 0)
    {
        return static_cast<vr::IVRDisplayComponent*>(&m_displayComponent);
    }
    return nullptr;
}

class MyDisplayComponent : public vr::IVRDisplayComponent
{
public:
    // ウィンドウの位置とサイズ（拡張モード用）
    void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override
    {
        *pnX = m_windowX;
        *pnY = m_windowY;
        *pnWidth = m_windowWidth;
        *pnHeight = m_windowHeight;
    }

    // デスクトップ拡張かどうか
    bool IsDisplayOnDesktop() override { return false; }

    // 実際のディスプレイかどうか（仮想デバイスならfalse）
    bool IsDisplayRealDisplay() override { return true; }

    // 推奨レンダリング解像度
    void GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight) override
    {
        *pnWidth = m_renderWidth;
        *pnHeight = m_renderHeight;
    }

    // 各目のビューポート
    void GetEyeOutputViewport(vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY,
                              uint32_t *pnWidth, uint32_t *pnHeight) override
    {
        *pnY = 0;
        *pnWidth = m_windowWidth / 2;
        *pnHeight = m_windowHeight;
        *pnX = (eEye == vr::Eye_Left) ? 0 : m_windowWidth / 2;
    }

    // 投影パラメータ（視野角のtan値）
    void GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight,
                          float *pfTop, float *pfBottom) override
    {
        // 90度FOVの例
        *pfLeft = -1.0f;
        *pfRight = 1.0f;
        *pfTop = -1.0f;
        *pfBottom = 1.0f;
    }

    // レンズ歪み補正
    vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU, float fV) override
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

## 設定ファイル

### driver.vrdrivermanifest

```json
{
  "alwaysActivate": false,
  "name": "mydriver",
  "directory": "",
  "resourceOnly": false,
  "hmd_presence": ["28DE.*"]
}
```

### resources/settings/default.vrsettings

```json
{
  "driver_mydriver": {
    "enable": true,
    "loadPriority": 0,
    "blocked_by_safe_mode": false,
    "custom_setting": "default_value"
  }
}
```

### resources/driver.vrresources

```json
{
  "jsonid": "vrresources",
  "statusicons": {
    "Controller": {
      "Prop_NamedIconPathDeviceOff_String": "{mydriver}/icons/controller_off.png",
      "Prop_NamedIconPathDeviceSearching_String": "{mydriver}/icons/controller_searching.gif",
      "Prop_NamedIconPathDeviceReady_String": "{mydriver}/icons/controller_ready.png",
      "Prop_NamedIconPathDeviceNotReady_String": "{mydriver}/icons/controller_error.png",
      "Prop_NamedIconPathDeviceStandby_String": "{mydriver}/icons/controller_standby.png",
      "Prop_NamedIconPathDeviceAlertLow_String": "{mydriver}/icons/controller_low.png"
    }
  }
}
```

## ビルド設定 (CMake)

```cmake
cmake_minimum_required(VERSION 3.7.1)
set(TARGET_NAME mydriver)
set(DRIVER_NAME "driver_${TARGET_NAME}")
project(${TARGET_NAME})

set(OPENVR_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/openvr)

# プラットフォーム設定
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DLINUX -DPOSIX)
    set(ARCH_TARGET linux64)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-D_WIN32)
    set(ARCH_TARGET win64)
endif()

find_library(OPENVR_LIBRARIES
    NAMES openvr_api
    PATHS ${OPENVR_LIB_DIR}/lib
    PATH_SUFFIXES ${ARCH_TARGET}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    $<1:${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/bin/${ARCH_TARGET}>)

add_library(${DRIVER_NAME} SHARED
    src/hmd_driver_factory.cpp
    src/device_provider.cpp
    src/my_device.cpp
)

target_link_libraries(${DRIVER_NAME} PRIVATE ${OPENVR_LIBRARIES})
target_include_directories(${DRIVER_NAME} PRIVATE ${OPENVR_LIB_DIR}/headers)
```

## SteamVRへのドライバー登録

```bash
# Windows
"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrpathreg.exe" adddriver "<path_to_driver>"

# Linux
~/.steam/steam/steamapps/common/SteamVR/bin/linux64/vrpathreg adddriver "<path_to_driver>"
```

## デバッグ

1. **Web Console**: SteamVR メニュー → Developer → Web Console
2. **ログファイル**: `C:\Program Files (x86)\Steam\logs\vrserver.txt`
3. **Visual Studio**: Child Process Debugging Power Tool拡張を使用してvrserver.exeにアタッチ

## トラブルシューティング

| 問題 | 原因 | 解決策 |
|------|------|--------|
| ドライバーが読み込まれない | 命名規則違反 | `driver_<name>.dll`の命名規則を確認 |
| デバイスが表示されない | ポーズが無効 | `qRotation.w = 1.0f`を設定 |
| アイコンが点滅 | `poseIsValid = false` | ポーズ更新を確認 |
| 入力が反映されない | 入力プロファイルのパスエラー | `Prop_InputProfilePath_String`を確認 |

## コントローラーロール

| ロール | 説明 |
|--------|------|
| `TrackedControllerRole_LeftHand` | 左手コントローラー |
| `TrackedControllerRole_RightHand` | 右手コントローラー |
| `TrackedControllerRole_OptOut` | 左右選択から除外 |
| `TrackedControllerRole_Treadmill` | トレッドミル |
| `TrackedControllerRole_Stylus` | スタイラス |

## トラッカーロール（ユーザー設定）

- `TrackerRole_LeftFoot` / `TrackerRole_RightFoot`
- `TrackerRole_LeftKnee` / `TrackerRole_RightKnee`
- `TrackerRole_Waist`
- `TrackerRole_Chest`
- `TrackerRole_LeftElbow` / `TrackerRole_RightElbow`
- `TrackerRole_LeftShoulder` / `TrackerRole_RightShoulder`

## 座標系

OpenVRは右手座標系を使用：
- **+Y**: 上
- **+X**: 右
- **-Z**: 前方
- 単位: **メートル**

## 推奨開発フロー

1. `samples/drivers/drivers/barebones/` をコピーして開始
2. `tutorial/README.md` を参照しながら機能追加
3. `simplehmd/`, `simplecontroller/`, `simpletrackers/` を参考に実装
4. `handskeletonsimulation/` でスケレタル入力を学習

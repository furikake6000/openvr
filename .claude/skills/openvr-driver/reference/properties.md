# プロパティ設定

## 目次
- プロパティの設定方法
- 共通プロパティ
- コントローラー固有
- HMD固有
- トラッカー固有
- アイコン設定

---

## プロパティの設定方法

```cpp
vr::PropertyContainerHandle_t container =
    vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

// 文字列
vr::VRProperties()->SetStringProperty(container,
    vr::Prop_ModelNumber_String, "MyModel");

// 整数
vr::VRProperties()->SetInt32Property(container,
    vr::Prop_ControllerRoleHint_Int32, vr::TrackedControllerRole_RightHand);

// 浮動小数点
vr::VRProperties()->SetFloatProperty(container,
    vr::Prop_DeviceBatteryPercentage_Float, 0.8f);

// ブール
vr::VRProperties()->SetBoolProperty(container,
    vr::Prop_DeviceProvidesBatteryStatus_Bool, true);
```

---

## 共通プロパティ

| プロパティ | 型 | 説明 |
|-----------|-----|------|
| `Prop_ModelNumber_String` | String | モデル番号 |
| `Prop_SerialNumber_String` | String | シリアル番号 |
| `Prop_ManufacturerName_String` | String | メーカー名 |
| `Prop_TrackingFirmwareVersion_String` | String | FWバージョン |
| `Prop_HardwareRevision_String` | String | HWリビジョン |

### バッテリー

```cpp
vr::VRProperties()->SetBoolProperty(container,
    vr::Prop_DeviceProvidesBatteryStatus_Bool, true);
vr::VRProperties()->SetFloatProperty(container,
    vr::Prop_DeviceBatteryPercentage_Float, 0.8f);  // 0.0〜1.0
vr::VRProperties()->SetBoolProperty(container,
    vr::Prop_DeviceIsCharging_Bool, false);
```

---

## コントローラー固有

```cpp
// コントローラータイプ（必須）
vr::VRProperties()->SetStringProperty(container,
    vr::Prop_ControllerType_String, "mycontroller");

// 入力プロファイル（必須）
vr::VRProperties()->SetStringProperty(container,
    vr::Prop_InputProfilePath_String,
    "{mydriver}/input/mycontroller_profile.json");

// ロールヒント
vr::VRProperties()->SetInt32Property(container,
    vr::Prop_ControllerRoleHint_Int32,
    vr::TrackedControllerRole_RightHand);
```

### コントローラーロール

| ロール | 説明 |
|--------|------|
| `TrackedControllerRole_LeftHand` | 左手 |
| `TrackedControllerRole_RightHand` | 右手 |
| `TrackedControllerRole_OptOut` | 左右選択から除外 |
| `TrackedControllerRole_Treadmill` | トレッドミル |
| `TrackedControllerRole_Stylus` | スタイラス |

---

## HMD固有

```cpp
// IPD
vr::VRProperties()->SetFloatProperty(container,
    vr::Prop_UserIpdMeters_Float, 0.063f);

// リフレッシュレート
vr::VRProperties()->SetFloatProperty(container,
    vr::Prop_DisplayFrequency_Float, 90.0f);

// Vsyncからフォトン表示までの時間
vr::VRProperties()->SetFloatProperty(container,
    vr::Prop_SecondsFromVsyncToPhotons_Float, 0.011f);

// デスクトップ拡張モードでない
vr::VRProperties()->SetBoolProperty(container,
    vr::Prop_IsOnDesktop_Bool, false);
```

---

## トラッカー固有

トラッカーロールは**ユーザーがSteamVR設定で割り当て**。
ドライバーでは設定不要。

### ユーザー設定可能なロール

- `TrackerRole_LeftFoot` / `TrackerRole_RightFoot`
- `TrackerRole_LeftKnee` / `TrackerRole_RightKnee`
- `TrackerRole_Waist`
- `TrackerRole_Chest`
- `TrackerRole_LeftElbow` / `TrackerRole_RightElbow`
- `TrackerRole_LeftShoulder` / `TrackerRole_RightShoulder`

---

## アイコン設定

`driver.vrresources` で設定するか、プロパティで直接設定：

```cpp
vr::VRProperties()->SetStringProperty(container,
    vr::Prop_NamedIconPathDeviceReady_String,
    "{mydriver}/icons/controller_ready.png");

vr::VRProperties()->SetStringProperty(container,
    vr::Prop_NamedIconPathDeviceOff_String,
    "{mydriver}/icons/controller_off.png");

vr::VRProperties()->SetStringProperty(container,
    vr::Prop_NamedIconPathDeviceSearching_String,
    "{mydriver}/icons/controller_searching.gif");
```

### アイコンプロパティ一覧

| プロパティ | 状態 |
|-----------|------|
| `Prop_NamedIconPathDeviceOff_String` | オフ |
| `Prop_NamedIconPathDeviceSearching_String` | 検索中 |
| `Prop_NamedIconPathDeviceSearchingAlert_String` | 検索中（警告） |
| `Prop_NamedIconPathDeviceReady_String` | 準備完了 |
| `Prop_NamedIconPathDeviceReadyAlert_String` | 準備完了（警告） |
| `Prop_NamedIconPathDeviceNotReady_String` | エラー |
| `Prop_NamedIconPathDeviceStandby_String` | スタンバイ |
| `Prop_NamedIconPathDeviceAlertLow_String` | バッテリー低下 |

### アイコンサイズ

- HMD: 50x32 または 100x64（@2x）
- その他: 32x32 または 64x64（@2x）
- 形式: PNG または GIF

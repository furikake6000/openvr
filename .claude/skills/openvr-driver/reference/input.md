# 入力システム

## 目次
- 入力プロファイルJSON
- 入力コンポーネントの作成
- 入力コンポーネントの更新
- ハプティックフィードバック
- イベント処理

---

## 入力プロファイルJSON

`<driver>/resources/input/<device>_profile.json` に配置。

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
      "binding_image_point": [200, 100],
      "order": 1
    },
    "/input/joystick": {
      "type": "joystick",
      "click": true,
      "touch": true,
      "binding_image_point": [100, 50],
      "order": 2
    },
    "/input/a": {
      "type": "button",
      "click": true,
      "touch": true,
      "binding_image_point": [150, 80],
      "order": 3
    },
    "/output/haptic": {
      "type": "vibration",
      "binding_image_point": [250, 150],
      "order": 4
    }
  }
}
```

### 入力タイプ

| type | 説明 | 自動生成コンポーネント |
|------|------|----------------------|
| `button` | ボタン | click, touch |
| `trigger` | トリガー | value, click |
| `joystick` | ジョイスティック | x, y, click, touch |
| `trackpad` | タッチパッド | x, y, click, touch, force |
| `skeleton` | スケレタル入力 | - |
| `vibration` | ハプティック | - |

---

## 入力コンポーネントの作成

`Activate()` 内で作成：

```cpp
vr::EVRInitError Activate(uint32_t unObjectId)
{
    vr::PropertyContainerHandle_t container =
        vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

    // 入力プロファイルパスを設定（必須）
    vr::VRProperties()->SetStringProperty(container,
        vr::Prop_InputProfilePath_String,
        "{mydriver}/input/mycontroller_profile.json");

    // Boolean（クリック、タッチ）
    vr::VRDriverInput()->CreateBooleanComponent(container,
        "/input/a/click", &m_aClick);
    vr::VRDriverInput()->CreateBooleanComponent(container,
        "/input/a/touch", &m_aTouch);

    // Scalar（トリガー値: 0.0〜1.0）
    vr::VRDriverInput()->CreateScalarComponent(container,
        "/input/trigger/value", &m_triggerValue,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedOneSided);

    // Scalar（ジョイスティック: -1.0〜1.0）
    vr::VRDriverInput()->CreateScalarComponent(container,
        "/input/joystick/x", &m_joystickX,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided);

    // Haptic
    vr::VRDriverInput()->CreateHapticComponent(container,
        "/output/haptic", &m_haptic);

    return vr::VRInitError_None;
}
```

### スカラータイプ

| タイプ | 説明 |
|--------|------|
| `VRScalarType_Absolute` | 絶対値（ジョイスティック、トリガー） |
| `VRScalarType_Relative` | 相対値（マウス、トラックボール） |

### スカラー単位

| 単位 | 範囲 | 用途 |
|------|------|------|
| `VRScalarUnits_NormalizedOneSided` | 0〜1 | トリガー |
| `VRScalarUnits_NormalizedTwoSided` | -1〜1 | ジョイスティック |

---

## 入力コンポーネントの更新

データ受信時に呼び出し：

```cpp
void UpdateInputs(const DeviceData &data)
{
    // Boolean
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_aClick, data.aPressed, 0.0);

    // Scalar
    vr::VRDriverInput()->UpdateScalarComponent(
        m_triggerValue, data.trigger, 0.0);

    vr::VRDriverInput()->UpdateScalarComponent(
        m_joystickX, data.joystickX, 0.0);
}
```

第3引数 `fTimeOffset`: 状態変化の時刻オフセット（通常0.0）

---

## ハプティックフィードバック

### イベント受信

`RunFrame()` でイベントをポーリング：

```cpp
void MyDeviceProvider::RunFrame()
{
    vr::VREvent_t event;
    while (vr::VRServerDriverHost()->PollNextEvent(&event, sizeof(event)))
    {
        if (event.eventType == vr::VREvent_Input_HapticVibration)
        {
            for (auto &device : m_devices)
            {
                device->HandleHaptic(event);
            }
        }
    }
}
```

### ハプティック処理

```cpp
void MyDevice::HandleHaptic(const vr::VREvent_t &event)
{
    if (event.data.hapticVibration.componentHandle != m_haptic)
        return;

    float duration = event.data.hapticVibration.fDurationSeconds;
    float frequency = event.data.hapticVibration.fFrequency;
    float amplitude = event.data.hapticVibration.fAmplitude;

    // duration: 振動時間（秒）、0ならパルス1回
    // frequency: 周波数（Hz）、低いほど「ゴロゴロ」感
    // amplitude: 強度

    if (frequency > 0 && amplitude > 0)
    {
        // ハードウェアに振動を送信
    }
}
```

---

## 予約済み入力パス

| パス | 説明 |
|------|------|
| `/input/system/click` | SteamVRダッシュボード呼び出し |
| `/proximity` | HMD装着検出 |

これらはアプリケーションにはバインドされません。

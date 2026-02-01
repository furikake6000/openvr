# コントローラーエミュレーション

## 概要

左右2つのVRコントローラーをエミュレートし、キーボード/マウス入力をVR入力として送信します。

## 入力コンポーネント

OpenVRの入力パスとタイプ：

| 入力 | OpenVRパス | タイプ |
|-----|-----------|-------|
| A/B/X/Yボタン | `/input/a/click`, `/input/b/click`, etc. | Boolean |
| ジョイスティック | `/input/joystick/x`, `/input/joystick/y` | Scalar (-1~+1) |
| ジョイスティック押込 | `/input/joystick/click` | Boolean |
| トリガー | `/input/trigger/value` | Scalar (0~1) |
| グリップ | `/input/grip/value` | Scalar (0~1) |
| システム | `/input/system/click` | Boolean |
| メニュー | `/input/application_menu/click` | Boolean |
| ハプティクス | `/output/haptic` | Vibration |

## デフォルトキーマッピング

### 左コントローラー

| 入力 | キー |
|-----|------|
| スティック移動 | W/A/S/D |
| スティック押込 | Space |
| トリガー | 左クリック |
| グリップ | 左Shift |
| A | Q |
| B | E |
| X | 1 |
| Y | 2 |
| バンパー | R |
| システム | Escape |
| メニュー | Tab |

### 右コントローラー

| 入力 | キー |
|-----|------|
| スティック移動 | Arrow Keys |
| スティック押込 | L |
| トリガー | 右クリック |
| グリップ | 右Shift |
| A | J |
| B | K |
| X | U |
| Y | I |
| バンパー | O |

## ポーズ（位置・姿勢）

コントローラーはHMDを基準とした固定位置に配置されます：

```cpp
// HMD基準のオフセット
左コントローラー: (-0.2m, -0.2m, -0.5m)  // 左下前方
右コントローラー: (+0.2m, -0.2m, -0.5m)  // 右下前方
```

コントローラーの姿勢はHMDに追従し、自然な持ち位置を維持します。

## 入力状態構造体

```cpp
// common/input_state.h
struct ControllerInputState {
    uint16_t buttons = 0;      // ボタンビットフィールド
    float stick_x = 0.0f;      // ジョイスティックX (-1~+1)
    float stick_y = 0.0f;      // ジョイスティックY (-1~+1)
    float trigger = 0.0f;      // トリガー (0~1)
    float grip = 0.0f;         // グリップ (0~1)
};

// ボタンビットマップ
enum ButtonFlags : uint16_t {
    BTN_A      = 1 << 0,
    BTN_B      = 1 << 1,
    BTN_X      = 1 << 2,
    BTN_Y      = 1 << 3,
    BTN_BUMPER = 1 << 4,
    BTN_STICK  = 1 << 5,  // ジョイスティック押し込み
    BTN_SYSTEM = 1 << 6,
    BTN_MENU   = 1 << 7,
};
```

## ドライバー実装

### EmuControllerDriver

`ITrackedDeviceServerDriver`を実装し、各コントローラーを表現します。

```cpp
class EmuControllerDriver : public vr::ITrackedDeviceServerDriver {
public:
    EmuControllerDriver(vr::ETrackedControllerRole role);

    // デバイス情報
    vr::EVRInitError Activate(uint32_t objectId) override;
    void Deactivate() override;
    vr::DriverPose_t GetPose() override;

    // 入力更新
    void UpdateInputState(const ControllerInputState& state);
    void RunFrame();

private:
    vr::ETrackedControllerRole role_;  // Left or Right
    vr::VRInputComponentHandle_t input_handles_[...];
};
```

### 入力プロファイル

`resources/input/emucontroller_profile.json`でSteamVRに入力構成を通知します。

```json
{
    "jsonid": "input_profile",
    "controller_type": "emucontroller",
    "input_source": [
        { "path": "/input/a", "type": "button", "click": true },
        { "path": "/input/joystick", "type": "joystick", "click": true },
        { "path": "/input/trigger", "type": "trigger" },
        ...
    ]
}
```

## GUI表示

GUIアプリケーションでは以下を表示：

1. **接続状態**: ドライバーとの接続状況
2. **入力可視化**: 各ボタン・スティック・トリガーの現在値
3. **キーバインド表示**: 現在のキーマッピング

```
┌─────────────────────────────────────────┐
│           Left Controller               │
│  ┌───────┐         [A][B]              │
│  │   ●   │ Stick   [X][Y]              │
│  │  →    │ (0.5,0) [Bumper]            │
│  └───────┘                              │
│  Trigger: ████████░░ 0.80               │
│  Grip:    ██░░░░░░░░ 0.20               │
└─────────────────────────────────────────┘
```

## 制限事項

- ハプティクスフィードバックは現在未実装
- キーマッピングのカスタマイズUIは未実装
- スティックのデッドゾーン設定は未実装

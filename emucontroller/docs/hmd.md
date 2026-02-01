# HMDエミュレーション

## 概要

物理的なVRヘッドセットなしでSteamVRコンテンツを表示できるHMDエミュレータです。VR映像はデスクトップウィンドウ（HEADSET WINDOW）に表示され、マウス/キーボードで視点を制御できます。

## 表示モード

### Extended Mode（現在の実装）

SteamVRコンポジターがデスクトップウィンドウに直接レンダリングします。

```cpp
// hmd_display_component.cpp
bool EmuHMDDisplayComponent::IsDisplayOnDesktop() {
    return true;  // デスクトップ上に表示
}

bool EmuHMDDisplayComponent::IsDisplayRealDisplay() {
    return false; // 仮想ディスプレイ
}
```

### 表示構成

```
┌──────────────────────────────────────┐
│     HEADSET WINDOW (1920 x 1080)     │
│ ┌────────────────┬────────────────┐  │
│ │   Left Eye     │   Right Eye    │  │
│ │   (960x1080)   │   (960x1080)   │  │
│ │                │                │  │
│ │                │                │  │
│ └────────────────┴────────────────┘  │
└──────────────────────────────────────┘
```

## 表示設定

`resources/settings/default.vrsettings`で設定可能：

```json
{
    "emucontroller_hmd": {
        "window_x": 100,
        "window_y": 100,
        "window_width": 1920,
        "window_height": 1080,
        "render_width": 1512,
        "render_height": 1680
    }
}
```

| 項目 | デフォルト | 説明 |
|-----|----------|------|
| window_x | 100 | ウィンドウX座標 |
| window_y | 100 | ウィンドウY座標 |
| window_width | 1920 | ウィンドウ幅 |
| window_height | 1080 | ウィンドウ高さ |
| render_width | 1512 | レンダリング幅（片目） |
| render_height | 1680 | レンダリング高さ |

## 視点操作

### 操作方法

| 入力 | アクション |
|-----|----------|
| マウス右クリック + 移動 | Yaw/Pitch回転（見回し） |
| W/S | 前後移動 |
| A/D | 左右移動 |
| Q/E | 上下移動 |
| Shift | 高速移動（2倍） |
| Ctrl | 低速移動（0.25倍） |
| R | 位置リセット（原点に戻る） |

### 移動パラメータ

```cpp
// common/input_state.h
struct HMDSettings {
    float move_speed = 2.0f;        // 通常移動速度 (m/s)
    float fast_multiplier = 2.0f;   // Shift時の倍率
    float slow_multiplier = 0.25f;  // Ctrl時の倍率
    float mouse_sensitivity = 0.002f; // マウス感度 (rad/pixel)
    float min_pitch = -1.5f;        // 下向き限界 (rad)
    float max_pitch = 1.5f;         // 上向き限界 (rad)
};
```

## ポーズ構造体

```cpp
// common/input_state.h
struct HMDPose {
    float position[3] = { 0.0f, 1.6f, 0.0f };  // x, y, z (メートル)
    float yaw = 0.0f;    // 左右回転 (ラジアン)
    float pitch = 0.0f;  // 上下回転 (ラジアン)
    float roll = 0.0f;   // 傾き (ラジアン)

    void Reset() {
        position[0] = 0.0f;
        position[1] = 1.6f;  // 立位の目の高さ
        position[2] = 0.0f;
        yaw = pitch = roll = 0.0f;
    }
};
```

初期位置は床から1.6m（立位の目の高さ）です。

## ドライバー実装

### EmuHMDDriver

`ITrackedDeviceServerDriver`を実装し、HMDデバイスを表現します。

```cpp
class EmuHMDDriver : public vr::ITrackedDeviceServerDriver {
public:
    vr::EVRInitError Activate(uint32_t objectId) override;
    void Deactivate() override;
    void* GetComponent(const char* component) override;
    vr::DriverPose_t GetPose() override;

    void UpdatePose(const HMDPoseData& pose);
    void RunFrame();

private:
    std::unique_ptr<EmuHMDDisplayComponent> display_component_;
    HMDPoseData current_pose_;
};
```

### EmuHMDDisplayComponent

`IVRDisplayComponent`を実装し、ディスプレイ出力を制御します。

```cpp
class EmuHMDDisplayComponent : public vr::IVRDisplayComponent {
public:
    void GetWindowBounds(int32_t* x, int32_t* y,
                         uint32_t* width, uint32_t* height) override;
    void GetRecommendedRenderTargetSize(uint32_t* width,
                                        uint32_t* height) override;
    void GetEyeOutputViewport(vr::EVREye eye,
                              uint32_t* x, uint32_t* y,
                              uint32_t* width, uint32_t* height) override;
    void GetProjectionRaw(vr::EVREye eye,
                          float* left, float* right,
                          float* top, float* bottom) override;
    vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eye,
                                                   float u, float v) override;
    bool IsDisplayOnDesktop() override;
    bool IsDisplayRealDisplay() override;
};
```

## GUI表示

GUIアプリケーションではHMD操作パネルを表示：

```
┌─────────────────────────────────────────┐
│              HMD Control                │
├─────────────────────────────────────────┤
│  Position: X: 0.00  Y: 1.60  Z: 0.00   │
│  Rotation: Yaw: 0°  Pitch: 0°  Roll: 0°│
│                                         │
│  Top View:      Side View:              │
│  ┌─────────┐    ┌─────────┐            │
│  │    ▲    │    │    ●    │            │
│  │    │    │    │   /     │            │
│  │    ●    │    │  /      │            │
│  └─────────┘    └─────────┘            │
│                                         │
│  [Right-click to enable mouse look]     │
└─────────────────────────────────────────┘
```

## 今後の拡張予定

### Direct Mode対応

現在のExtended Modeから、Direct Modeへの移行を検討中。Direct Modeでは：

- ドライバーがテクスチャを直接受け取り可能
- 外部デバイスへの映像転送に適している
- `IVRDriverDirectModeComponent`の実装が必要

## 制限事項

- **Direct Mode未対応**: Extended Mode（デスクトップウィンドウ）のみ
- **歪み補正なし**: エミュレータのためレンズ歪みは適用しない
- **低遅延は保証しない**: デバッグ/開発用途のため
- **物理HMDとの競合**: 同時に1つのHMDのみ使用可能

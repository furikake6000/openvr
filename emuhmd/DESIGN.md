# EmuHMD - OpenVR HMDエミュレータ 設計計画書

## 概要

物理的なVRヘッドセットなしでSteamVRコンテンツを表示・操作できるHMDエミュレータ。
デスクトップウィンドウにVR映像を表示し、マウス/キーボードで視点を制御する。

### 目的
- EmuControllerと組み合わせて、VRアプリケーションの動作確認・デバッグを可能にする
- 物理HMDがなくてもVR開発・テストができる環境を提供

### 決定事項
- **表示方式**: デスクトップウィンドウ（Extended Mode）
- **視点操作**: マウスで回転、キーボードで移動
- **統合方針**: EmuControllerと同じドライバーに統合（IPC共有）

---

## アーキテクチャ

```
┌─────────────────────────┐     Named Pipe (IPC)     ┌─────────────────────────┐
│    GUI Application      │ ─────────────────────→   │    OpenVR Driver        │
│    (EmuController.exe)  │ ←─────────────────────   │    (driver_emucontroller.dll)│
├─────────────────────────┤                          ├─────────────────────────┤
│ - HMD視点操作UI         │                          │ - ITrackedDeviceServerDriver │
│ - マウス/キーボード入力  │                          │ - IVRDisplayComponent        │
│ - 接続状態表示          │                          │ - 左右コントローラー         │
└─────────────────────────┘                          └─────────────────────────┘
         │                                                      │
         v                                                      v
   Windows API                                           SteamVR Runtime
   (GetAsyncKeyState, Mouse)                            (vrserver + vrcompositor)
                                                                │
                                                                v
                                                        ┌─────────────────┐
                                                        │  Display Window │
                                                        │  (VR映像表示)    │
                                                        └─────────────────┘
```

---

## ディスプレイ設計

### 表示モード: Extended Mode
- SteamVRコンポジターがデスクトップウィンドウに直接レンダリング
- `IsDisplayRealDisplay() = false` で仮想ディスプレイとして動作
- `IsDisplayOnDesktop() = true` でデスクトップ上に表示

### 表示設定
| 項目 | デフォルト値 | 説明 |
|-----|------------|------|
| window_x | 100 | ウィンドウX座標 |
| window_y | 100 | ウィンドウY座標 |
| window_width | 1920 | ウィンドウ幅 |
| window_height | 1080 | ウィンドウ高さ |
| render_width | 1512 | レンダリング幅（片目） |
| render_height | 1680 | レンダリング高さ |
| display_frequency | 60 | リフレッシュレート |

### レンダリング構成
```
┌──────────────────────────────────────┐
│         Window (1920 x 1080)         │
│ ┌────────────────┬────────────────┐  │
│ │   Left Eye     │   Right Eye    │  │
│ │   (960x1080)   │   (960x1080)   │  │
│ │                │                │  │
│ │                │                │  │
│ └────────────────┴────────────────┘  │
└──────────────────────────────────────┘
```

---

## HMDポーズ制御

### 操作方式
| 入力 | アクション |
|-----|----------|
| マウス移動 | Yaw/Pitch回転（見回し） |
| W/S | 前後移動 |
| A/D | 左右移動 |
| Q/E | 上下移動 |
| Shift | 高速移動 |
| Ctrl | 低速移動 |
| R | 位置リセット（原点に戻る） |
| マウス右クリック | マウスルック有効/無効切替 |

### ポーズ計算
```cpp
struct HMDPose {
    float position[3];    // x, y, z (メートル)
    float rotation[3];    // yaw, pitch, roll (ラジアン)
};

// 初期位置: 床から1.6m（立位）
// 移動速度: 2.0 m/s（通常）、4.0 m/s（Shift）、0.5 m/s（Ctrl）
// 回転感度: 0.002 rad/pixel
```

---

## IPC拡張

### 新規メッセージタイプ
```cpp
// ipc_protocol.h に追加
enum MessageType : uint16_t {
    MSG_CONNECT      = 0x0001,
    MSG_DISCONNECT   = 0x0002,
    MSG_INPUT_STATE  = 0x0003,
    MSG_STATUS       = 0x0004,
    MSG_HMD_POSE     = 0x0010,  // NEW: HMDポーズ更新
    MSG_HMD_CONFIG   = 0x0011,  // NEW: HMD設定
};

// HMDポーズメッセージ
struct HMDPoseMessage {
    MessageHeader header;
    float position[3];      // x, y, z
    float rotation[4];      // quaternion (w, x, y, z)
    uint64_t timestamp;
};
```

---

## フォルダ構成

```
emucontroller/              # 既存のコントローラーエミュレータ
├── common/
│   ├── ipc_protocol.h      # HMDメッセージ追加
│   └── input_state.h
├── driver/
│   ├── src/
│   │   ├── hmd_device_driver.h/cpp   # NEW: HMDデバイス
│   │   ├── hmd_display_component.h/cpp # NEW: ディスプレイ
│   │   ├── device_provider.cpp       # HMD追加
│   │   └── ...
│   └── emucontroller/
│       └── resources/
│           ├── settings/default.vrsettings  # HMD設定追加
│           └── ...
└── app/
    └── src/
        ├── main.cpp          # HMD操作UI追加
        └── hmd_controller.h/cpp  # NEW: HMD操作ロジック

emuhmd/                     # 設計書のみ（実装はemucontrollerに統合）
└── DESIGN.md
```

---

## 実装ステップ

### Phase 1: 設計確認
1. DESIGN.md作成 ← 現在
2. ユーザー確認

### Phase 2: モックアプリ
3. EmuControllerにHMD操作パネル追加
4. マウス/キーボードでのポーズ操作
5. ポーズ値の表示

### Phase 3: ドライバー実装
6. `hmd_device_driver.h/cpp` 作成
7. `hmd_display_component.h/cpp` 作成
8. `device_provider.cpp` にHMDデバイス追加
9. IPC拡張（MSG_HMD_POSE）

### Phase 4: 統合テスト
10. SteamVRでHMD認識確認
11. VR映像がウィンドウに表示されることを確認
12. ポーズ操作がVR空間に反映されることを確認

---

## 参照ファイル

| 用途 | パス |
|-----|-----|
| HMDドライバー例 | samples/drivers/drivers/simplehmd/src/hmd_device_driver.cpp |
| ディスプレイコンポーネント例 | samples/drivers/drivers/simplehmd/src/hmd_device_driver.h |
| 設定ファイル例 | samples/drivers/drivers/simplehmd/simplehmd/resources/settings/default.vrsettings |
| OpenVR Driver API | headers/openvr_driver.h |

---

## 制限事項・注意点

1. **Direct Modeには対応しない**: Extended Mode（デスクトップウィンドウ）のみ
2. **歪み補正なし**: エミュレータなのでレンズ歪みは適用しない
3. **低遅延は保証しない**: デバッグ/開発用途のため
4. **同時起動**: 物理HMDがある場合は競合する可能性あり

---

## 検証方法

1. **ドライバー認識**: SteamVR設定でEmuHMDが表示されること
2. **ウィンドウ表示**: VR映像がデスクトップウィンドウに表示されること
3. **視点操作**: マウス/キーボードでVR空間内を移動・見回しできること
4. **コントローラー連携**: EmuControllerと同時に動作すること
5. **VRアプリテスト**: SteamVR Homeなどで正常に動作すること

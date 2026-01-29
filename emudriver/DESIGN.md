# OpenVR コントローラーエミュレータ 設計計画書

## 概要

Windows GUIアプリケーションとOpenVRドライバーのセットで、キーボード/マウス入力を仮想コントローラーとしてSteamVRに送信するシステムを構築する。

### 決定事項
- **コントローラー数**: 左右両方（2本）をエミュレート
- **ポーズ方式**: HMD基準の固定位置（頭の前方に配置）

---

## アーキテクチャ

```
┌─────────────────────────┐     Named Pipe (IPC)     ┌─────────────────────────┐
│    GUI Application      │ ─────────────────────→   │    OpenVR Driver        │
│    (EmuController.exe)  │ ←─────────────────────   │    (driver_emucontroller.dll)│
├─────────────────────────┤                          ├─────────────────────────┤
│ - ImGui + DirectX 11    │                          │ - IServerTrackedDeviceProvider│
│ - キー/マウス入力監視    │                          │ - ITrackedDeviceServerDriver │
│ - 入力状態・接続状態表示 │                          │ - 左右コントローラーデバイス │
└─────────────────────────┘                          └─────────────────────────┘
         │                                                      │
         v                                                      v
   Windows API                                           SteamVR Runtime
   (GetAsyncKeyState)                                    (vrserver.exe)
```

---

## IPC通信方式

### Named Pipe
- **パイプ名**: `\\.\pipe\EmuControllerPipe`
- **理由**: 双方向通信、接続状態検知、低レイテンシ、実装が適度

### 通信フォーマット（バイナリ）

```cpp
// メッセージヘッダ (4 bytes)
struct MessageHeader {
    uint16_t type;    // MSG_INPUT_STATE=0x0001, MSG_CONNECT=0x0002, etc.
    uint16_t length;  // ペイロード長
};

// 入力状態 (28 bytes)
struct ControllerInputState {
    uint16_t buttons;        // ビットフィールド (A,B,X,Y,LB,RB,Back,Start,L3,R3)
    int8_t dpad_x, dpad_y;   // 十字キー (-1, 0, +1)
    int16_t left_stick_x, left_stick_y;   // 左スティック
    int16_t right_stick_x, right_stick_y; // 右スティック
    uint8_t left_trigger, right_trigger;  // トリガー (0-255)
    uint64_t timestamp;
    uint8_t reserved[4];
};

// ボタンビットマップ
enum ButtonFlags : uint16_t {
    BTN_A      = 1 << 0,
    BTN_B      = 1 << 1,
    BTN_X      = 1 << 2,
    BTN_Y      = 1 << 3,
    BTN_LB     = 1 << 4,
    BTN_RB     = 1 << 5,
    BTN_BACK   = 1 << 6,
    BTN_START  = 1 << 7,
    BTN_L3     = 1 << 8,  // 左スティック押し込み
    BTN_R3     = 1 << 9,  // 右スティック押し込み
};
```

---

## 入力コンポーネント定義

| 入力 | OpenVRパス | タイプ |
|-----|-----------|-------|
| A/B/X/Yボタン | `/input/a/click`, `/input/b/click`, etc. | Boolean |
| 十字キー | `/input/dpad_up`, `/input/dpad_down`, etc. | Boolean |
| ジョイスティック | `/input/joystick/x`, `/input/joystick/y` | Scalar (-1~+1) |
| ジョイスティック押込 | `/input/joystick/click` | Boolean |
| トリガー | `/input/trigger/value` | Scalar (0~1) |
| グリップ | `/input/grip/value` | Scalar (0~1) |
| システム/メニュー | `/input/system/click`, `/input/application_menu/click` | Boolean |
| ハプティクス | `/output/haptic` | Vibration |

---

## デフォルトキーマッピング

| 入力 | 左コントローラー | 右コントローラー |
|-----|-----------------|-----------------|
| スティック移動 | W/A/S/D | マウス移動 |
| スティック押込 | Space | 左クリック |
| トリガー | Left Shift | 右クリック |
| グリップ | Left Ctrl | 中クリック |
| A | Q | J |
| B | E | K |
| X | 1 | U |
| Y | 2 | I |
| システム | Escape | - |
| メニュー | Tab | - |

---

## フォルダ構成

```
emudriver/
├── DESIGN.md              # この設計書
├── CMakeLists.txt         # ルートCMake
├── common/
│   ├── ipc_protocol.h     # IPCメッセージ定義
│   └── input_state.h      # 入力状態構造体
├── driver/
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── hmd_driver_factory.cpp
│   │   ├── device_provider.h/cpp
│   │   ├── controller_driver.h/cpp
│   │   └── ipc_server.h/cpp
│   └── emucontroller/
│       ├── driver.vrdrivermanifest
│       └── resources/
│           ├── input/emucontroller_profile.json
│           └── settings/default.vrsettings
└── app/
    ├── CMakeLists.txt
    └── src/
        ├── main.cpp
        ├── application.h/cpp
        ├── input_manager.h/cpp
        ├── ipc_client.h/cpp
        └── ui_controller.h/cpp
```

---

## 実装ステップ

### Phase 1: GUIモック（ドライバー接続なし）
1. `app/` ディレクトリ構造作成
2. CMakeLists.txt でImGui + DirectX 11環境構築
3. 基本ウィンドウ表示
4. キーボード/マウス入力取得と画面表示
5. ビルド確認・動作テスト

### Phase 2: ドライバー実装
6. `driver/` ディレクトリ構造作成
7. simplecontrollerをベースにドライバー骨格作成
8. 入力プロファイルJSON作成
9. SteamVRでの認識確認

### Phase 3: IPC通信統合
10. `common/` で共通プロトコル定義
11. ドライバー側: Named Pipeサーバー実装
12. GUI側: Named Pipeクライアント実装
13. 統合テスト

### Phase 4: 機能完成
14. 入力値のドライバー反映
15. ポーズ計算（HMD基準）
16. 総合動作テスト

---

## 参照ファイル

| 用途 | パス |
|-----|-----|
| ドライバーテンプレート | samples/drivers/drivers/simplecontroller/src/controller_device_driver.cpp |
| デバイスプロバイダー例 | samples/drivers/drivers/simplecontroller/src/device_provider.cpp |
| 入力プロファイル例 | samples/drivers/drivers/simplecontroller/simplecontroller/resources/input/mycontroller_profile.json |
| ImGui Win32+DX11例 | emucontroller/build/_deps/imgui-src/examples/example_win32_directx11/main.cpp |
| OpenVR Driver API | headers/openvr_driver.h |

---

## 検証方法

1. **GUIモック**: アプリ起動でウィンドウ表示、キー入力が画面に反映されること
2. **ビルド確認**: CMakeでEXEが生成されること
3. **ドライバー登録**: `vrpathreg adddriver` でSteamVRに認識されること
4. **入力テスト**: SteamVR入力バインディングUIでボタン/スティック入力が反映されること
5. **VRアプリテスト**: 実際のVRアプリでコントローラーが動作すること

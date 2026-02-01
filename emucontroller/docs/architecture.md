# アーキテクチャ

## システム概要

EmuControllerは2つの主要コンポーネントで構成されています：

1. **GUIアプリケーション** (`EmuController.exe`) - ユーザー入力を受け付けるWindowsアプリ
2. **OpenVRドライバー** (`driver_emucontroller.dll`) - SteamVRに仮想デバイスを提供

```
┌─────────────────────────┐     Named Pipe (IPC)     ┌─────────────────────────┐
│    GUI Application      │ ─────────────────────→   │    OpenVR Driver        │
│    (EmuController.exe)  │ ←─────────────────────   │  (driver_emucontroller) │
├─────────────────────────┤                          ├─────────────────────────┤
│ - ImGui + DirectX 11    │                          │ - IServerTrackedDeviceProvider│
│ - キーボード/マウス入力  │                          │ - EmuHMDDriver          │
│ - HMD視点操作           │                          │ - EmuControllerDriver x2│
│ - コントローラー入力     │                          │ - IPCServer             │
└─────────────────────────┘                          └─────────────────────────┘
         │                                                      │
         v                                                      v
   Windows API                                           SteamVR Runtime
   (GetAsyncKeyState)                                    (vrserver.exe)
                                                               │
                                                               v
                                                        ┌─────────────────┐
                                                        │  HEADSET WINDOW │
                                                        │  (VR映像表示)    │
                                                        └─────────────────┘
```

## フォルダ構成

```
emucontroller/
├── README.md              # メインドキュメント
├── CMakeLists.txt         # ルートCMake
├── docs/                  # 詳細ドキュメント
│   ├── architecture.md    # このファイル
│   ├── controller.md      # コントローラー詳細
│   ├── hmd.md            # HMD詳細
│   └── ipc-protocol.md   # IPC通信仕様
├── common/                # 共通ヘッダー
│   ├── ipc_protocol.h    # IPCメッセージ定義
│   └── input_state.h     # 入力状態構造体
├── driver/               # OpenVRドライバー
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── hmd_driver_factory.cpp    # エントリポイント
│   │   ├── device_provider.h/cpp     # デバイス管理
│   │   ├── hmd_device_driver.h/cpp   # HMDデバイス
│   │   ├── hmd_display_component.h/cpp # ディスプレイ
│   │   ├── controller_device_driver.h/cpp # コントローラー
│   │   ├── ipc_server.h/cpp          # IPC受信
│   │   └── driverlog.cpp             # ログ出力
│   └── emucontroller/
│       ├── driver.vrdrivermanifest   # ドライバーマニフェスト
│       └── resources/
│           ├── input/emucontroller_profile.json
│           └── settings/default.vrsettings
└── app/                  # GUIアプリケーション
    ├── CMakeLists.txt
    └── src/
        ├── main.cpp      # メインループ・UI
        ├── ipc_client.h/cpp # IPC送信
        └── ...
```

## コンポーネント詳細

### GUIアプリケーション

| モジュール | 役割 |
|-----------|------|
| `main.cpp` | ImGuiウィンドウ、入力処理、メインループ |
| `ipc_client.cpp` | Named Pipeクライアント、メッセージ送信 |

**使用ライブラリ:**
- ImGui (FetchContentで自動取得)
- DirectX 11

### OpenVRドライバー

| クラス | インターフェース | 役割 |
|--------|----------------|------|
| `EmuDeviceProvider` | `IServerTrackedDeviceProvider` | デバイス管理、フレーム更新 |
| `EmuHMDDriver` | `ITrackedDeviceServerDriver` | HMDデバイス |
| `EmuHMDDisplayComponent` | `IVRDisplayComponent` | ディスプレイ出力 |
| `EmuControllerDriver` | `ITrackedDeviceServerDriver` | コントローラーデバイス |
| `IPCServer` | - | Named Pipeサーバー |

### データフロー

```
[GUIアプリ]
    │
    │ キーボード/マウス入力
    v
┌─────────────────┐
│ UpdateHMDInput  │─────┐
│ UpdateInput     │     │
└─────────────────┘     │
    │                   │
    │ HMDPose,          │ ControllerInputState
    │ ControllerInputState
    v                   v
┌─────────────────────────┐
│     IPCClient           │
│  (Named Pipe送信)       │
└─────────────────────────┘
           │
           │ MSG_HMD_POSE, MSG_INPUT_STATE
           v
┌─────────────────────────┐
│     IPCServer           │
│  (Named Pipe受信)       │
└─────────────────────────┘
    │                   │
    v                   v
┌─────────────┐   ┌─────────────┐
│ EmuHMDDriver│   │EmuController│
│ UpdatePose  │   │Driver       │
└─────────────┘   └─────────────┘
    │                   │
    v                   v
┌─────────────────────────┐
│   SteamVR Runtime       │
│   (vrserver.exe)        │
└─────────────────────────┘
```

## ビルドシステム

CMakeベースの2段階構成：

1. **ルートCMakeLists.txt** - サブプロジェクトを統合
2. **driver/CMakeLists.txt** - ドライバーDLLをビルド
3. **app/CMakeLists.txt** - GUIアプリをビルド

### ビルド成果物

```
build/
├── bin/
│   └── Release/
│       └── EmuController.exe
└── drivers/
    └── emucontroller/
        ├── driver.vrdrivermanifest
        ├── bin/
        │   └── win64/
        │       └── driver_emucontroller.dll
        └── resources/
            ├── input/
            │   └── emucontroller_profile.json
            └── settings/
                └── default.vrsettings
```

## 参照ファイル

| 用途 | パス |
|-----|-----|
| OpenVR Driver API | `headers/openvr_driver.h` |
| コントローラーサンプル | `samples/drivers/drivers/simplecontroller/` |
| HMDサンプル | `samples/drivers/drivers/simplehmd/` |

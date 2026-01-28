---
name: openvr-driver-development
description: OpenVR/SteamVR向けドライバー（HMD、コントローラー、トラッカー）の新規開発を支援します。ドライバーの作成、ポーズ送信、入力システム実装、プロパティ設定について質問された場合に使用してください。
---

# OpenVR ドライバー開発スキル

OpenVR Driver APIに準拠したデバイスドライバーの開発を支援します。

## このリポジトリについて

**OpenVR**はValve社が開発したVRハードウェア向けのオープンなAPIです。SteamVRランタイムの基盤として使用され、様々なVRデバイスがSteamVRプラットフォーム上で動作することを可能にします。

**ドライバー**は、物理的なVRデバイス（HMD、コントローラー、トラッカー等）とSteamVRランタイムを接続するプラグインです。ドライバーがOpenVR Driver APIに準拠していれば、アプリケーション側での個別サポートなしに、すべてのSteamVR対応ゲーム・アプリで動作します。

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  VRアプリ   │ ←→ │  SteamVR    │ ←→ │  ドライバー │ ←→ ハードウェア
│ (ゲーム等)  │     │ ランタイム  │     │ (このスキル │
└─────────────┘     └─────────────┘     │  で開発)    │
                                        └─────────────┘
```

## 重要な参照先

- **公式ドキュメント**: `docs/Driver_API_Documentation.md`
- **ヘッダー**: `headers/openvr_driver.h`
- **サンプル**: `samples/drivers/drivers/` 配下

## クイックスタート

```cpp
// エントリポイント: HmdDriverFactory
extern "C" __declspec(dllexport)
void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode)
{
    if (0 == strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName))
        return &g_deviceProvider;
    if (pReturnCode) *pReturnCode = vr::VRInitError_Init_InterfaceNotFound;
    return nullptr;
}
```

## デバイスクラス

| クラス | 用途 |
|--------|------|
| `TrackedDeviceClass_HMD` | ヘッドセット |
| `TrackedDeviceClass_Controller` | コントローラー |
| `TrackedDeviceClass_GenericTracker` | トラッカー |
| `TrackedDeviceClass_TrackingReference` | ベースステーション |

## ドライバーフォルダ構造

```
<driver_name>/
├── bin/win64/driver_<name>.dll   # バイナリ（必須命名規則）
├── resources/
│   ├── input/<device>_profile.json
│   └── driver.vrresources
└── driver.vrdrivermanifest       # 必須
```

## 開発ワークフロー

1. `samples/drivers/drivers/barebones/` をコピーして開始
2. `IServerTrackedDeviceProvider` を実装
3. `ITrackedDeviceServerDriver` でデバイスを実装
4. デバイスを `TrackedDeviceAdded()` で登録
5. ポーズを `TrackedDevicePoseUpdated()` で送信
6. 入力を `CreateXxxComponent()` / `UpdateXxxComponent()` で処理

## 詳細リファレンス

**インターフェース実装** → [reference/interfaces.md](reference/interfaces.md)
- IServerTrackedDeviceProvider
- ITrackedDeviceServerDriver
- IVRDisplayComponent（HMD用）

**ポーズと座標系** → [reference/poses.md](reference/poses.md)
- DriverPose_t構造体
- 座標系（右手座標系、メートル単位）
- ポーズ更新のベストプラクティス

**入力システム** → [reference/input.md](reference/input.md)
- 入力プロファイルJSON
- 入力コンポーネントの作成・更新
- ハプティックフィードバック

**プロパティ設定** → [reference/properties.md](reference/properties.md)
- デバイスプロパティ一覧
- コントローラーロール
- アイコン設定

**設定ファイル** → [reference/config-files.md](reference/config-files.md)
- driver.vrdrivermanifest
- driver.vrresources
- default.vrsettings

## コードテンプレート

**コントローラー実装** → [examples/controller.md](examples/controller.md)
**HMD実装** → [examples/hmd.md](examples/hmd.md)
**トラッカー実装** → [examples/tracker.md](examples/tracker.md)
**共通ユーティリティ** → [examples/utilities.md](examples/utilities.md)

## トラブルシューティング

| 問題 | 原因 | 解決策 |
|------|------|--------|
| ドライバー読み込み失敗 | DLL命名規則 | `driver_<name>.dll`形式に |
| デバイス非表示 | ポーズ無効 | `qRotation.w = 1.0f`を設定 |
| 入力反映なし | プロファイルパス | `Prop_InputProfilePath_String`確認 |

## デバッグ

- **Web Console**: SteamVR → Developer → Web Console
- **ログ**: `C:\Program Files (x86)\Steam\logs\vrserver.txt`

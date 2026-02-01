# EmuController - OpenVR VR Emulator

物理的なVRヘッドセットやコントローラーなしで、SteamVRアプリケーションを操作・テストできるエミュレータです。

## 機能

- **HMDエミュレーション**: デスクトップウィンドウにVR映像を表示
- **コントローラーエミュレーション**: 左右両方のVRコントローラーをエミュレート
- **キーボード/マウス操作**: 直感的なPC入力でVR空間を操作
- **GUIアプリケーション**: ImGui製の操作パネルで入力状態を可視化

## クイックスタート

### 1. ビルド

```bash
# Releaseビルド
cmake -S emucontroller -B emucontroller/build -G "Visual Studio 17 2022" -A x64
cmake --build emucontroller/build --config Release
```

### 2. ドライバー登録

```bash
# SteamVRにドライバーを登録（管理者権限推奨）
"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win64\vrpathreg.exe" adddriver "path\to\emucontroller\build\drivers\emucontroller"
```

### 3. 起動

1. SteamVRを起動
2. `emucontroller/build/bin/Release/EmuController.exe` を実行
3. VR映像が「HEADSET WINDOW」に表示される

## 操作方法

### HMD操作

| 入力 | アクション |
|-----|----------|
| マウス右クリック + 移動 | 視点回転（Yaw/Pitch） |
| W/A/S/D | 前後左右移動 |
| Q/E | 上下移動 |
| Shift | 高速移動 |
| Ctrl | 低速移動 |
| R | 位置リセット |

### コントローラー操作

| 入力 | 左コントローラー | 右コントローラー |
|-----|-----------------|-----------------|
| スティック | W/A/S/D | Arrow Keys |
| トリガー | 左クリック | 右クリック |
| グリップ | 左Shift | 右Shift |
| A | Q | J |
| B | E | K |
| X | 1 | U |
| Y | 2 | I |
| システム | Escape | - |

## 必要環境

- Windows 10/11
- Visual Studio 2022（C++デスクトップ開発）
- CMake 3.16以上
- SteamVR

## 出力ファイル

| ファイル | パス |
|---------|-----|
| GUIアプリ | `build/bin/Release/EmuController.exe` |
| ドライバーDLL | `build/drivers/emucontroller/bin/win64/driver_emucontroller.dll` |
| マニフェスト | `build/drivers/emucontroller/driver.vrdrivermanifest` |

## トラブルシューティング

### 「Can't add a second HMD」エラー

SteamVR付属の`simplehmd`ドライバーが有効になっている可能性があります。

1. SteamVRを停止
2. 以下のファイルを編集（管理者権限必要）:
   `C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\simplehmd\resources\settings\default.vrsettings`
3. `"enable": true` を `"enable": false` に変更
4. SteamVRを再起動

### ドライバーが認識されない

```bash
# 登録状況を確認
vrpathreg show

# 再登録
vrpathreg removedriver "path\to\driver"
vrpathreg adddriver "path\to\driver"
```

## ドキュメント

詳細な技術情報は以下を参照してください：

- [アーキテクチャ](docs/architecture.md) - システム全体の設計
- [コントローラー](docs/controller.md) - コントローラーエミュレーションの詳細
- [HMD](docs/hmd.md) - HMDエミュレーションの詳細
- [IPC通信](docs/ipc-protocol.md) - アプリケーション-ドライバー間通信仕様

## ライセンス

このプロジェクトはOpenVR SDKのサンプルをベースにしています。

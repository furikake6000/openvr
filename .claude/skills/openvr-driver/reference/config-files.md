# 設定ファイル

## 目次
- driver.vrdrivermanifest
- driver.vrresources
- default.vrsettings
- 入力プロファイル
- デフォルトバインディング

---

## driver.vrdrivermanifest

ドライバールートに配置（**必須**）。

```json
{
  "alwaysActivate": false,
  "name": "mydriver",
  "directory": "",
  "resourceOnly": false,
  "hmd_presence": []
}
```

| キー | 説明 |
|------|------|
| `name` | ドライバー名（フォルダ名と一致） |
| `alwaysActivate` | 常にアクティブにするか |
| `resourceOnly` | リソースのみ（コード実行なし） |
| `hmd_presence` | HMD検出用USB VID:PID（例: `["28DE.*"]`） |
| `redirectsDisplay` | IVRVirtualDisplay使用時はtrue |

---

## driver.vrresources

`resources/driver.vrresources` に配置。

```json
{
  "jsonid": "vrresources",
  "statusicons": {
    "Controller": {
      "Prop_NamedIconPathDeviceOff_String": "{mydriver}/icons/off.png",
      "Prop_NamedIconPathDeviceSearching_String": "{mydriver}/icons/searching.gif",
      "Prop_NamedIconPathDeviceReady_String": "{mydriver}/icons/ready.png",
      "Prop_NamedIconPathDeviceNotReady_String": "{mydriver}/icons/error.png",
      "Prop_NamedIconPathDeviceStandby_String": "{mydriver}/icons/standby.png",
      "Prop_NamedIconPathDeviceAlertLow_String": "{mydriver}/icons/low.png"
    }
  }
}
```

### キー種別

- `Controller`: 全コントローラー
- `LeftController` / `RightController`: 左右別
- `<ModelNumber>`: モデル番号別

---

## default.vrsettings

`resources/settings/default.vrsettings` に配置。

```json
{
  "driver_mydriver": {
    "enable": true,
    "loadPriority": 0,
    "blocked_by_safe_mode": false,
    "custom_option": "default_value"
  }
}
```

### 設定の読み取り

```cpp
char buffer[256];
vr::VRSettings()->GetString("driver_mydriver", "custom_option",
                            buffer, sizeof(buffer));

int32_t intVal = vr::VRSettings()->GetInt32("driver_mydriver", "int_option");
float floatVal = vr::VRSettings()->GetFloat("driver_mydriver", "float_option");
bool boolVal = vr::VRSettings()->GetBool("driver_mydriver", "bool_option");
```

---

## 入力プロファイル

`resources/input/<device>_profile.json` に配置。

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
      "order": 1
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

### input_bindingui_mode

| モード | 説明 |
|--------|------|
| `controller_handed` | 左右ペアコントローラー |
| `single_device` | 単体デバイス |
| `hmd` | HMD |

---

## デフォルトバインディング

`resources/input/default_bindings/` に配置。

SteamVRバインディングUIで作成 → エクスポート → コピー。

```json
{
  "app_key": "steam.app.546560",
  "bindings": {
    "/actions/main": {
      "sources": [
        {
          "path": "/user/hand/right/input/trigger",
          "mode": "button",
          "inputs": {
            "click": { "output": "/actions/main/in/grab" }
          }
        }
      ]
    }
  }
}
```

バインディング優先順位:
1. ユーザー設定
2. アプリ開発者提供
3. Steamworks設定
4. ドライバーのdefault_bindings

---

## ディレクトリ構造まとめ

```
mydriver/
├── driver.vrdrivermanifest          # 必須
├── bin/
│   └── win64/
│       └── driver_mydriver.dll      # 必須命名規則
└── resources/
    ├── driver.vrresources           # アイコン設定
    ├── icons/
    │   ├── ready.png
    │   ├── off.png
    │   └── searching.gif
    ├── input/
    │   ├── mycontroller_profile.json
    │   └── default_bindings/
    │       └── steam.app.XXXXXX_mycontroller.json
    ├── settings/
    │   └── default.vrsettings
    └── localization/
        └── localization.json        # オプション
```

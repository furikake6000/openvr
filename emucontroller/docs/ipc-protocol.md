# IPC通信プロトコル

## 概要

GUIアプリケーション（クライアント）とOpenVRドライバー（サーバー）間の通信には、Windows Named Pipeを使用します。

## Named Pipe設定

| 項目 | 値 |
|-----|-----|
| パイプ名 | `\\.\pipe\EmuControllerPipe` |
| アクセスモード | 双方向（PIPE_ACCESS_DUPLEX） |
| 読み取りモード | メッセージモード（PIPE_READMODE_MESSAGE） |
| 最大インスタンス | 1 |

## メッセージフォーマット

すべてのメッセージは共通ヘッダーで始まります：

```cpp
// common/ipc_protocol.h
#pragma pack(push, 1)

struct MessageHeader {
    uint16_t type;    // メッセージタイプ
    uint16_t length;  // ペイロード長（ヘッダーを除く）
};

#pragma pack(pop)
```

## メッセージタイプ

```cpp
enum MessageType : uint16_t {
    // 接続管理
    MSG_CONNECT         = 0x0001,  // クライアント → サーバー: 接続要求
    MSG_DISCONNECT      = 0x0002,  // クライアント → サーバー: 切断通知
    MSG_STATUS_RESPONSE = 0x0004,  // サーバー → クライアント: 状態応答

    // 入力データ
    MSG_INPUT_STATE     = 0x0010,  // クライアント → サーバー: コントローラー入力
    MSG_HMD_POSE        = 0x0020,  // クライアント → サーバー: HMDポーズ
};
```

## メッセージ詳細

### MSG_CONNECT (0x0001)

クライアントがサーバーへ接続を通知します。

```cpp
struct ConnectMessage {
    MessageHeader header;  // type=0x0001, length=0
};
```

サーバーは`MSG_STATUS_RESPONSE`で応答します。

### MSG_DISCONNECT (0x0002)

クライアントが切断を通知します。

```cpp
struct DisconnectMessage {
    MessageHeader header;  // type=0x0002, length=0
};
```

### MSG_STATUS_RESPONSE (0x0004)

サーバーがデバイス状態を応答します。

```cpp
struct StatusResponse {
    MessageHeader header;  // type=0x0004
    uint8_t driver_active;    // ドライバー動作中
    uint8_t hmd_connected;    // HMD接続済み
    uint8_t left_connected;   // 左コントローラー接続済み
    uint8_t right_connected;  // 右コントローラー接続済み
};
```

### MSG_INPUT_STATE (0x0010)

コントローラーの入力状態を送信します。

```cpp
// コンパクトなワイヤーフォーマット
struct CompactControllerState {
    uint16_t buttons;     // ボタンビットフィールド
    int8_t stick_x;       // スティックX (-128~127 → -1.0~1.0)
    int8_t stick_y;       // スティックY (-128~127 → -1.0~1.0)
    uint8_t trigger;      // トリガー (0~255 → 0.0~1.0)
    uint8_t grip;         // グリップ (0~255 → 0.0~1.0)
};

struct InputStateMessage {
    MessageHeader header;  // type=0x0010
    CompactControllerState left;
    CompactControllerState right;
    uint64_t timestamp;    // ミリ秒タイムスタンプ
};
```

**変換関数:**

```cpp
// スティック値の変換
inline int8_t FloatToStick(float value) {
    return static_cast<int8_t>(std::clamp(value, -1.0f, 1.0f) * 127.0f);
}

inline float StickToFloat(int8_t value) {
    return static_cast<float>(value) / 127.0f;
}

// トリガー/グリップ値の変換
inline uint8_t FloatToTrigger(float value) {
    return static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
}

inline float TriggerToFloat(uint8_t value) {
    return static_cast<float>(value) / 255.0f;
}
```

### MSG_HMD_POSE (0x0020)

HMDの位置・姿勢を送信します。

```cpp
struct HMDPoseMessage {
    MessageHeader header;  // type=0x0020
    float position[3];     // x, y, z (メートル)
    float quaternion[4];   // w, x, y, z (回転)
    uint64_t timestamp;    // ミリ秒タイムスタンプ
};
```

**オイラー角からクォータニオンへの変換:**

```cpp
inline void EulerToQuaternion(float yaw, float pitch, float roll, float* quat) {
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);

    quat[0] = cr * cp * cy + sr * sp * sy;  // w
    quat[1] = sr * cp * cy - cr * sp * sy;  // x
    quat[2] = cr * sp * cy + sr * cp * sy;  // y
    quat[3] = cr * cp * sy - sr * sp * cy;  // z
}
```

## 通信フロー

### 接続シーケンス

```
Client                          Server (Driver)
   │                                │
   │──── MSG_CONNECT ──────────────→│
   │                                │
   │←─── MSG_STATUS_RESPONSE ───────│
   │                                │
```

### 入力更新シーケンス

```
Client                          Server (Driver)
   │                                │
   │──── MSG_INPUT_STATE ──────────→│  (コントローラー入力)
   │                                │
   │──── MSG_HMD_POSE ─────────────→│  (HMDポーズ)
   │                                │
   │    ... (60Hz程度で繰り返し) ...   │
   │                                │
```

### 切断シーケンス

```
Client                          Server (Driver)
   │                                │
   │──── MSG_DISCONNECT ───────────→│
   │                                │
   │         (パイプクローズ)         │
```

## エラー処理

| 状況 | クライアント動作 | サーバー動作 |
|-----|-----------------|-------------|
| サーバー未起動 | 1秒ごとに再接続試行 | - |
| 通信エラー | パイプクローズ、再接続試行 | パイプ再作成 |
| クライアント異常終了 | - | 接続待機状態に戻る |

## 実装クラス

### クライアント側 (IPCClient)

```cpp
class IPCClient {
public:
    bool Connect();
    void Disconnect();
    bool IsConnected() const;

    bool SendInputState(const ControllerInputState& left,
                        const ControllerInputState& right);
    bool SendHMDPose(const HMDPose& pose);

    void StartConnectionThread();  // バックグラウンド自動再接続
    void StopConnectionThread();
};
```

### サーバー側 (IPCServer)

```cpp
class IPCServer {
public:
    bool Start();
    void Stop();
    bool IsClientConnected() const;

    bool GetInputState(ControllerInputState& left,
                       ControllerInputState& right);
    bool GetHMDPose(HMDPoseData& pose);
};
```

## 今後の拡張予定

### フレームテクスチャ共有（Direct Mode用）

```cpp
// 将来の拡張
MSG_FRAME_TEXTURE = 0x0030,  // サーバー → クライアント: テクスチャ通知

struct FrameTextureMessage {
    MessageHeader header;
    uint64_t shared_handle;  // 共有テクスチャハンドル
    uint32_t width;
    uint32_t height;
    uint64_t frame_id;
};
```

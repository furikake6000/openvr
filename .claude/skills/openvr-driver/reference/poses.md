# ポーズと座標系

## 目次
- 座標系
- DriverPose_t構造体
- ポーズの作成
- ポーズの送信
- HMDポーズの取得

---

## 座標系

OpenVRは**右手座標系**を使用：
- **+Y**: 上
- **+X**: 右
- **-Z**: 前方
- 単位: **メートル**

```
      +Y (上)
       |
       |
       +---- +X (右)
      /
     /
   +Z (後方)   ※ -Z が前方
```

---

## DriverPose_t構造体

```cpp
struct DriverPose_t
{
    // 位置 (メートル)
    double vecPosition[3];

    // 速度 (m/s)
    double vecVelocity[3];

    // 加速度 (m/s²)
    double vecAcceleration[3];

    // 回転 (クォータニオン)
    HmdQuaternion_t qRotation;

    // 角速度 (rad/s)
    double vecAngularVelocity[3];

    // 角加速度 (rad/s²)
    double vecAngularAcceleration[3];

    // トラッキング状態
    ETrackingResult result;

    // ポーズの有効性
    bool poseIsValid;

    // 接続状態
    bool deviceIsConnected;

    // 座標変換（通常は単位クォータニオン）
    HmdQuaternion_t qWorldFromDriverRotation;
    double vecWorldFromDriverTranslation[3];

    HmdQuaternion_t qDriverFromHeadRotation;
    double vecDriverFromHeadTranslation[3];

    // ポーズのタイムオフセット
    double poseTimeOffset;
};
```

---

## ポーズの作成

```cpp
vr::DriverPose_t CreatePose(double x, double y, double z,
                            const vr::HmdQuaternion_t &rotation)
{
    vr::DriverPose_t pose = {0};

    // 必須: 座標変換クォータニオンを単位クォータニオンに
    pose.qWorldFromDriverRotation.w = 1.0f;
    pose.qDriverFromHeadRotation.w = 1.0f;

    // 位置
    pose.vecPosition[0] = x;
    pose.vecPosition[1] = y;
    pose.vecPosition[2] = z;

    // 回転（必須: wを有効な値に）
    pose.qRotation = rotation;

    // 状態
    pose.poseIsValid = true;
    pose.deviceIsConnected = true;
    pose.result = vr::TrackingResult_Running_OK;

    return pose;
}
```

### 単位クォータニオン（回転なし）

```cpp
vr::HmdQuaternion_t identity = {1.0, 0.0, 0.0, 0.0}; // w, x, y, z
```

---

## ポーズの送信

### 推奨: 別スレッドで定期更新

```cpp
void PoseUpdateThread()
{
    while (m_isActive)
    {
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
            m_deviceIndex,
            GetPose(),
            sizeof(vr::DriverPose_t)
        );

        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // 200Hz
    }
}
```

### 非推奨: RunFrame内での更新

RunFrameはメインループで呼ばれるため、ブロッキングを避けるべき。

---

## HMDポーズの取得

他デバイス（コントローラー等）からHMDの位置を参照する場合：

```cpp
vr::TrackedDevicePose_t hmd_pose{};
vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0.f, &hmd_pose, 1);

// HMDは常にインデックス0

// 位置を抽出
float x = hmd_pose.mDeviceToAbsoluteTracking.m[0][3];
float y = hmd_pose.mDeviceToAbsoluteTracking.m[1][3];
float z = hmd_pose.mDeviceToAbsoluteTracking.m[2][3];
```

### 行列からクォータニオンを抽出

```cpp
vr::HmdQuaternion_t HmdQuaternion_FromMatrix(const vr::HmdMatrix34_t &m)
{
    vr::HmdQuaternion_t q = {};
    q.w = sqrt(fmax(0, 1 + m.m[0][0] + m.m[1][1] + m.m[2][2])) / 2;
    q.x = sqrt(fmax(0, 1 + m.m[0][0] - m.m[1][1] - m.m[2][2])) / 2;
    q.y = sqrt(fmax(0, 1 - m.m[0][0] + m.m[1][1] - m.m[2][2])) / 2;
    q.z = sqrt(fmax(0, 1 - m.m[0][0] - m.m[1][1] + m.m[2][2])) / 2;
    q.x = copysign(q.x, m.m[2][1] - m.m[1][2]);
    q.y = copysign(q.y, m.m[0][2] - m.m[2][0]);
    q.z = copysign(q.z, m.m[1][0] - m.m[0][1]);
    return q;
}
```

---

## トラッキング状態 (ETrackingResult)

| 値 | 説明 |
|----|------|
| `TrackingResult_Uninitialized` | 初期化前 |
| `TrackingResult_Calibrating_InProgress` | キャリブレーション中 |
| `TrackingResult_Calibrating_OutOfRange` | 範囲外 |
| `TrackingResult_Running_OK` | 正常トラッキング |
| `TrackingResult_Running_OutOfRange` | 動作中だが範囲外 |
| `TrackingResult_Fallback_RotationOnly` | 回転のみ |

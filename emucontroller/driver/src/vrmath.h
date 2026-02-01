#pragma once

#include <openvr_driver.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)

inline vr::HmdVector3_t HmdVector3_From34Matrix(const vr::HmdMatrix34_t& matrix) {
    return { matrix.m[0][3], matrix.m[1][3], matrix.m[2][3] };
}

inline vr::HmdQuaternion_t HmdQuaternion_FromMatrix(const vr::HmdMatrix34_t& matrix) {
    vr::HmdQuaternion_t q;
    float trace = matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2];
    if (trace > 0) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (matrix.m[2][1] - matrix.m[1][2]) * s;
        q.y = (matrix.m[0][2] - matrix.m[2][0]) * s;
        q.z = (matrix.m[1][0] - matrix.m[0][1]) * s;
    } else {
        if (matrix.m[0][0] > matrix.m[1][1] && matrix.m[0][0] > matrix.m[2][2]) {
            float s = 2.0f * sqrtf(1.0f + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2]);
            q.w = (matrix.m[2][1] - matrix.m[1][2]) / s;
            q.x = 0.25f * s;
            q.y = (matrix.m[0][1] + matrix.m[1][0]) / s;
            q.z = (matrix.m[0][2] + matrix.m[2][0]) / s;
        } else if (matrix.m[1][1] > matrix.m[2][2]) {
            float s = 2.0f * sqrtf(1.0f + matrix.m[1][1] - matrix.m[0][0] - matrix.m[2][2]);
            q.w = (matrix.m[0][2] - matrix.m[2][0]) / s;
            q.x = (matrix.m[0][1] + matrix.m[1][0]) / s;
            q.y = 0.25f * s;
            q.z = (matrix.m[1][2] + matrix.m[2][1]) / s;
        } else {
            float s = 2.0f * sqrtf(1.0f + matrix.m[2][2] - matrix.m[0][0] - matrix.m[1][1]);
            q.w = (matrix.m[1][0] - matrix.m[0][1]) / s;
            q.x = (matrix.m[0][2] + matrix.m[2][0]) / s;
            q.y = (matrix.m[1][2] + matrix.m[2][1]) / s;
            q.z = 0.25f * s;
        }
    }
    return q;
}

inline vr::HmdQuaternion_t HmdQuaternion_FromEulerAngles(float roll, float pitch, float yaw) {
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);

    vr::HmdQuaternion_t q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return q;
}

inline vr::HmdQuaternion_t operator*(const vr::HmdQuaternion_t& a, const vr::HmdQuaternion_t& b) {
    vr::HmdQuaternion_t result;
    result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return result;
}

inline vr::HmdVector3_t operator*(const vr::HmdVector3_t& v, const vr::HmdQuaternion_t& q) {
    vr::HmdQuaternion_t vq = { 0, v.v[0], v.v[1], v.v[2] };
    vr::HmdQuaternion_t qConj = { q.w, -q.x, -q.y, -q.z };
    vr::HmdQuaternion_t result = q * vq * qConj;
    return { static_cast<float>(result.x), static_cast<float>(result.y), static_cast<float>(result.z) };
}

inline vr::HmdVector3_t operator+(const vr::HmdVector3_t& a, const vr::HmdVector3_t& b) {
    return { a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2] };
}

#include "Quaternion.h"
#include <cmath>
#include <numbers>

void Quaternion::SetFromTo(const Vector3 &from, const Vector3 &to) {
    Vector3 f = from.Normalize();
    Vector3 t = to.Normalize();

    float dot = f.Dot(t);

    // 逆方向の場合の特別処理
    if (dot < -0.999999f) {
        // 90度回転した軸を見つける
        Vector3 axis = Vector3(1.0f, 0.0f, 0.0f).Cross(f);
        if (axis.Length() < 0.000001f) {
            axis = Vector3(0.0f, 1.0f, 0.0f).Cross(f);
        }
        axis = axis.Normalize();

        // 180度回転
        *this = FromAxisAngle(axis, std::numbers::pi_v<float>);
        return;
    }

    // 同じ方向の場合
    if (dot > 0.999999f) {
        *this = IdentityQuaternion();
        return;
    }

    Vector3 cross = f.Cross(t);

    w = std::sqrt((1.0f + dot) * 0.5f);
    float s = 0.5f / w;

    x = cross.x * s;
    y = cross.y * s;
    z = cross.z * s;
}

Quaternion Quaternion::FromEulerAngles(const Vector3 &eulerAngles) {
    // XYZ回転順序でオイラー角からクォータニオンを作成
    float halfX = eulerAngles.x * 0.5f;
    float halfY = eulerAngles.y * 0.5f;
    float halfZ = eulerAngles.z * 0.5f;

    float cosX = std::cos(halfX);
    float sinX = std::sin(halfX);
    float cosY = std::cos(halfY);
    float sinY = std::sin(halfY);
    float cosZ = std::cos(halfZ);
    float sinZ = std::sin(halfZ);

    return Quaternion(
        sinX * cosY * cosZ - cosX * sinY * sinZ, // x
        cosX * sinY * cosZ + sinX * cosY * sinZ, // y
        cosX * cosY * sinZ - sinX * sinY * cosZ, // z
        cosX * cosY * cosZ + sinX * sinY * sinZ  // w
    );
}

Vector3 Quaternion::ToEulerAngles() const {
    Vector3 angles;

    // X軸回転（ピッチ）
    float sinP = 2.0f * (w * x + y * z);
    float cosP = 1.0f - 2.0f * (x * x + y * y);
    angles.x = std::atan2(sinP, cosP);

    // Y軸回転（ヨー）
    float sinY = 2.0f * (w * y - z * x);
    if (std::abs(sinY) >= 1.0f) {
        angles.y = std::copysign(std::numbers::pi_v<float> / 2.0f, sinY);
    } else {
        angles.y = std::asin(sinY);
    }

    // Z軸回転（ロール）
    float sinR = 2.0f * (w * z + x * y);
    float cosR = 1.0f - 2.0f * (y * y + z * z);
    angles.z = std::atan2(sinR, cosR);

    return angles;
}

Quaternion Quaternion::Conjugate() const {
    return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::Normalize() const {
    float length = std::sqrt(x * x + y * y + z * z + w * w);
    if (length < 0.000001f) {
        return IdentityQuaternion();
    }
    return Quaternion(x / length, y / length, z / length, w / length);
}

Quaternion Quaternion::FromLookRotation(const Vector3 &direction, const Vector3 &up) {
    Vector3 forward = direction.Normalize();
    Vector3 right = up.Cross(forward).Normalize();
    Vector3 newUp = forward.Cross(right);

    // 修正されたクォータニオン計算
    float trace = right.x + newUp.y + forward.z;

    if (trace > 0.0f) {
        float s = std::sqrt(trace + 1.0f) * 2.0f;
        return Quaternion(
            (newUp.z - forward.y) / s,
            (forward.x - right.z) / s,
            (right.y - newUp.x) / s,
            s / 4.0f);
    } else if (right.x > newUp.y && right.x > forward.z) {
        float s = std::sqrt(1.0f + right.x - newUp.y - forward.z) * 2.0f;
        return Quaternion(
            s / 4.0f,
            (right.y + newUp.x) / s,
            (forward.x + right.z) / s,
            (newUp.z - forward.y) / s);
    } else if (newUp.y > forward.z) {
        float s = std::sqrt(1.0f + newUp.y - right.x - forward.z) * 2.0f;
        return Quaternion(
            (right.y + newUp.x) / s,
            s / 4.0f,
            (newUp.z + forward.y) / s,
            (forward.x - right.z) / s);
    } else {
        float s = std::sqrt(1.0f + forward.z - right.x - newUp.y) * 2.0f;
        return Quaternion(
            (forward.x + right.z) / s,
            (newUp.z + forward.y) / s,
            s / 4.0f,
            (right.y - newUp.x) / s);
    }
}

Quaternion Quaternion::operator*(const Quaternion &q) const {
    return Quaternion(
        w * q.x + x * q.w + y * q.z - z * q.y,
        w * q.y - x * q.z + y * q.w + z * q.x,
        w * q.z + x * q.y - y * q.x + z * q.w,
        w * q.w - x * q.x - y * q.y - z * q.z);
}

Quaternion Quaternion::operator+(const Quaternion &other) const {
    return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w);
}

Quaternion Quaternion::operator-(const Quaternion &other) const {
    return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w);
}

Quaternion Quaternion::operator/(const Quaternion &other) const {
    Quaternion inverse = other.Inverse();
    return *this * inverse;
}

Quaternion Quaternion::operator*(const float &scalar) const {
    return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
}

Quaternion Quaternion::operator+=(const Quaternion &other) {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

// クォータニオンの減算代入
Quaternion Quaternion::operator-=(const Quaternion &other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

Vector3 Quaternion::operator*(const Vector3 &v) const {
    Quaternion qv(v.x, v.y, v.z, 0.0f);
    Quaternion res = (*this) * qv * this->Conjugate();
    return {res.x, res.y, res.z};
}

Quaternion Quaternion::IdentityQuaternion() {
    return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
}

float Quaternion::Norm() const {
    return std::sqrt(x * x + y * y + z * z + w * w);
}

float Quaternion::Dot(const Quaternion &other) const {
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

Quaternion Quaternion::Inverse() const {
    float normSquared = x * x + y * y + z * z + w * w;
    if (normSquared < 0.000001f) {
        return IdentityQuaternion();
    }
    Quaternion conjugate = Conjugate();
    return Quaternion(conjugate.x / normSquared, conjugate.y / normSquared,
                      conjugate.z / normSquared, conjugate.w / normSquared);
}

Quaternion Quaternion::Slerp(const Quaternion &q1, const Quaternion &q2, float t) {
    Quaternion q0 = q1;
    Quaternion q1_copy = q2;

    float dot = q0.Dot(q1_copy);

    // 短い回転経路を選択
    if (dot < 0.0f) {
        q0 = Quaternion(-q0.x, -q0.y, -q0.z, -q0.w);
        dot = -dot;
    }

    const float threshold = 0.9995f;
    if (dot > threshold) {
        // 線形補間
        Quaternion result = Quaternion(
            q0.x + t * (q1_copy.x - q0.x),
            q0.y + t * (q1_copy.y - q0.y),
            q0.z + t * (q1_copy.z - q0.z),
            q0.w + t * (q1_copy.w - q0.w));
        return result.Normalize();
    }

    // 球面線形補間
    float theta_0 = std::acos(std::abs(dot));
    float theta = theta_0 * t;

    float sin_theta = std::sin(theta);
    float sin_theta_0 = std::sin(theta_0);

    float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;

    return Quaternion(
        s0 * q0.x + s1 * q1_copy.x,
        s0 * q0.y + s1 * q1_copy.y,
        s0 * q0.z + s1 * q1_copy.z,
        s0 * q0.w + s1 * q1_copy.w);
}

Vector3 Quaternion::GetAxis() const {
    float sinHalfAngle = std::sqrt(1.0f - w * w);
    if (sinHalfAngle < 0.000001f) {
        return Vector3(1.0f, 0.0f, 0.0f); // 任意の軸
    }
    return Vector3(x / sinHalfAngle, y / sinHalfAngle, z / sinHalfAngle);
}

float Quaternion::GetAngle() const {
    return 2.0f * std::acos(std::abs(w));
}

Quaternion Quaternion::FromAxisAngle(const Vector3 &axis, float angle) {
    Vector3 normalizedAxis = axis.Normalize();
    float halfAngle = angle * 0.5f;
    float sinHalfAngle = std::sin(halfAngle);

    return Quaternion(
        normalizedAxis.x * sinHalfAngle,
        normalizedAxis.y * sinHalfAngle,
        normalizedAxis.z * sinHalfAngle,
        std::cos(halfAngle));
}
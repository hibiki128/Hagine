#pragma once
#include "Vector3.h"
#include <cmath>
#include <numbers>

class Quaternion final {
public:
    float x, y, z, w;

    // コンストラクタ
    Quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    // クォータニオン同士の掛け算をオーバーロード
    Quaternion operator*(const Quaternion& q) const;

    // クォータニオン同士の加算
    Quaternion operator+(const Quaternion& other) const;

    // クォータニオン同士の減算
    Quaternion operator-(const Quaternion& other) const;

    // クォータニオン同士の除算
    Quaternion operator/(const Quaternion& other) const;

    Quaternion operator*(const float& scalar) const;

    // 2つのベクトルの間の回転を計算
    void SetFromTo(const Vector3& from, const Vector3& to);

    // オイラー角からクォータニオンを生成
    static Quaternion FromEulerAngles(const Vector3& eulerAngles);

    // クォータニオンをオイラー角に変換
    Vector3 ToEulerAngles() const;

    // クォータニオンの共役を返す
    Quaternion Conjugate() const;
    // クォータニオンの正規化
    Quaternion Normalize() const;
    // 方向ベクトルと上ベクトルから回転クォータニオンを生成
    static Quaternion FromLookRotation(const Vector3& direction, const Vector3& up);
    // 単位クォータニオンを返す
    static Quaternion IdentityQuaternion();

    // ノルム（長さ）を計算
    float Norm() const;

    float Dot(const Quaternion& other)const;

    // 逆クォータニオンを返す
    Quaternion Inverse() const;

    // Sleap補間（Slerp）を計算
    Quaternion Sleap(Quaternion q1, Quaternion q2, float t);
};


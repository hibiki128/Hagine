#pragma once
#include "Vector3.h"
#include <cmath>
#include <numbers>
#include "Matrix4x4.h"

class Quaternion final {
  public:
    float x, y, z, w;

    // コンストラクタ
    Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    // クォータニオン同士の掛け算をオーバーロード
    Quaternion operator*(const Quaternion &q) const;

    // クォータニオン同士の加算
    Quaternion operator+(const Quaternion &other) const;

    // クォータニオン同士の減算
    Quaternion operator-(const Quaternion &other) const;

    // クォータニオン同士の除算
    Quaternion operator/(const Quaternion &other) const;

    Quaternion operator*(const float &scalar) const;

    // クォータニオンの加算代入
    Quaternion operator+=(const Quaternion &other);

    // クォータニオンの減算代入
    Quaternion operator-=(const Quaternion &other);

    Vector3 operator*(const Vector3 &v) const;

    // 単項マイナス演算子（符号反転）
    Quaternion operator-() const;

    // 2つのベクトルの間の回転を計算
    void SetFromTo(const Vector3 &from, const Vector3 &to);

    // オイラー角からクォータニオンを生成
    static Quaternion FromEulerAngles(const Vector3 &eulerAngles);

    // クォータニオンをオイラー角に変換 - ToEulerメソッド追加
    Vector3 ToEulerAngles() const;
    Vector3 ToEuler() const { return ToEulerAngles(); }

    // クォータニオンの共役を返す
    Quaternion Conjugate() const;

    // クォータニオンの正規化
    Quaternion Normalize() const;

    // 方向ベクトルと上ベクトルから回転クォータニオンを生成
    static Quaternion FromLookRotation(const Vector3 &direction, const Vector3 &up);

    // 単位クォータニオンを返す
    static Quaternion IdentityQuaternion();

    // ノルム（長さ）を計算
    float Norm() const;

    float Dot(const Quaternion &other) const;

    // 逆クォータニオンを返す
    Quaternion Inverse() const;

    // Slerp補間を計算（メソッド名を修正）
    static Quaternion Slerp(const Quaternion &q1, const Quaternion &q2, float t);

    // 回転軸を取得
    Vector3 GetAxis() const;

    // 回転角度を取得（ラジアン）
    float GetAngle() const;

    // 軸と角度からクォータニオンを作成
    static Quaternion FromAxisAngle(const Vector3 &axis, float angle);

    static Quaternion FromEulerDegrees(const Vector3 &eulerDegrees);
    Vector3 ToEulerDegrees() const;

    static Quaternion FromMatrix(const Matrix4x4 &matrix);

    // ジンバルロックを回避するための改良されたメソッド
    static Quaternion FromEulerAnglesSafe(const Vector3 &eulerAngles);
    Vector3 ToEulerAnglesSafe() const;
    
    // 軸回転を個別に適用するメソッド
    static Quaternion FromAxisRotations(const Vector3 &axisRotations);
};
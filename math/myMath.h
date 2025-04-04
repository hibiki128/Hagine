#pragma once
#include"Matrix4x4.h"
#include "Vector4.h"
#include "assert.h"
#include "cmath"
#include <Vector3.h>
#include <Quaternion.h>

struct Sphere {
	Vector3 center; // !< 中心点
	float radius;   // !< 半径
};
struct AABB {
	Vector3 min; //!< 最小点
	Vector3 max; //!< 最大点
};
struct OBB {
	Vector3 rotationCenter;  // 回転中心
	Vector3 scaleCenter;     // スケール中心
	Vector3 scaleCenterRotated; // 回転後のスケール中心
	Vector3 size;            // サイズ
	Vector3 orientations[3]; // 各軸の方向ベクトル
};

class ViewProjection;

static const int kColumnWidth = 60;
static const int kRowHeight = 20;

float Lerp(float _start, float _end, float _t);
Vector3 Lerp(const Vector3& _start, const Vector3& _end, float _t);
Vector4 Lerp(const Vector4& _start, const Vector4& _end, float _t);


//float Lerp(float _start, float _end, float _t);
//Vector3 Lerp(const Vector3& _start, const Vector3& _end, float _t);
//Vector4 Lerp(const Vector4& _start, const Vector4& _end, float _t);
// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

// 拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

Matrix4x4 MakeOBBWorldMatrix(const OBB& obb, const Matrix4x4& rotateMatrix);
AABB ConvertOBBToAABB(const OBB& obb);
float getProjection(const Vector3& axis, const OBB& obb);

// 座標変換
Vector3 Transformation(const Vector3& vector, const Matrix4x4& matrix);
Vector4 Transformation(const Vector4& vector, const Matrix4x4& matrix);

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

// 逆行列
Matrix4x4 Inverse(const Matrix4x4& m);

// 転置行列
Matrix4x4 Transpose(const Matrix4x4& m);

// 単位行列の作成
Matrix4x4 MakeIdentity4x4();

// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian);
// Y軸回転行列							
Matrix4x4 MakeRotateYMatrix(float radian);
// Z軸回転行列							
Matrix4x4 MakeRotateZMatrix(float radian);
// X,Y,Z軸回転行列を合成した行列
Matrix4x4 MakeRotateXYZMatrix(const Vector3& radian);

Matrix4x4 MakeRotateXYZMatrix(const Quaternion& quat);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

// tanθの逆数
float cotf(float theta);

// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

// ビューポート変換行列
Matrix4x4 MakeViewPortMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

// クォータニオンから回転軸(Vector3)を計算する関数
Vector3 QuaternionToAxis(const Quaternion& q);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate);

Matrix4x4 QuaternionToMatrix4x4(const Quaternion& q);

float LerpShortAngle(float a, float b, float t);

// 行列から回転成分をオイラー角に変換して取得
Vector3 GetEulerAnglesFromMatrix(const Matrix4x4& mat);


Vector3 ScreenTransform(Vector3 worldPos, const ViewProjection& viewProjection);


float radiansToDegrees(float radians);

float degreesToRadians(float degrees);

Quaternion Slerp(Quaternion q0, Quaternion q1, float t);

//// デバッグ用
//void VectorScreenPrintf(int x, int y, const Vector3& vector, const char* label);
//void MatrixScreenPrintf(int x, int y, const Matrix4x4& matrix, const char* label);

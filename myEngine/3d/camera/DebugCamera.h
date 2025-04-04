#pragma once
#include"ViewProjection.h"
#include"Vector3.h"
#include"Vector2.h"
#include"Matrix4x4.h"
class DebugCamera
{
public:
	// X,Y,Z軸回りのローカル回転角
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
	// ローカル座標
	Vector3 translation_ = { 0.0f, 0.0f, -50.0f };
	Matrix4x4 matRot_;
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(ViewProjection* viewProjection);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	void imgui();

	bool GetActive() { return isActive_; }

private:
	void CameraMove(Vector3& cameraRotate, Vector3& cameraTranslate, Vector2& clickPosition);
private:
	ViewProjection* viewProjection_;
	Vector2 mouse;
	bool useMouse = true;
	float mouseSensitivity = 0.003f;
	// カメラの移動速度
	float moveZspeed = 0.005f;
	Matrix4x4 matRotDelta;
	Matrix4x4 rotateXYZMatrix;
	bool isActive_ = false;
};


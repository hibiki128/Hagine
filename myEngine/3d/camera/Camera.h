#pragma once
#include <math/Vector3.h>
#include"math/Matrix4x4.h"
class Camera
{
private:

	struct Transform {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};

	Transform transform;
	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
	Matrix4x4 viewProjectionMatrix;

	float fovY;
	float aspectRatio;
	float nearClip;
	float farClip;


public: // メンバ関数
	
	/// <summary>
	/// コンストラクタ
	/// </summary>
	Camera();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// setter
	/// </summary>
	/// <param name="rotate"></param>
	void SetRotate(const Vector3& rotate) { transform.rotate=rotate; }
	void SetTranslate(const Vector3& translate) { transform.translate = translate; }
	void SetFovY(const float& fovY) { this->fovY = fovY; }
	void SetAspectRatio(const float& aspectRatio) { this->aspectRatio = aspectRatio; }
	void SetNearClip(const float& nearClip) { this->nearClip = nearClip; }
	void SetFarClip(const float& farClip) { this->farClip = farClip; }
	
	/// <summary>
	/// getter
	/// </summary>
	/// <returns></returns>
	const Matrix4x4& GetWorldMatrix()const { return worldMatrix; }
	const Matrix4x4& GetViewMatrix()const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix()const { return projectionMatrix; }
	const Matrix4x4& GetViewProjectionMatrix()const { return viewProjectionMatrix; }
	const Vector3& GetRotate()const { return transform.rotate; }
	const Vector3& GetTranslate()const { return transform.translate; }

};


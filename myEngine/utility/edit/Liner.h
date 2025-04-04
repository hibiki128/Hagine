#pragma once
#include"Object3d.h"
#include"WorldTransform.h"
class Liner
{
public:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	Liner();
	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const Vector3& startPoint, const Vector3& endPoint, const ViewProjection& viewProjection);

private:
	/// <summary>
	/// 更新
	/// </summary>
	void Update(const Vector3& startPoint, const Vector3& endPoint);

private:
	std::unique_ptr<Object3d>obj3d_;
	WorldTransform worldTransform_;

};


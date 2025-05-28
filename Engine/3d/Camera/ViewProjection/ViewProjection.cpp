#define NOMINMAX
#include "ViewProjection.h"
#include"myMath.h"
#include <type/Vector2.h>
#include"cmath"

void ViewProjection::Initialize()
{
	matView_ = MakeIdentity4x4();
	matProjection_ = MakeIdentity4x4();
	matWorld_ = MakeIdentity4x4();

	dxCommon_ = DirectXCommon::GetInstance();

	CreateConstBuffer();
	Map();
	UpdateMatrix();
}

void ViewProjection::CreateConstBuffer()
{
	const UINT bufferSize = sizeof(ConstBufferDataViewProjection);
	constBuffer_ = dxCommon_->CreateBufferResource(bufferSize);
}

void ViewProjection::Map()
{
	HRESULT hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap));
	if (FAILED(hr)) {
		// エラーハンドリング

	}
}

void ViewProjection::UpdateMatrix()
{
	UpdateViewMatrix();
	UpdateProjectionMatrix();

	TransferMatrix();
}

void ViewProjection::TransferMatrix()
{
	if (constMap) {
		constMap->view = matView_;
		constMap->projection = matProjection_;
		constMap->cameraPos = translation_;
	}
}

void ViewProjection::UpdateViewMatrix()
{
	matView_ = Inverse(MakeAffineMatrix({ 1.0f,1.0f,1.0f }, rotation_, translation_));
}

void ViewProjection::UpdateProjectionMatrix()
{
	matProjection_ = MakePerspectiveFovMatrix(fovAngleY, aspectRatio, nearZ, farZ);
}

bool ViewProjection::IsOutsideViewFrustum(const WorldTransform& worldTransform) const
{
	// オブジェクトのワールド座標を取得
	Vector3 objPosition = worldTransform.translation_;

	// スケールを加味してオブジェクトのサイズを考慮
	// ここではスケールの最大値を取得し、オフセットを計算する
	float maxScale = std::max({ worldTransform.scale_.x, worldTransform.scale_.y, worldTransform.scale_.z });

	// ビュー行列を使ってカメラ座標に変換
	Vector3 viewSpacePosition = Transformation(objPosition, matView_);

	// 射影行列を使ってクリッピング空間に変換
	Vector4 clipSpacePosition = Transformation(Vector4(viewSpacePosition.x, viewSpacePosition.y, viewSpacePosition.z, 1.0f), matProjection_);

	// NDC (Normalized Device Coordinates) に変換
	Vector2 ndcPosition = Vector2(
		clipSpacePosition.x / clipSpacePosition.w,
		clipSpacePosition.y / clipSpacePosition.w
	);

	// NDC範囲チェック (-1.0 ~ 1.0)
	// スケールを加味して範囲を拡大
	bool isOutside = (ndcPosition.x < (-0.25f - maxScale) || ndcPosition.x >(0.25f + maxScale) ||
		ndcPosition.y < (-0.25f - maxScale) || ndcPosition.y >(0.25f + maxScale));

	return isOutside; // 視野外であればtrue
}


#include "WorldTransform.h"

void WorldTransform::Initialize()
{
	// スケール、回転、平行移動を初期化
	scale_ = { 1.0f, 1.0f, 1.0f }; // デフォルトのスケール
	rotation_ = { 0.0f, 0.0f, 0.0f }; // デフォルトの回転
	translation_ = { 0.0f, 0.0f, 0.0f }; // デフォルトの位置
	dxCommon_ = DirectXCommon::GetInstance();
	// 行列の初期化
	matWorld_ = MakeIdentity4x4(); // 単位行列で初期化

	CreateConstBuffer();
	Map();
	UpdateMatrix();
}

void WorldTransform::TransferMatrix()
{
	// 定数バッファに転送
	if (constMap) {
		constMap->matWorld = matWorld_;
	}
}

void WorldTransform::CreateConstBuffer()
{
	const UINT bufferSize = sizeof(ConstBufferDataWorldTransform);
	constBuffer_ = dxCommon_->CreateBufferResource(bufferSize);
}

void WorldTransform::Map()
{
	// バッファのマッピング
	HRESULT hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap));
	if (FAILED(hr)) {
		// エラーハンドリング
	}
}

void WorldTransform::UpdateMatrix()
{
	// スケール、回転、平行移動を合成して行列を計算する
	matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

	// 親があれば親のワールド行列を掛ける
	if (parent_) {
		matWorld_ *= parent_->matWorld_;
	}

	// 定数バッファに転送する
	TransferMatrix();
}


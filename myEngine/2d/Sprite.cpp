#include "Sprite.h"
#include"SpriteCommon.h"
#include "math/myMath.h"
#include "TextureManager.h"

void Sprite::Initialize(const std::string& textureFilePath, Vector2 position, Vector4 color, Vector2 anchorpoint, bool isFlipX, bool isFlipY)
{
	// 引数で受け取ってメンバ変数に記録する
	spriteCommon_ = SpriteCommon::GetInstance();

	fullpath = textureFilePath;

	TextureManager::GetInstance()->LoadTexture(fullpath);

	CreateVartexData();

	CreateMaterial();

	CreateTransformationMatrix();

	position_ = position;
	materialData->color = color;
	anchorPoint_ = anchorpoint;
	isFlipX_ = isFlipX;
	isFlipY_ = isFlipY;

	AdjustTextureSize();
}

void Sprite::Update(bool isbackmost_)
{

	float left = 0.0f - anchorPoint_.x;
	float right = 1.0f - anchorPoint_.x;
	float top = 0.0f - anchorPoint_.y;
	float bottom = 1.0f - anchorPoint_.y;

	// 左右反転
	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	// 上下反転
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(fullpath);
	float tex_left = textureLeftTop.x / metadata.width;
	float tex_right = (textureLeftTop.x + textureSize.x) / metadata.width;
	float tex_top = textureLeftTop.y / metadata.height;
	float tex_bottom = (textureLeftTop.y + textureSize.y) / metadata.height;

	// 頂点データの設定
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 1枚目の三角形
	vertexData[0].position = { left,bottom,0.0f,1.0f }; // 左下
	vertexData[0].texcoord = { tex_left,tex_bottom };

	vertexData[1].position = { left,top,0.0f,1.0f }; // 左上
	vertexData[1].texcoord = { tex_left,tex_top };

	vertexData[2].position = { right,bottom,0.0f,1.0f }; // 右下
	vertexData[2].texcoord = { tex_right,tex_bottom };

	vertexData[3].position = { right,top,0.0f,1.0f }; // 右上
	vertexData[3].texcoord = { tex_right,tex_top };
	///==========================================================

	// インデックスリソースにデータを書き込む（sprite用）
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
	indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;
	///=======================================================

	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	if (isbackmost_) {
		transform.translate = { position_.x,position_.y,10000.0f };
	}
	else {
	transform.translate = { position_.x,position_.y,0.0f };
	}
	transform.rotate = { 0.0f,0.0f,rotation };
	transform.scale = { size.x,size.y,1.0f };

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	Matrix4x4 worldProjectionMatrix = (worldMatrix * (viewMatrix * projectionMatrix));
	transformationMatrixData->WVP = worldProjectionMatrix;
	transformationMatrixData->World = worldMatrix;
}

void Sprite::Draw(bool isBackMost)
{

	Update(isBackMost);

	// Spriteの描画。変更が必要な物だけ変更する
	spriteCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
	// IBVを設定
	spriteCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);
	// マテリアルCBufferの場所を設定
	spriteCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	// TransformationMatrixCBufferの場所を設定
	spriteCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
	// 使うSRVの切り替え
	srvManager_ = TextureManager::GetInstance()->GetSrvManager();
	srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(fullpath));
	// 描画！(DrawCall/ドローコール)
	spriteCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::SetTexturePath(std::string textureFilePath)
{
    fullpath = textureFilePath;
    TextureManager::GetInstance()->LoadTexture(textureFilePath);
	TextureManager::GetInstance()->GetTextureIndexByFilePath(fullpath);

}

void Sprite::CreateVartexData()
{
	// Sprite用の頂点リソースを作る
	vertexResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * 6);
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// 頂点データの設定
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));


	// Index用のリソースを作る（sprite用）
	indexResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);
	// リソースの先頭アドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// インデックスリソースにデータを書き込む（sprite用）
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

}

void Sprite::CreateMaterial()
{
	// Sprite用のマテリアルリソースをつくる
	materialResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 色の設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	// Lightingの設定
	materialData->uvTransform = MakeIdentity4x4();
}

void Sprite::CreateTransformationMatrix()
{
	transformationMatrixResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	// 書き込むかめのアドレスを取得
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	// 単位行列を書き込んでおく
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
}

void Sprite::AdjustTextureSize()
{
	// テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(fullpath);

	textureSize.x = static_cast<float>(metadata.width);
	textureSize.y = static_cast<float>(metadata.height);
	// 画像サイズをテクスチャサイズに合わせる
	size = textureSize;
}

#pragma once
#include"d3d12.h"
#include"Vector4.h"
#include"Vector2.h"
#include"Vector3.h"
#include "Matrix4x4.h"
#include"wrl.h"
#include"string"
#include "Srv/SrvManager.h"
class SpriteCommon;
class Sprite
{
public: // メンバ関数

	/// <summary>
	/// 
	/// </summary>
	/// <param name="textureFilePath">テクスチャハンドル</param>
	/// <param name="position">座標</param>
	/// <param name="color">色</param>
	/// <param name="anchorpoint">アンカーポイント</param>
	/// <param name="isFlipX">左右反転</param>
	/// <param name="isFlipY">上下反転</param>
	void Initialize(const std::string& textureFilePath, Vector2 position, Vector4 color = { 1,1,1,1 }, Vector2 anchorpoint = { 0.0f,0.0f }, bool isFlipX = false, bool isFlipY = false);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(bool isBackMost = false);

	/// <summary>
	/// getter
	/// </summary>
	/// <returns></returns>
	const Vector2& GetPosition()const { return position_; }
	float GetRotation() const { return rotation; }
	const Vector2& GetSize() const { return size; }
	const Vector4& GetColor()const { return materialData->color; }
	const Vector2& GetAnchorPoint()const { return anchorPoint_; }
	const bool GetFlipX()const { return isFlipX_; }
	const bool GetFilpY()const { return isFlipY_; }
	const Vector2& GetTexLeftTop()const { return textureLeftTop; }
	const Vector2& GetTexSize()const { return textureSize; }

	/// <summary>
	/// setter
	/// </summary>
	/// <param name="position"></param>
	void SetPosition(const Vector2& position) { this->position_ = position; }
	void SetRotation(float rotation) { this->rotation = rotation; }
	void SetSize(const Vector2& size) { this->size = size; }
	void SetColor(const Vector3& color) { materialData->color.x = color.x, materialData->color.y = color.y, materialData->color.z = color.z; }
	void SetAlpha(const float& alpha) { materialData->color.w = alpha; }
	void SetTexturePath(std::string textureFilePath);
	void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint_ = anchorPoint; }
	void SetFlipX(bool isFlipX) { isFlipX_ = isFlipX; }
	void SetFlipY(bool isFlipY) { isFlipY_ = isFlipY; }
	void SetTexLeftTop(const Vector2& textureLeftTop) { this->textureLeftTop = textureLeftTop; }
	void SetTexSize(const Vector2& textureSize) { this->textureSize = textureSize; }
	void SetUVTransform(const Matrix4x4& uvTransform) { materialData->uvTransform = uvTransform; }

private: // メンバ関数

	/// <summary>
	/// 更新
	/// </summary>
	void Update(bool isbackmost_);

	/// <summary>
	/// 頂点データ作成
	/// </summary>
	void CreateVartexData();

	/// <summary>
	/// マテリアルデータ作成
	/// </summary>
	void CreateMaterial();

	/// <summary>
	/// 座標変換行列データ作成
	/// </summary>
	void CreateTransformationMatrix();

	/// <summary>
	/// テクスチャサイズをイメージに合わせる
	/// </summary>
	void AdjustTextureSize();
private:

	struct Transform {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};

	// 頂点データ
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};

	// マテリアルデータ
	struct Material {
		Vector4 color;
		Matrix4x4 uvTransform;
		float padding[3];
	};

	// 座標変換行列データ
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	SpriteCommon* spriteCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
	// バッファリソース内のデータを指すポインタ
	Material* materialData = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
	// バッファリソース内のデータを指すポインタ
	TransformationMatrix* transformationMatrixData = nullptr;

	// 移動させる用各SRT
	Vector2 position_ = { 0.0f,0.0f };
	float rotation = 0.0f;
	Vector2 size = { 640.0f,360.0f };

	//std::string directoryPath_ = "resources/images";
	std::string fullpath;
	Vector2 anchorPoint_ = { 0.0f,0.0f };

	// 左右フリップ
	bool isFlipX_ = false;

	// 上下フリップ
	bool isFlipY_ = false;

	bool isbackmost_ = false;

	// テクスチャ左上座標
	Vector2 textureLeftTop = { 0.0f,0.0f };
	// テクスチャ切り出しサイズ
	Vector2 textureSize = { 512.0f,512.0f };

};


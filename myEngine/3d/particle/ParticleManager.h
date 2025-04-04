#pragma once
#include"SrvManager.h"
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include <Matrix4x4.h>
#include"ViewProjection.h"
#include <WorldTransform.h>
#include"random"
#include"ParticleCommon.h"
class ParticleManager
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(SrvManager* srvManager);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(const ViewProjection& viewProjeciton);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// パーティクルグループの生成
	/// </summary>
	/// <param name="name"></param>
	/// <param name="textureFilePath"></param>
	void CreateParticleGroup(const std::string name, const std::string& filename);

	std::string GetTexturePath() { return modelData.material.textureFilePath; }

	void SetBillBorad(bool isBillBoard) { isBillboard = isBillBoard; }
	void SetRandomRotate(bool isRandomRotate) { isRandomRotate_ = isRandomRotate; }
	void SetAcceMultipy(bool isAcceMultipy) { isAcceMultipy_ = isAcceMultipy; }
	void SetRandomSize(bool isRandomSize) { isRandomSize_ = isRandomSize; }
	void SetAllRandomSize(bool isAllRandomSize) { isRandomAllSize_ = isAllRandomSize; }
	void SetSinMove(bool isSinMove) { isSinMove_ = isSinMove; }
	void SetFaceDirection(bool flag) { isFaceDirection_ = flag; }
	void SetTexture(const std::string& filePath);
	void SetColor(const Vector4& color) { materialData->color = color; }

private:
	/// <summary>
	/// 頂点データ作成
	/// </summary>
	void CreateVartexData(const std::string& filename);

private:

	struct ParticleForGPU {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

	// 頂点データ
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};

	struct MaterialData
	{
		std::string textureFilePath;
		uint32_t textureIndex;
	};

	struct ModelData
	{
		std::vector<VertexData> vertices;
		MaterialData material;
	};

	struct Particle {
		WorldTransform transform; // 位置
		Vector3 velocity;  // 速度
		Vector3 Acce;
		Vector3 startScale;
		Vector3 endScale;
		Vector3 startAcce;
		Vector3 endAcce;
		Vector3 startRote;
		Vector3 endRote;
		Vector3 rotateVelocity;
		Vector3 fixedDirection;
		Vector4 color;     // 色
		float lifeTime;    // ライフタイム
		float currentTime; // 現在の時間
		float initialAlpha;
	};

	// マテリアルデータ
	struct Material {
		Vector4 color;
		Matrix4x4 uvTransform;
		float padding[3];
	};

	ModelData modelData;

	struct ParticleGroup {
		// マテリアルデータ
		MaterialData material;
		// パーティクルのリスト (std::list<Particle> 型)
		std::list<Particle> particles;
		// インスタンシングデータ用SRVインデックス
		uint32_t instancingSRVIndex = 0;
		// インスタンシングリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = nullptr;
		// インスタンス数
		uint32_t instanceCount = 0;
		// インスタンシングデータを書き込むためのポインタ
		ParticleForGPU* instancingData = nullptr;
	};

	ParticleCommon* particleCommon = nullptr;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
	// バッファリソース内のデータを指すポインタ
	Material* materialData = nullptr;

	SrvManager* srvManager_;
	static std::unordered_map<std::string, ModelData> modelCache;
	std::unordered_map<std::string, ParticleGroup>particleGroups;

	// Δtを定義
	static const uint32_t kNumMaxInstance = 10000; // 最大インスタンス数の制限

	std::random_device seedGenerator;
	std::mt19937 randomEngine;

	bool isBillboard = false;
	bool isRandomRotate_ = false;
	bool isAcceMultipy_ = false;
	bool isRandomSize_ = false;
	bool isRandomAllSize_ = false;
	bool isSinMove_ = false;
	bool isFaceDirection_ = false;

public:
	// nameで指定した名前のパーティクルグループにパーティクルを発生させる関数
	std::list<Particle> Emit(const std::string name, const Vector3& position, uint32_t count, const Vector3& scale,
		const Vector3& velocityMin, const Vector3& velocityMax, float lifeTimeMin, float lifeTimeMax,
		const Vector3& particleStartScale, const Vector3& particleEndScale, const Vector3& startAcce, const Vector3& endAcce,
		const Vector3& startRote, const Vector3& endRote, bool isRandomColor, float alphaMin, float alphaMax,
		const Vector3& rotateVelocityMin, const Vector3& rotateVelocityMax,
		const Vector3& allScaleMax, const Vector3& allScaleMin,
		const float& scaleMin, const float& scaleMax, const Vector3& rotation);


private:
	/// <summary>
	/// .mtlファイルの読み取り
	/// </summary>
	/// <param name="directoryPath"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	///  .objファイルの読み取り
	/// </summary>
	/// <param name="directoryPath"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);


	/// <summary>
	/// マテリアルデータ作成
	/// </summary>
	void CreateMaterial();

	Particle MakeNewParticle(std::mt19937& randomEngine,
		const Vector3& translate,
		const Vector3& rotation,
		const Vector3& scale,
		const Vector3& velocityMin, const Vector3& velocityMax,
		float lifeTimeMin, float lifeTimeMax, const Vector3& particleStartScale, const Vector3& particleEndScale,
		const Vector3& startAcce, const Vector3& endAcce, const Vector3& startRote, const Vector3& endRote
		, bool isRamdomColor, float alphaMin, float alphaMax, const Vector3& rotateVelocityMin, const Vector3& rotateVelocityMax,
		const Vector3& allScaleMax, const Vector3& allScaleMin,
		const float& scaleMin, const float& scaleMax);
};


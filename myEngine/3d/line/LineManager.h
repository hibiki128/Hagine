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
class LineManager
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(SrvManager* srvManager);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(const ViewProjection& viewProjection, const std::vector<Vector3>& startPoints, const std::vector<Vector3>& endPoints);

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
		Vector3 normal;
	};

	struct MaterialData
	{
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	struct ModelData
	{
		std::vector<VertexData> vertices;
		MaterialData material;
	};

	struct Line {
		WorldTransform transform; // 位置
		Vector4 color;     // 色
		Vector3 size;        // 現在のサイズ
	};

	// マテリアルデータ
	struct Material {
		Vector4 color;
		Matrix4x4 uvTransform;
		float padding[3];
	};

	ModelData modelData;

	struct LineGroup {
		// マテリアルデータ
		MaterialData material;
		// パーティクルのリスト (std::list<Particle> 型)
		std::list<Line> lines;
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

	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
	ParticleForGPU* instancingData = nullptr;

	SrvManager* srvManager_;

	std::unordered_map<std::string, LineGroup>lineGroups;

	// Δtを定義
	const float kDeltaTime = 1.0f / 60.0f;
	static const uint32_t kNumMaxInstance = 10000; // 最大インスタンス数の制限

	std::random_device seedGenerator;
	std::mt19937 randomEngine;

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
};


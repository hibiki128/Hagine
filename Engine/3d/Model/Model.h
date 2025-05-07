#pragma once
#include"ModelCommon.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Vector2.h"
#include"Srv/SrvManager.h"
#include"assimp/Importer.hpp"
#include"assimp/scene.h"
#include"assimp/postprocess.h"
#include "map"
#include"Quaternion.h"
#include "span"
#include"array"
#include"animation/Animator.h"
#include"animation/Bone.h"
#include"animation/Skin.h"
#include"ModelStructs.h"

#include <unordered_set>
#include <Primitive/PrimitiveModel.h>
class Model
{
private:
	ModelCommon* modelCommon_;

	// Objファイルのデータ
	ModelData modelData;
	SrvManager* srvManager_;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
	uint32_t* indexData;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	std::string filename_;
	std::string directorypath_;

	bool isGltf;

	Matrix4x4 localMatrix;

	Animator* animator_;
	Skin* skin_;
	Bone* bone_;
	static std::unordered_set<std::string> jointNames;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="modelCommon"></param>
	void CreateModel(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename);

	void CreatePrimitiveModel(ModelCommon *modelCommon, const PrimitiveType &type);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	void SetSrv(SrvManager* srvManager) { srvManager_ = srvManager; }
	void SetAnimator(Animator* animator) { animator_ = animator; }
	void SetSkin(Skin* skin) { skin_ = skin; }
	void SetBone(Bone* bone) { bone_ = bone; }
	void SetTextureIndex(const std::string& filePath);
	void SetMaterialData(const MaterialData& materialData) { modelData.material = materialData; }
	MaterialData GetMaterialData() { return modelData.material; }

	ModelData GetModelData() { return modelData; }

	bool IsGltf() { return isGltf; }

private:

	/// <summary>
	/// 頂点データ作成
	/// </summary>
	void CreateVartexData();

	/// <summary>
	/// indexの作成
	/// </summary>
	void CreateIndexResource();

	/// <summary>
	/// .mtlファイルの読み取り
	/// </summary>
	/// <param name="directoryPath"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	///  .objファイルの読み取り
	/// </summary>
	/// <param name="directoryPath"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// ノード読み取り
	/// </summary>
	/// <param name="node"></param>
	/// <returns></returns>
	static Node ReadNode(aiNode* node);
};


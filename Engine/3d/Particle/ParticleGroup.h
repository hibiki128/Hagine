#pragma once
#include "Model/Model.h"
#include "Primitive/PrimitiveModel.h"
#include <Model/ModelStructs.h>
#include"ParticleCommon.h"
#include <Transform/WorldTransform.h>
#include <list>
class ParticleGroup {
  public:
    struct ParticleMaterial {
        Vector4 color;
        Matrix4x4 uvTransform;
        float padding[3];
    };
  public:
    void Initialize();

    void Update();

    ParticleGroupData CreateParticleGroup(const std::string &groupName, const std::string &filename, const std::string &texturePath = {});
    ParticleGroupData CreatePrimitiveParticleGroup(const std::string &groupName, PrimitiveType type, const std::string &texturePath = {});

    const std::string GetGroupName() { return particleGroupData_.groupName; }

    uint32_t GetMaxInstance() { return kNumMaxInstance; }

    ParticleGroupData &GetParticleGroupData() { return particleGroupData_; }

    std::string &GetTexturePath(uint32_t index) { return particleGroupData_.materials[index].textureFilePath; }
    std::string &GetModelPath() { return modelFilePath_; }

    D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() { return vertexBufferView; }
    D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView() { return indexBufferView; }

    ModelData GetModelData() { return modelData; }

    MaterialData GetMaterialData(uint32_t index) { return modelData.materials[index]; }

    PrimitiveType GetPrimitiveType() { return type_; }

    Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexResource() { return vertexResource; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetmaterialResource() { return materialResource; }

  private:
    void CreateVertexData();
    void CreateMaterial();
    void CreateIndexResource();

  private:
    static std::unordered_map<std::string, ModelData> modelCache;
    static const uint32_t kNumMaxInstance = 10000; // 最大インスタンス数の制限

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
    // バッファリソース内のデータを指すポインタ
    VertexData *vertexData = nullptr;
    // バッファリソースの使い道を補足するバッファビュー
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    // バッファリソース内のデータを指すポインタ
    ParticleMaterial *materialData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
    uint32_t *indexData;
    // バッファリソースの使い道を補足するバッファビュー
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    Model *model_;
    ModelData modelData;
    ParticleGroupData particleGroupData_;
    PrimitiveType type_;
    std::string modelFilePath_;
};

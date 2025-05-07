#include "ParticleGroup.h"
#include "Model/ModelManager.h"
#include "Srv/SrvManager.h"
#include "fstream"
#include <Texture/TextureManager.h>

std::unordered_map<std::string, ModelData> ParticleGroup::modelCache;

void ParticleGroup::Initialize() {
}

void ParticleGroup::Update() {
}

ParticleGroupData ParticleGroup::CreateParticleGroup(const std::string &groupName, const std::string &filename, const std::string &texturePath) {
    particleGroupData_.groupName = groupName;
    particleGroupData_.material.modelFilePath = filename;
    ModelManager::GetInstance()->LoadModel(filename);
    model_ = ModelManager::GetInstance()->FindModel(filename);
    modelData = model_->GetModelData();
    //modelData = LoadObjFile("resources/models/", filename);
    CreateVertexData();
    CreateIndexResource();
    if (texturePath.empty()) {
        particleGroupData_.material.textureFilePath = modelData.material.textureFilePath;
    } else {
        particleGroupData_.material.textureFilePath = texturePath;
    }
    TextureManager::GetInstance()->LoadTexture(particleGroupData_.material.textureFilePath);
    particleGroupData_.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(particleGroupData_.material.textureFilePath);
    particleGroupData_.instancingResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
    particleGroupData_.instancingSRVIndex = SrvManager::GetInstance()->Allocate() + 1;
    particleGroupData_.instancingResource->Map(0, nullptr, reinterpret_cast<void **>(&particleGroupData_.instancingData));

    SrvManager::GetInstance()->CreateSRVforStructuredBuffer(particleGroupData_.instancingSRVIndex, particleGroupData_.instancingResource.Get(), kNumMaxInstance, sizeof(ParticleForGPU));

    CreateMaterial();
    particleGroupData_.instanceCount = 0;
    return particleGroupData_;
}

ParticleGroupData ParticleGroup::CreatePrimitiveParticleGroup(const std::string &groupName, PrimitiveType type, const std::string &texturePath) {
    particleGroupData_.groupName = groupName;
    type_ = type;
    model_ = ModelManager::GetInstance()->FindModel(ModelManager::GetInstance()->CreatePrimitiveModel(type));
    modelData = model_->GetModelData();
    CreateVertexData();
    CreateIndexResource();
    if (texturePath.empty()) {
        particleGroupData_.material.textureFilePath = modelData.material.textureFilePath;
    } else {
        particleGroupData_.material.textureFilePath = texturePath;
    }
    TextureManager::GetInstance()->LoadTexture(particleGroupData_.material.textureFilePath);
    particleGroupData_.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(particleGroupData_.material.textureFilePath);
    particleGroupData_.instancingResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
    particleGroupData_.instancingSRVIndex = SrvManager::GetInstance()->Allocate() + 1;
    particleGroupData_.instancingResource->Map(0, nullptr, reinterpret_cast<void **>(&particleGroupData_.instancingData));

    SrvManager::GetInstance()->CreateSRVforStructuredBuffer(particleGroupData_.instancingSRVIndex, particleGroupData_.instancingResource.Get(), kNumMaxInstance, sizeof(ParticleForGPU));

    CreateMaterial();
    particleGroupData_.instanceCount = 0;
    return particleGroupData_;
}

void ParticleGroup::CreateVertexData() {

    // 頂点リソースを作る
    vertexResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
    // 頂点バッファビューを作成する
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();            // リソースの先頭アドレスから使う
    vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
    vertexBufferView.StrideInBytes = sizeof(VertexData);                                 // 1頂点当たりのサイズ

    // 頂点リソースにデータを書き込む
    vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData)); // 書き込むためのアドレスを取得
    std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void ParticleGroup::CreateMaterial() {
    // Sprite用のマテリアルリソースをつくる
    materialResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(Material));
    // 書き込むためのアドレスを取得
    materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
    // 色の設定
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    // Lightingの設定
    materialData->uvTransform = MakeIdentity4x4();
}

void ParticleGroup::CreateIndexResource() {
    indexResource = ParticleCommon::GetInstance()->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * modelData.indices.size());
    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * modelData.indices.size());
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
    std::memcpy(indexData, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());
}

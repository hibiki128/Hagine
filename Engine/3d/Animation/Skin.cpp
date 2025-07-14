#include "Skin.h"
#include "algorithm"
#include <DirectXCommon.h>
#include <Graphics/Srv/SrvManager.h>
#include <cassert>
#include <myMath.h>

void Skin::Initialize(const Skeleton &skeleton, const ModelData &modelData) {
    dxCommon_ = DirectXCommon::GetInstance();
    srvManager_ = SrvManager::GetInstance();
    skinCluster_ = CreateSkinCluster(skeleton, modelData);
}

void Skin::Update(const Skeleton &skeleton) {
    for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
        assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
        skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
            skinCluster_.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
        skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
            Transpose(Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));
    }
}

void Skin::UpdateInputVertices(const ModelData &modelData) {
    // 入力頂点データを更新
   
    for (const auto &mesh : modelData.meshes) {
        // メッシュごとの頂点データをコピー
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            if (vertexOffset + i < totalVertexCount) {
                skinCluster_.mappedVertex[vertexOffset + i] = mesh.vertices[i];
            }
        }
        vertexOffset += mesh.vertices.size();
    }
}

void Skin::ExecuteSkinning(ID3D12GraphicsCommandList *commandList) {
    // リソースをバインド
    // t0: MatrixPalette
    commandList->SetComputeRootDescriptorTable(0, skinCluster_.paletteSrvHandle.second);

    // t1: InputVertices
    commandList->SetComputeRootDescriptorTable(1, skinCluster_.inputVertexSrvHandle.second);

    // t2: Influences
    commandList->SetComputeRootDescriptorTable(2, skinCluster_.influenceSrvHandle.second);

    // u0: OutputVertices
    commandList->SetComputeRootDescriptorTable(3, skinCluster_.outputVertexSrvHandle.second);

    // b0: SkinningInformation
    commandList->SetComputeRootConstantBufferView(4,
                                                  skinCluster_.skinningInformationResource->GetGPUVirtualAddress());

    // Dispatch実行
    uint32_t numGroups = (static_cast<uint32_t>(totalVertexCount) + 1023) / 1024;
    commandList->Dispatch(numGroups, 1, 1);
}

SkinCluster Skin::CreateSkinCluster(const Skeleton &skeleton, const ModelData &modelData) {
    SkinCluster skinCluster;

    // --- マルチメッシュ対応: 全頂点数を集計 ---

    for (const auto &mesh : modelData.meshes) {
        totalVertexCount += mesh.vertices.size();
    }

    CreatePaletteResource(skinCluster, skeleton);

    CreateInfluenceResource(skinCluster, skeleton);

    CreateInputVertexResource(skinCluster, skeleton);

    CreateOutputVertexResource(skinCluster, skeleton);

    CreateSkinningInformationResource(skinCluster, skeleton);

    // InverseBindPoseMatrixの保存領域を作成
    skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
    std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), []() { return MakeIdentity4x4(); });

    // --- マルチメッシュ・マルチマテリアル対応: 各メッシュごとにオフセットを管理 ---
    std::vector<size_t> meshVertexOffsets;
    size_t vertexOffset = 0;
    meshVertexOffsets.reserve(modelData.meshes.size());
    for (const auto &mesh : modelData.meshes) {
        meshVertexOffsets.push_back(vertexOffset);
        vertexOffset += mesh.vertices.size();
    }

    // ModelDataのSkinCluster情報を解析してInfluenceの中身を埋める
    for (const auto &jointWeight : modelData.skinClusterData) {
        auto it = skeleton.jointMap.find(jointWeight.first);
        if (it == skeleton.jointMap.end()) {
            continue;
        }
        skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
        for (const auto &vertexWeight : jointWeight.second.vertexWeights) {
            // vertexWeight.meshIndex, vertexWeight.vertexIndex を想定
            size_t meshIndex = vertexWeight.meshIndex;
            size_t localVertexIndex = vertexWeight.vertexIndex;
            if (meshIndex >= meshVertexOffsets.size())
                continue;
            size_t globalVertexIndex = meshVertexOffsets[meshIndex] + localVertexIndex;
            if (globalVertexIndex >= totalVertexCount)
                continue;
            auto &currentInfluence = skinCluster.mappedInfluence[globalVertexIndex];
            for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
                if (currentInfluence.weights[index] == 0.0f) {
                    currentInfluence.weights[index] = vertexWeight.weight;
                    currentInfluence.jointIndices[index] = (*it).second;
                    break;
                }
            }
        }
    }

    meshVertexOffsets_.clear();
    size_t offset = 0;
    for (const auto &mesh : modelData.meshes) {
        meshVertexOffsets_.push_back(offset);
        offset += mesh.vertices.size();
    }

    return skinCluster;
}

void Skin::CreatePaletteResource(SkinCluster &skinCluster, const Skeleton &skeleton) {
    // palette用のResourceを確保
    skinCluster.paletteResource = dxCommon_->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());
    WellForGPU *mappedPalette = nullptr;
    skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedPalette));
    skinCluster.mappedPalette = {mappedPalette, skeleton.joints.size()};
    skinClusterPaletteSrvIndex_ = srvManager_->Allocate() + 1;
    skinCluster.paletteSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterPaletteSrvIndex_);
    skinCluster.paletteSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterPaletteSrvIndex_);

    // palette用のSRVを作成
    srvManager_->CreateSRVforStructuredBuffer(skinClusterPaletteSrvIndex_, skinCluster.paletteResource.Get(), UINT(skeleton.joints.size()), sizeof(WellForGPU));
}

void Skin::CreateInfluenceResource(SkinCluster &skinCluster, const Skeleton &skeleton) {

    // influence用のResourceを確保（全メッシュ分）
    skinCluster.influenceResource = dxCommon_->CreateBufferResource(sizeof(VertexInfluence) * totalVertexCount);
    VertexInfluence *mappedInfluence = nullptr;
    skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedInfluence));
    std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * totalVertexCount);
    skinCluster.mappedInfluence = {mappedInfluence, totalVertexCount};
    skinClusterInfluenceSrvIndex_ = srvManager_->Allocate() + 1;
    skinCluster.influenceSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterInfluenceSrvIndex_);
    skinCluster.influenceSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterInfluenceSrvIndex_);

    srvManager_->CreateSRVforStructuredBuffer(skinClusterInfluenceSrvIndex_, skinCluster.influenceResource.Get(), UINT(totalVertexCount), sizeof(VertexInfluence));
}

void Skin::CreateInputVertexResource(SkinCluster &skinCluster, const Skeleton &skeleton) {

    // inputVertex用のResourceを確保
    skinCluster.inputVertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * totalVertexCount);
    VertexData *mappedVertex = nullptr;
    skinCluster.inputVertexResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedVertex));
    skinCluster.mappedVertex = {mappedVertex, totalVertexCount};
    skinClusterInputVertexSrvIndex_ = srvManager_->Allocate() + 1;
    skinCluster.inputVertexSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterInputVertexSrvIndex_);
    skinCluster.inputVertexSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterInputVertexSrvIndex_);

    srvManager_->CreateSRVforStructuredBuffer(skinClusterInputVertexSrvIndex_, skinCluster.inputVertexResource.Get(), UINT(totalVertexCount), sizeof(VertexData));
}

void Skin::CreateOutputVertexResource(SkinCluster &skinCluster, const Skeleton &skeleton) {

    // outPutVertex用のResourceを確保
    skinCluster.outputVertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * totalVertexCount, true);
    skinClusterOutputVertexSrvIndex_ = srvManager_->Allocate() + 1;
    skinCluster.outputVertexSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterOutputVertexSrvIndex_);
    skinCluster.outputVertexSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterOutputVertexSrvIndex_);

    srvManager_->CreateUAVStructuredBuffer(skinClusterOutputVertexSrvIndex_, skinCluster.outputVertexResource.Get(), static_cast<uint32_t>(totalVertexCount), sizeof(VertexData));

    skinCluster.outputVertexBufferView.BufferLocation = skinCluster.outputVertexResource->GetGPUVirtualAddress();
    skinCluster.outputVertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * totalVertexCount);
    skinCluster.outputVertexBufferView.StrideInBytes = sizeof(VertexData);
}

void Skin::CreateSkinningInformationResource(SkinCluster &skinCluster, const Skeleton &skeleton) {

    // SkinningInformation用のResourceを確保
    skinCluster.skinningInformationResource = dxCommon_->CreateBufferResource(sizeof(SkinningInformationForGPU));
    skinCluster.SkinningInfomationData = nullptr;
    skinCluster.skinningInformationResource->Map(0, nullptr, reinterpret_cast<void **>(&skinCluster.SkinningInfomationData));
    skinCluster.SkinningInfomationData->numVertices = static_cast<uint32_t>(totalVertexCount);
}

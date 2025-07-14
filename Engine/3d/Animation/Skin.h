#pragma once
#include "Model/ModelStructs.h"
#include <cstdint>
class DirectXCommon;
class SrvManager;
class Skin {
  private:
    SkinCluster skinCluster_;
    uint32_t skinClusterPaletteSrvIndex_ = 0;
    uint32_t skinClusterInfluenceSrvIndex_ = 0;
    uint32_t skinClusterOutputVertexSrvIndex_ = 0;
    uint32_t skinClusterInputVertexSrvIndex_ = 0;

    size_t totalVertexCount = 0;
    size_t vertexOffset = 0;

    DirectXCommon *dxCommon_;
    SrvManager *srvManager_;

    std::vector<size_t> meshVertexOffsets_;
  public:
    void Initialize(const Skeleton &skeleton, const ModelData &modelData);
    void Update(const Skeleton &skeleton);
    uint32_t GetPaletteSrvIndex() { return skinClusterPaletteSrvIndex_; }
    uint32_t GetInfluenceSrvIndex() { return skinClusterInfluenceSrvIndex_; }
    uint32_t GetInputVertexSrvIndex() { return skinClusterInputVertexSrvIndex_; }
    uint32_t GetOutputVertexSrvIndex() { return skinClusterOutputVertexSrvIndex_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetSkinningInformationResource() { return skinCluster_.skinningInformationResource; }

    // 出力頂点バッファのリソースを取得
    ID3D12Resource *GetOutputVertexResource() { return skinCluster_.outputVertexResource.Get(); }

    // 出力頂点バッファビューを取得
    D3D12_VERTEX_BUFFER_VIEW GetOutputVertexBufferView() { return skinCluster_.outputVertexBufferView; }

    uint32_t GetTotalVertex() { return static_cast<uint32_t>(totalVertexCount); }

    SkinCluster GetSkinCluster() { return skinCluster_; }

    void UpdateInputVertices(const ModelData &modelData);
    void ExecuteSkinning(ID3D12GraphicsCommandList *commandList);

    size_t GetMeshVertexOffset(size_t meshIndex) const {
        if (meshIndex < meshVertexOffsets_.size()) {
            return meshVertexOffsets_[meshIndex];
        }
        return 0;
    }

  private:
    /// <summary>
    /// SkinClusterの生成
    /// </summary>
    /// <param name="device"></param>
    /// <param name="skeleton"></param>
    /// <param name="modelData"></param>
    /// <param name="descriptorHeap"></param>
    /// <param name="descriptorSize"></param>
    /// <returns></returns>
    SkinCluster CreateSkinCluster(const Skeleton &skeleton, const ModelData &modelData);

    void CreatePaletteResource(SkinCluster &skinCluster, const Skeleton &skeleton);
    void CreateInfluenceResource(SkinCluster &skinCluster, const Skeleton &skeleton);
    void CreateInputVertexResource(SkinCluster &skinCluster, const Skeleton &skeleton);
    void CreateOutputVertexResource(SkinCluster &skinCluster, const Skeleton &skeleton);
    void CreateSkinningInformationResource(SkinCluster &skinCluster, const Skeleton &skeleton);
};

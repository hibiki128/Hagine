#pragma once
#include "wrl.h"
#include <ModelStructs.h>
#include <Primitive/PrimitiveModel.h>
#include <d3d12.h>
class DirectXCommon;
class Mesh {

  public:
    /// ==========================================
    /// public methods
    /// ==========================================

    void Initialize();
    void PrimitiveInitialize(const PrimitiveType &type);

    void Draw();

    MeshData GetMeshData() { return meshData_; }
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() { return vertexBufferView; }
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() { return indexBufferView; }

  private:
    /// ==========================================
    /// private variaus
    /// ==========================================

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
    // バッファリソース内のデータを指すポインタ
    VertexData *vertexData = nullptr;
    // バッファリソースの使い道を補足するバッファビュー
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
    uint32_t *indexData;
    // バッファリソースの使い道を補足するバッファビュー
    D3D12_INDEX_BUFFER_VIEW indexBufferView;

    MeshData meshData_;
    DirectXCommon *dxCommon_;

  private:
    /// ==========================================
    /// private methods
    /// ==========================================

    /// <summary>
    /// 頂点データ作成
    /// </summary>
    void CreateVartexData();

    /// <summary>
    /// indexの作成
    /// </summary>
    void CreateIndexResource();
};

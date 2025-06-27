#include "Mesh.h"
#include "DirectXCommon.h"
void Mesh::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();

    CreateVartexData();
    CreateIndexResource();
}

void Mesh::PrimitiveInitialize(const PrimitiveType &type) {
    meshData_.vertices = PrimitiveModel::GetInstance()->GetPrimitiveData(type).vertices;
    meshData_.indices = PrimitiveModel::GetInstance()->GetPrimitiveData(type).indices;
}

void Mesh::CreateVartexData() {
    vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * meshData_.vertices.size());
    // リソースの先頭のアドレスから使う
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    // 使用するリソースのサイズは頂点6つ分のサイズ
    vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * meshData_.vertices.size());
    // 1頂点あたりのサイズ
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    // 頂点データの設定
    vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

    std::memcpy(vertexData, meshData_.vertices.data(), sizeof(VertexData) * meshData_.vertices.size());
}

void Mesh::CreateIndexResource() {
    indexResource = dxCommon_->CreateBufferResource(sizeof(uint32_t) * meshData_.indices.size());
    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * meshData_.indices.size());
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
    std::memcpy(indexData, meshData_.indices.data(), sizeof(uint32_t) * meshData_.indices.size());
}

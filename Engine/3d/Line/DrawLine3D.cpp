#include "DrawLine3D.h"
#include "DirectXCommon.h"

DrawLine3D *DrawLine3D::instance = nullptr;

DrawLine3D *DrawLine3D::GetInstance() {
    if (instance == nullptr) {
        instance = new DrawLine3D();
    }
    return instance;
}

std::unique_ptr<DrawLine3D::LineData> DrawLine3D::CreateMesh(UINT vertexCount, UINT indexCount) {

    std::unique_ptr<LineData> mesh = std::make_unique<LineData>();

    UINT vertBufferSize = sizeof(VertexPosColor) * vertexCount;
    mesh->vertBuffer = dxCommon->CreateBufferResource(vertBufferSize);

    mesh->vbView.BufferLocation = mesh->vertBuffer->GetGPUVirtualAddress();
    mesh->vbView.StrideInBytes = sizeof(VertexPosColor);
    mesh->vbView.SizeInBytes = vertBufferSize;

    D3D12_RANGE readRange = {0, 0};
    mesh->vertBuffer->Map(0, &readRange, reinterpret_cast<void **>(&mesh->vertMap));

    UINT indexBufferSize = sizeof(uint16_t) * indexCount;
    if (indexCount > 0) {
        mesh->indexBuffer = dxCommon->CreateBufferResource(indexBufferSize);

        mesh->ibView.BufferLocation = mesh->indexBuffer->GetGPUVirtualAddress();
        mesh->ibView.Format = DXGI_FORMAT_R16_UINT;
        mesh->ibView.SizeInBytes = indexBufferSize;

        mesh->indexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&mesh->indexMap));
    }

    return mesh;
}

void DrawLine3D::Initialize() {
    dxCommon = DirectXCommon::GetInstance();
    CreateMeshes();
    CreateResource();
    psoManager_ = PipeLineManager::GetInstance();
}

void DrawLine3D::Finalize() {
    delete instance;
    instance = nullptr;
}

void DrawLine3D::SetPoints(const Vector3 &p1, const Vector3 &p2, const Vector4 &color) {

    if (indexLine_ < kMaxLineCount) {
        line_->vertMap[indexLine_ * 2] = {p1, color};
        line_->vertMap[indexLine_ * 2 + 1] = {p2, color};

        indexLine_++;
    }
}

void DrawLine3D::Reset() {
    indexLine_ = 0;
}

void DrawLine3D::Draw(const ViewProjection &viewProjection) {

    if (indexLine_ == 0) {
        return;
    }
    cBufferData_->viewProject = viewProjection.matView_ * viewProjection.matProjection_;

    psoManager_->DrawCommonSetting(PipelineType::kLine3d);
    D3D12_VERTEX_BUFFER_VIEW vbView = line_->vbView;
    dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
    dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, cBufferResource_->GetGPUVirtualAddress());
    dxCommon->GetCommandList()->DrawInstanced(indexLine_ * 2, 1, 0, 0);

    Reset();
}

void DrawLine3D::CreateMeshes() {

    const UINT maxVertex = kMaxLineCount * kVertexCountLine;
    const UINT maxIndices = 20;

    line_ = CreateMesh(maxVertex, maxIndices);
}

void DrawLine3D::CreateResource() {
    cBufferResource_ = dxCommon->CreateBufferResource(sizeof(CBuffer));
    cBufferData_ = nullptr;
    cBufferResource_->Map(0, nullptr, reinterpret_cast<void **>(&cBufferData_));
    cBufferData_->viewProject = MakeIdentity4x4();
}
#include "ParticleCS.h"
#include "myMath.h"
#include <Frame.h>
#include <type/Vector3.h>

void ParticleCS::Initialize() {
    dxCommon_ = ParticleCommon::GetInstance()->GetDxCommon();
    srvManager_ = SrvManager::GetInstance();
    particleCommon_ = ParticleCommon::GetInstance();
    texManager_ = TextureManager::GetInstance();
    texManager_->LoadTexture(texPath_);
    commandList = dxCommon_->GetCommandList().Get();
    CreateOutputParticleResource();
    CreatePerViewResource();
    CreateMaterialResource();
    CreateIndexResource();
    CreateVertexResource();
    CreateEmitterSphereResource();
    CreatePerFrameResource();
    CreateFreeListIndexResource();
    CreateFreeListResource();
    InitParticle();
}

void ParticleCS::Draw(const ViewProjection &vp) {
    EmitterUpdate();
    Update();

    perViewData_->viewProjection = vp.matView_ * vp.matProjection_;
    perViewData_->billboardMatrix = vp.matView_;
    perViewData_->billboardMatrix.m[3][0] = 0.0f;
    perViewData_->billboardMatrix.m[3][1] = 0.0f;
    perViewData_->billboardMatrix.m[3][2] = 0.0f;
    perViewData_->billboardMatrix.m[3][3] = 1.0f;
    perViewData_->billboardMatrix = Inverse(perViewData_->billboardMatrix);

    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    particleCommon_->GPUDrawCommonSetting(BlendMode::kAdd);

    commandList->IASetIndexBuffer(&indexBufferView_);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->SetGraphicsRootConstantBufferView(0, perViewResource_->GetGPUVirtualAddress());
    srvManager_->SetGraphicsRootDescriptorTable(1, outputParticleSrvForVSIndex_);
    srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(texPath_));
    commandList->SetGraphicsRootConstantBufferView(3, materialResource_->GetGPUVirtualAddress());

    commandList->DrawIndexedInstanced(6, kMaxParticleCount, 0, 0, 0);
}

void ParticleCS::InitParticle() {

    srvManager_->SetDescriptorHeap();

    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());

    // InitParticle.CSの処理
    particleCommon_->ComputeInitDrawCommonSetting();
    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
    int disPatchCount = (kMaxParticleCount + threadsPerGroup_ - 1) / threadsPerGroup_;
    commandList->Dispatch(disPatchCount, 1, 1);

    dxCommon_->TransitionSRVBarrier();
}

void ParticleCS::UpdateParticleCSDisPatch() {
    // UpdateParticle.CSの処理
    particleCommon_->ComputeUpdateEmitterDrawCommonSetting();
    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
    commandList->SetComputeRootConstantBufferView(3, perFrameResource_->GetGPUVirtualAddress());
    int disPatchCount = (kMaxParticleCount + threadsPerGroup_ - 1) / threadsPerGroup_;
    commandList->Dispatch(disPatchCount, 1, 1);
}

void ParticleCS::EmitterDisPatch() {
    // EmitParticle.CSの処理
    particleCommon_->ComputeEmitterDrawCommonSetting();

    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
    commandList->SetComputeRootConstantBufferView(3, emitterSphereResource_->GetGPUVirtualAddress()); // CBV (b0)
    commandList->SetComputeRootConstantBufferView(4, perFrameResource_->GetGPUVirtualAddress());      // CBV (b0)
    int disPatchCount = (emitterSphereData_->count + threadsPerGroup_ - 1) / threadsPerGroup_;
    commandList->Dispatch(disPatchCount, 1, 1);
}

void ParticleCS::EmitterUpdate() {
    emitterSphereData_->frequencyTime += Frame::DeltaTime();
    if (emitterSphereData_->frequency <= emitterSphereData_->frequencyTime) {
        emitterSphereData_->frequencyTime -= emitterSphereData_->frequency;
        emitterSphereData_->emit = 1;
    } else {
        emitterSphereData_->emit = 0;
    }
}

void ParticleCS::Update() {

    perFrameData_->time += Frame::DeltaTime();
    perFrameData_->deltaTime = Frame::DeltaTime();

    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());
    EmitterDisPatch();
    UpdateParticleCSDisPatch();
    dxCommon_->TransitionSRVBarrier();
}

void ParticleCS::CreateOutputParticleResource() {
    outputParticleResource_ = dxCommon_->CreateBufferResource(sizeof(CSParticle) * kMaxParticleCount, true);

    // UAV用のインデックス（Compute Shader用）
    outputParticleSrvIndex_ = srvManager_->Allocate() + 1;
    outputParticleSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(outputParticleSrvIndex_);
    outputParticleSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(outputParticleSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(outputParticleSrvIndex_, outputParticleResource_.Get(), kMaxParticleCount, sizeof(CSParticle));

    // SRV用のインデックス（Vertex Shader用）
    outputParticleSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(outputParticleSrvForVSIndex_, outputParticleResource_.Get(), kMaxParticleCount, sizeof(CSParticle));
}
void ParticleCS::CreatePerViewResource() {
    perViewResource_ = dxCommon_->CreateBufferResource(sizeof(PerView));
    perViewResource_->Map(0, nullptr, reinterpret_cast<void **>(&perViewData_));
    perViewData_->viewProjection = MakeIdentity4x4();
    perViewData_->billboardMatrix = MakeIdentity4x4();
}

void ParticleCS::CreateMaterialResource() {
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleMaterial));
    materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData_->uvTransform = MakeIdentity4x4();
}

void ParticleCS::CreateIndexResource() {
    // クアッド用のインデックス（2つの三角形で四角形）
    std::vector<uint32_t> indices = {
        0, 1, 2, // 最初の三角形
        2, 1, 3  // 二番目の三角形
    };

    indexResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * indices.size());
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = UINT(sizeof(uint32_t) * indices.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));
    std::memcpy(indexData_, indices.data(), sizeof(uint32_t) * indices.size());
}

void ParticleCS::CreateEmitterSphereResource() {
    emitterSphereResource_ = dxCommon_->CreateBufferResource(sizeof(EmitterSphere));
    emitterSphereResource_->Map(0, nullptr, reinterpret_cast<void **>(&emitterSphereData_));
    emitterSphereData_->count = 10;
    emitterSphereData_->frequency = 0.5f;
    emitterSphereData_->frequencyTime = 0.0f;
    emitterSphereData_->translate = Vector3(0.0f, 0.0f, 0.0f);
    emitterSphereData_->radius = 1.0f;
    emitterSphereData_->emit = 0;
}

void ParticleCS::CreateVertexResource() {
    // クアッド用の頂点データ
    std::vector<VertexData> vertices = {
        {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // 左下
        {{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 左上
        {{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},  // 右下
        {{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}    // 右上
    };

    vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * vertices.size());
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
    std::memcpy(vertexData_, vertices.data(), sizeof(VertexData) * vertices.size());
}

void ParticleCS::CreatePerFrameResource() {
    perFrameResource_ = dxCommon_->CreateBufferResource(sizeof(PerFrame));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void **>(&perFrameData_));
    perFrameData_->time = 0.0f;
    perFrameData_->deltaTime = 0.0f;
}

void ParticleCS::CreateFreeListIndexResource() {
    freeListIndexResource_ = dxCommon_->CreateBufferResource(sizeof(int), true);

    // UAV用のインデックス（Compute Shader用）
    freeListIndexSrvIndex_ = srvManager_->Allocate() + 1;
    freeListIndexSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListIndexSrvIndex_);
    freeListIndexSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListIndexSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListIndexSrvIndex_, freeListIndexResource_.Get(), 1, sizeof(int));
}

void ParticleCS::CreateFreeListResource() {
    freeListResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * kMaxParticleCount, true);

    // UAV用のインデックス（Compute Shader用）
    freeListSrvIndex_ = srvManager_->Allocate() + 1;
    freeListSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListSrvIndex_);
    freeListSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListSrvIndex_, freeListResource_.Get(), kMaxParticleCount, sizeof(uint32_t));
}

void ParticleCS::DrawImGui() {
    ImGui::Begin("ParticleCS");
    int dragCount = int(emitterSphereData_->count);
    ImGui::DragInt("パーティクルの数", &dragCount, 1, 0, kMaxParticleCount);
    emitterSphereData_->count = uint32_t(dragCount);
    ImGui::DragFloat("発生間隔", &emitterSphereData_->frequency, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("エミッタの半径", &emitterSphereData_->radius, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat3("エミッタの座標", &emitterSphereData_->translate.x, 0.1f, -100.0f, 100.0f);
    ImGui::End();
}
#define NOMINMAX
#include "ParticleCSGroup.h"
#include <Frame.h>
#include <Graphics/Model/ModelManager.h>
#include <Line/DrawLine3D.h>
#include <d3dx12.h>
#include <implot.h>

void ParticleCSGroup::Initialize(uint32_t maxParticleCount) {
    dxCommon_ = ParticleCommon::GetInstance()->GetDxCommon();
    srvManager_ = SrvManager::GetInstance();
    particleCommon_ = ParticleCommon::GetInstance();
    texManager_ = TextureManager::GetInstance();
    commandList = dxCommon_->GetCommandList().Get();
    CreateSettingsResource();
    settingsData_->maxParticleCount = maxParticleCount;
    CreateOutputParticleResource();
    CreatePerViewResource();
    CreatePerFrameResource();
    CreateFreeListIndexResource();
    CreateFreeListTrailIndexResource();
    CreateFreeListResource();
    CreateAliveCountResource();

    isInitialized_ = true;
}

int ParticleCSGroup::CalculateOptimalEmitCount() const {
    if (frequency_ <= 0.0f || settingsData_->lifeTimeMax <= 0.0f) {
        return static_cast<int>(settingsData_->maxParticleCount);
    }

    float emissionCount = settingsData_->lifeTimeMax / frequency_;

    int result;
    if (emissionCount <= 1.0f) {
        result = static_cast<int>(settingsData_->maxParticleCount);
    } else {
        result = static_cast<int>(settingsData_->maxParticleCount / emissionCount);
    }

    return std::clamp(result, 1, static_cast<int>(settingsData_->maxParticleCount));
}

ParticleCSGroup::~ParticleCSGroup() {
    if (!isInitialized_) {
        return;
    }

    // Map済みリソースのUnmap
    if (settingsResource_) {
        settingsResource_->Unmap(0, nullptr);
    }
    if (perViewResource_) {
        perViewResource_->Unmap(0, nullptr);
    }
    if (perFrameResource_) {
        perFrameResource_->Unmap(0, nullptr);
    }
    if (materialResource_) {
        materialResource_->Unmap(0, nullptr);
    }
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
    }
}

ParticleCSGroupData ParticleCSGroup::CreateParticleGroup(const std::string &groupName, const std::string &filename, uint32_t maxParticleCount, const std::string &texturePath, BlendMode blendMode) {
    Initialize(maxParticleCount);
    particleGroupData_.groupName = groupName;
    modelFilePath_ = filename;
    ModelManager::GetInstance()->LoadModel(filename);
    model_ = ModelManager::GetInstance()->FindModel(filename);
    modelData = model_->GetModelData();
    CreateVertexResource();
    CreateIndexResource();
    // マテリアルが複数ある場合は最初のものを使う
    particleGroupData_.materials.clear();
    if (texturePath.empty()) {
        if (!modelData.materials.empty()) {
            particleGroupData_.materials = ForParticleMaterials(modelData.materials);
        } else {
            particleGroupData_.materials.push_back(ParticleMaterial{});
        }
    } else {
        ParticleMaterial mat;
        mat.textureFilePath = texturePath;
        mat.textureIndex = texManager_->GetTextureIndexByFilePath(texturePath);
        particleGroupData_.materials.push_back(mat);
    }
    // すべてのマテリアルのテクスチャをロード
    for (auto &mat : particleGroupData_.materials) {
        texManager_->LoadTexture(mat.textureFilePath);
        mat.textureIndex = texManager_->GetTextureIndexByFilePath(mat.textureFilePath);
    }

    CreateMaterialResource();

    InitParticle();
    particleGroupData_.blendMode = blendMode;
    return particleGroupData_;
}

ParticleCSGroupData ParticleCSGroup::CreatePrimitiveParticleGroup(const std::string &groupName, PrimitiveType type, uint32_t maxParticleCount, const std::string &texturePath, BlendMode blendMode) {
    Initialize(maxParticleCount);
    particleGroupData_.groupName = groupName;
    type_ = type;
    model_ = ModelManager::GetInstance()->FindModel(ModelManager::GetInstance()->CreatePrimitiveModel(type, texturePath));
    texManager_->LoadTexture(texturePath);
    modelData = model_->GetModelData();
    CreateVertexResource();
    CreateIndexResource();
    // マテリアルが複数ある場合は最初のものを使う
    particleGroupData_.materials.clear();
    if (texturePath.empty()) {
        if (!modelData.materials.empty()) {
            particleGroupData_.materials = ForParticleMaterials(modelData.materials);
        } else {
            particleGroupData_.materials.push_back(ParticleMaterial{});
        }
    } else {
        ParticleMaterial mat;
        mat.textureFilePath = texturePath;
        mat.textureIndex = texManager_->GetTextureIndexByFilePath(texturePath);
        particleGroupData_.materials.push_back(mat);
    }
    // すべてのマテリアルのテクスチャをロード
    for (auto &mat : particleGroupData_.materials) {
        texManager_->LoadTexture(mat.textureFilePath);
        mat.textureIndex = texManager_->GetTextureIndexByFilePath(mat.textureFilePath);
    }

    CreateMaterialResource();

    InitParticle();
    particleGroupData_.blendMode = blendMode;

    return particleGroupData_;
}

void ParticleCSGroup::InitParticle() {
    srvManager_->SetDescriptorHeap();

    dxCommon_->TransitionUAVBarrier(outputParticleResource_.Get());

    // InitParticle.CSの処理
    particleCommon_->ComputeInitDrawCommonSetting();
    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(3, freeListTrailIndexSrvHandle_.second);
    commandList->SetComputeRootConstantBufferView(4, settingsResource_->GetGPUVirtualAddress());
    int disPatchCount = (settingsData_->maxParticleCount + threadsPerGroup_ - 1) / threadsPerGroup_;
    commandList->Dispatch(disPatchCount, 1, 1);

    dxCommon_->TransitionSRVBarrier();
}

void ParticleCSGroup::UpdateParticleCSDisPatch() {
    // UpdateParticle.CSの処理
    particleCommon_->ComputeUpdateEmitterDrawCommonSetting();
    commandList->SetComputeRootDescriptorTable(0, outputParticleSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(1, freeListIndexSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(2, freeListSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(3, freeListTrailIndexSrvHandle_.second);
    commandList->SetComputeRootConstantBufferView(4, perFrameResource_->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(5, settingsResource_->GetGPUVirtualAddress());
    int disPatchCount = (settingsData_->maxParticleCount + threadsPerGroup_ - 1) / threadsPerGroup_;
    commandList->Dispatch(disPatchCount, 1, 1);
}

void ParticleCSGroup::Update(const ViewProjection &vp) {
    perFrameData_->time += Frame::DeltaTime();
    perFrameData_->deltaTime = Frame::DeltaTime();

    perViewData_->viewProjection = vp.matView_ * vp.matProjection_;
    perViewData_->billboardMatrix = vp.matView_;
    perViewData_->billboardMatrix.m[3][0] = 0.0f;
    perViewData_->billboardMatrix.m[3][1] = 0.0f;
    perViewData_->billboardMatrix.m[3][2] = 0.0f;
    perViewData_->billboardMatrix.m[3][3] = 1.0f;
    perViewData_->billboardMatrix = Inverse(perViewData_->billboardMatrix);

    CopyDebugDataToReadback();
}

void ParticleCSGroup::CreateOutputParticleResource() {

    outputParticleResource_ = dxCommon_->CreateBufferResource(sizeof(CSParticle) * settingsData_->maxParticleCount, true);

    // UAV用のインデックス（Compute Shader用）
    outputParticleSrvIndex_ = srvManager_->Allocate() + 1;
    outputParticleSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(outputParticleSrvIndex_);
    outputParticleSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(outputParticleSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(outputParticleSrvIndex_, outputParticleResource_.Get(), settingsData_->maxParticleCount, sizeof(CSParticle));

    // SRV用のインデックス（Vertex Shader用）
    outputParticleSrvForVSIndex_ = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(outputParticleSrvForVSIndex_, outputParticleResource_.Get(), settingsData_->maxParticleCount, sizeof(CSParticle));
}

void ParticleCSGroup::CreatePerViewResource() {
    perViewResource_ = dxCommon_->CreateBufferResource(sizeof(PerView));
    perViewResource_->Map(0, nullptr, reinterpret_cast<void **>(&perViewData_));
    perViewData_->viewProjection = MakeIdentity4x4();
    perViewData_->billboardMatrix = MakeIdentity4x4();
}

void ParticleCSGroup::CreateMaterialResource() {
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleMaterial));
    materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData_->uvTransform = MakeIdentity4x4();
}

void ParticleCSGroup::CreateIndexResource() {
    // 複数メッシュ対応: 全メッシュのインデックスを連結し、頂点オフセットを考慮
    std::vector<uint32_t> allIndices;
    uint32_t vertexOffset = 0;
    for (const auto &mesh : modelData.meshes) {
        for (auto idx : mesh.indices) {
            allIndices.push_back(idx + vertexOffset);
        }
        vertexOffset += static_cast<uint32_t>(mesh.vertices.size());
    }
    indexResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * allIndices.size());
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = UINT(sizeof(uint32_t) * allIndices.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));
    std::memcpy(indexData_, allIndices.data(), sizeof(uint32_t) * allIndices.size());
}

void ParticleCSGroup::CreateVertexResource() {
    // クアッド用の頂点データ
    std::vector<VertexData> allVertices;
    for (const auto &mesh : modelData.meshes) {
        allVertices.insert(allVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    }
    vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * allVertices.size());
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * allVertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
    std::memcpy(vertexData_, allVertices.data(), sizeof(VertexData) * allVertices.size());
}

void ParticleCSGroup::CreatePerFrameResource() {
    perFrameResource_ = dxCommon_->CreateBufferResource(sizeof(PerFrame));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void **>(&perFrameData_));
    perFrameData_->time = 0.0f;
    perFrameData_->deltaTime = 0.0f;
    perFrameData_->groupId = 0;
}

void ParticleCSGroup::CreateFreeListIndexResource() {
    freeListIndexResource_ = dxCommon_->CreateBufferResource(sizeof(int), true);

    // UAV用のインデックス（Compute Shader用）
    freeListIndexSrvIndex_ = srvManager_->Allocate() + 1;
    freeListIndexSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListIndexSrvIndex_);
    freeListIndexSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListIndexSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListIndexSrvIndex_, freeListIndexResource_.Get(), 1, sizeof(int));

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

    // リソース設定: int 1個分 (4バイト)
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(int32_t));

    // バッファ作成
    dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&freeListIndexReadbackBuffer_));
    freeListIndexReadbackBuffer_->SetName(L"FreeListIndex_Readback");
}

void ParticleCSGroup::CreateFreeListTrailIndexResource() {
    freeListTrailIndexResource_ = dxCommon_->CreateBufferResource(sizeof(int), true);

    freeListTrailIndexSrvIndex_ = srvManager_->Allocate() + 1;
    freeListTrailIndexSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListTrailIndexSrvIndex_);
    freeListTrailIndexSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListTrailIndexSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListTrailIndexSrvIndex_, freeListTrailIndexResource_.Get(), 1, sizeof(int));

    // ★ Readbackバッファも作成
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(int32_t));

    dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&freeListTrailIndexReadbackBuffer_));
    freeListTrailIndexReadbackBuffer_->SetName(L"FreeListTrailIndex_Readback");
}

void ParticleCSGroup::CopyDebugDataToReadback() {
    // === Head (freeListIndex) のコピー ===
    auto barrierHeadToCopy = CD3DX12_RESOURCE_BARRIER::Transition(
        freeListIndexResource_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(1, &barrierHeadToCopy);

    commandList->CopyBufferRegion(
        freeListIndexReadbackBuffer_.Get(), 0,
        freeListIndexResource_.Get(), 0,
        sizeof(int32_t));

    auto barrierHeadToUAV = CD3DX12_RESOURCE_BARRIER::Transition(
        freeListIndexResource_.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &barrierHeadToUAV);

    // === Tail (freeListTrailIndex) のコピー ===
    auto barrierTailToCopy = CD3DX12_RESOURCE_BARRIER::Transition(
        freeListTrailIndexResource_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(1, &barrierTailToCopy);

    commandList->CopyBufferRegion(
        freeListTrailIndexReadbackBuffer_.Get(), 0,
        freeListTrailIndexResource_.Get(), 0,
        sizeof(int32_t));

    auto barrierTailToUAV = CD3DX12_RESOURCE_BARRIER::Transition(
        freeListTrailIndexResource_.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &barrierTailToUAV);
}

void ParticleCSGroup::CreateFreeListResource() {
    freeListResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * settingsData_->maxParticleCount, true);

    // UAV用のインデックス（Compute Shader用）
    freeListSrvIndex_ = srvManager_->Allocate() + 1;
    freeListSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListSrvIndex_);
    freeListSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(freeListSrvIndex_, freeListResource_.Get(), settingsData_->maxParticleCount, sizeof(uint32_t));
}

void ParticleCSGroup::CreateSettingsResource() {
    settingsResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleCSSettings));
    settingsResource_->Map(0, nullptr, reinterpret_cast<void **>(&settingsData_));

    // デフォルト設定
    settingsData_->lifeTimeMin = 1.0f;
    settingsData_->lifeTimeMax = 3.0f;
    settingsData_->scaleMin = 0.5f;
    settingsData_->scaleMax = 1.5f;
    settingsData_->velocityMin = {-0.25f, -0.25f, -0.25f};
    settingsData_->velocityMax = {0.25f, 0.25f, 0.25f};
    settingsData_->startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    settingsData_->endColor = {1.0f, 1.0f, 1.0f, 0.0f};
    settingsData_->enableLifetimeScale = 0;
    settingsData_->enableRandomColor = 1;
    settingsData_->enableSinScale = 0;
    settingsData_->sinScaleFrequency = 5.0f;
    settingsData_->sinScaleAmplitude = 0.3f;
    settingsData_->maxParticleCount = 10000;
    settingsData_->emitCount = 0;
    settingsData_->enableGravity = 0;
    settingsData_->gravity = {0.0f, -9.8f, 0.0f};

    // トレイル設定のデフォルト値
    settingsData_->enableTrail = 0;
    settingsData_->trailSpawnDistance = 0.1f; // デフォルトを短く
    settingsData_->maxTrailPerParticle = 5;
    settingsData_->trailLifeTimeScale = 1.0f; // 親と同じ寿命割合に
    settingsData_->trailScaleMultiplier = {0.8f, 0.8f, 0.8f};
    settingsData_->trailColorMultiplier = {1.0f, 1.0f, 1.0f, 0.7f};
    settingsData_->trailVelocityScale = 0.3f;
    settingsData_->trailInheritVelocity = 1;
    settingsData_->trailMinLifeTime = 0.5f; // 最小寿命を長めに

    // ギャザー設定のデフォルト値を追加
    settingsData_->enableGather = 0;
    settingsData_->gatherStartRatio = 0.5f;
    settingsData_->gatherStrength = 2.0f;
    settingsData_->gatherTarget = {0.0f, 0.0f, 0.0f};

    settingsData_->enableAcceleration = 0;
    settingsData_->acceleration = {0.0f, 0.0f, 0.0f};
    settingsData_->enableVelocityDamping = 0;
    settingsData_->velocityDampingFactor = 0.98f;
    settingsData_->enableLifetimeVelocityDamping = 0;
    settingsData_->lifetimeVelocityDampingStart = 0.5f;
    settingsData_->enableRadialVelocity = 0;
    settingsData_->radialVelocityStrength = 1.0f;
    settingsData_->radialVelocityRandomness = 0.2f;
    settingsData_->radialVelocityCenter = {0.0f, 0.0f, 0.0f};
}

void ParticleCSGroup::CreateAliveCountResource() {
    // GPU側のカウント用バッファ (UAV)
    aliveCountResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t), true);

    aliveCountSrvIndex_ = srvManager_->Allocate() + 1;
    aliveCountSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(aliveCountSrvIndex_);
    aliveCountSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(aliveCountSrvIndex_);
    srvManager_->CreateUAVStructuredBuffer(aliveCountSrvIndex_, aliveCountResource_.Get(), 1, sizeof(uint32_t));

    // CPU読み取り用のReadbackバッファ
    D3D12_HEAP_PROPERTIES readbackHeapProps{};
    readbackHeapProps.Type = D3D12_HEAP_TYPE_READBACK;

    D3D12_RESOURCE_DESC readbackDesc{};
    readbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    readbackDesc.Width = sizeof(uint32_t);
    readbackDesc.Height = 1;
    readbackDesc.DepthOrArraySize = 1;
    readbackDesc.MipLevels = 1;
    readbackDesc.SampleDesc.Count = 1;
    readbackDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    dxCommon_->GetDevice()->CreateCommittedResource(
        &readbackHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&aliveCountReadbackResource_));
}

void ParticleCSGroup::CountAliveParticles() {
    // CountParticle.CSを実行
    particleCommon_->ComputeCountDrawCommonSetting();

    commandList->SetComputeRootConstantBufferView(0, settingsResource_->GetGPUVirtualAddress());
    commandList->SetComputeRootDescriptorTable(1, aliveCountSrvHandle_.second);
    commandList->SetComputeRootDescriptorTable(2, outputParticleSrvHandle_.second);

    int dispatchCount = (settingsData_->maxParticleCount + threadsPerGroup_ - 1) / threadsPerGroup_;
    commandList->Dispatch(dispatchCount, 1, 1);

    // UAVバリア（UAV書き込み完了を保証）
    dxCommon_->TransitionUAVBarrier(aliveCountResource_.Get());

    // CopyResource前にリソース状態を遷移（自作関数を使う）
    dxCommon_->BarrierTransition(
        aliveCountResource_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE);

    // GPU→CPUへコピー
    commandList->CopyResource(aliveCountReadbackResource_.Get(), aliveCountResource_.Get());

    // 戻す（次のDispatch用に再びUAV状態へ）
    dxCommon_->BarrierTransition(
        aliveCountResource_.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

uint32_t ParticleCSGroup::GetAliveParticleCount() {
    // Readbackバッファから読み取り
    uint32_t *mappedData = nullptr;
    D3D12_RANGE readRange{0, sizeof(uint32_t)};

    HRESULT hr = aliveCountReadbackResource_->Map(0, &readRange, reinterpret_cast<void **>(&mappedData));
    if (SUCCEEDED(hr) && mappedData) {
        cachedAliveCount_ = *mappedData;
        aliveCountReadbackResource_->Unmap(0, nullptr);
    }

    return cachedAliveCount_;
}

void ParticleCSGroup::DrawImGui() {
#ifdef USE_IMGUI
    if (!settingsData_)
        return;

    // -------------------------------------------------------
    // ヘルパー：色付きセクションヘッダ（CollapsingHeader）
    // 開いた状態をデフォルトにしたい場合は ImGuiTreeNodeFlags_DefaultOpen を渡す
    // -------------------------------------------------------
    auto PushSectionColor = [](ImVec4 col) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(col.x * 0.7f, col.y * 0.7f, col.z * 0.7f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(col.x * 0.85f, col.y * 0.85f, col.z * 0.85f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, col);
    };
    auto PopSectionColor = []() { ImGui::PopStyleColor(3); };

    // 行ラベル幅を統一
    ImGui::PushItemWidth(-120.0f);

    // =======================================================
    // 1. 出現・寿命・サイズ（赤系）
    // =======================================================
    PushSectionColor(ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    bool openBasic = ImGui::CollapsingHeader("  出現 / 寿命 / サイズ");
    PopSectionColor();
    if (openBasic) {
        ImGui::Indent();

        // 出現数
        {
            int emitCount = static_cast<int>(settingsData_->emitCount);
            int dynMax = CalculateOptimalEmitCount();
            int absMax = static_cast<int>(settingsData_->maxParticleCount);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.15f, 0.15f, 0.5f));
            if (ImGui::DragInt("出現数", &emitCount, 1, 0, absMax))
                settingsData_->emitCount = static_cast<uint32_t>(std::clamp(emitCount, 0, absMax));
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("推奨上限: %d  /  絶対上限: %d", dynMax, absMax);
            if (emitCount > dynMax) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
                ImGui::TextUnformatted(" 推奨超過");
                ImGui::PopStyleColor();
            }
        }

        // 寿命（横並び）
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.15f, 0.15f, 0.5f));
            float hw = (ImGui::GetContentRegionAvail().x - 130.0f) * 0.5f - 4.0f;
            ImGui::SetNextItemWidth(hw);
            ImGui::DragFloat("##lifeMin", &settingsData_->lifeTimeMin, 0.1f, 0.1f, settingsData_->lifeTimeMax, "Min %.1fs");
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(hw);
            if (ImGui::DragFloat("##lifeMax", &settingsData_->lifeTimeMax, 0.1f, settingsData_->lifeTimeMin, 30.0f, "Max %.1fs"))
                settingsData_->lifeTimeMax = std::max(settingsData_->lifeTimeMax, settingsData_->lifeTimeMin);
            ImGui::SameLine();
            ImGui::TextUnformatted("寿命(s)");
            ImGui::PopStyleColor();
        }

        // サイズ（横並び）
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.3f, 0.1f, 0.5f));
            float hw = (ImGui::GetContentRegionAvail().x - 130.0f) * 0.5f - 4.0f;
            ImGui::SetNextItemWidth(hw);
            ImGui::DragFloat("##scMin", &settingsData_->scaleMin, 0.01f, 0.0f, settingsData_->scaleMax, "Min %.2f");
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(hw);
            if (ImGui::DragFloat("##scMax", &settingsData_->scaleMax, 0.01f, settingsData_->scaleMin, 100.0f, "Max %.2f"))
                settingsData_->scaleMax = std::max(settingsData_->scaleMax, settingsData_->scaleMin);
            ImGui::SameLine();
            ImGui::TextUnformatted("サイズ");
            ImGui::PopStyleColor();
        }

        ImGui::Unindent();
    }

    // =======================================================
    // 2. 速度・色彩・ブレンド（青系）
    // =======================================================
    PushSectionColor(ImVec4(0.25f, 0.45f, 0.8f, 1.0f));
    bool openAppearance = ImGui::CollapsingHeader("  速度 / 色彩 / ブレンド");
    PopSectionColor();
    if (openAppearance) {
        ImGui::Indent();

        // 初期速度
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.2f, 0.4f, 0.5f));
        ImGui::DragFloat3("速度 Min", &settingsData_->velocityMin.x, 0.01f);
        ImGui::DragFloat3("速度 Max", &settingsData_->velocityMax.x, 0.01f);
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // 色彩
        {
            bool rnd = settingsData_->enableRandomColor != 0;
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));
            if (ImGui::Checkbox("ランダムカラー", &rnd))
                settingsData_->enableRandomColor = rnd ? 1 : 0;
            ImGui::PopStyleColor();
            if (!rnd) {
                ImGui::ColorEdit4("開始色", &settingsData_->startColor.x);
                ImGui::ColorEdit4("終了色", &settingsData_->endColor.x);
            }
        }

        ImGui::Spacing();

        // ブレンドモード
        {
            const char *blendNames[] = {"なし", "通常", "加算", "減算", "乗算", "スクリーン"};
            int bm = static_cast<int>(particleGroupData_.blendMode);
            if (ImGui::Combo("ブレンドモード", &bm, blendNames, IM_ARRAYSIZE(blendNames)))
                particleGroupData_.blendMode = static_cast<BlendMode>(bm);
        }

        ImGui::Unindent();
    }

    // =======================================================
    // 3. 動作設定（黄緑系）— 基本動作フラグ
    // =======================================================
    PushSectionColor(ImVec4(0.5f, 0.75f, 0.2f, 1.0f));
    bool openMotion = ImGui::CollapsingHeader("  動作設定");
    PopSectionColor();
    if (openMotion) {
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.6f, 0.9f, 0.4f, 1.0f));

        // 寿命で縮小
        {
            bool v = settingsData_->enableLifetimeScale != 0;
            if (ImGui::Checkbox("寿命で縮小", &v))
                settingsData_->enableLifetimeScale = v ? 1 : 0;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("時間経過と共にスケールが 0 に近づきます");
        }

        // Sin波拡縮
        {
            bool v = settingsData_->enableSinScale != 0;
            if (ImGui::Checkbox("Sin波で拡縮", &v))
                settingsData_->enableSinScale = v ? 1 : 0;
            if (v) {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.35f, 0.15f, 0.5f));
                ImGui::DragFloat("周波数##sf", &settingsData_->sinScaleFrequency, 0.1f);
                ImGui::DragFloat("振幅##sa", &settingsData_->sinScaleAmplitude, 0.01f);
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }
        }

        // 重力
        {
            bool v = settingsData_->enableGravity != 0;
            if (ImGui::Checkbox("重力", &v))
                settingsData_->enableGravity = v ? 1 : 0;
            if (v) {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.35f, 0.15f, 0.5f));
                ImGui::DragFloat3("重力ベクトル", &settingsData_->gravity.x, 0.1f);
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }
        }

        // 加速度
        {
            bool v = settingsData_->enableAcceleration != 0;
            if (ImGui::Checkbox("加速度", &v))
                settingsData_->enableAcceleration = v ? 1 : 0;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("重力とは別に毎フレーム速度に加算されます");
            if (v) {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.35f, 0.15f, 0.5f));
                ImGui::DragFloat3("加速度ベクトル", &settingsData_->acceleration.x, 0.1f);
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }
        }

        // 速度減衰
        {
            bool v = settingsData_->enableVelocityDamping != 0;
            if (ImGui::Checkbox("速度減衰", &v))
                settingsData_->enableVelocityDamping = v ? 1 : 0;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("空気抵抗のように徐々に減速します");
            if (v) {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.35f, 0.15f, 0.5f));
                ImGui::DragFloat("減衰係数##vd", &settingsData_->velocityDampingFactor, 0.001f, 0.8f, 0.999f, "%.3f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("推奨: 0.95-0.99");
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }
        }

        // 寿命速度減衰
        {
            bool v = settingsData_->enableLifetimeVelocityDamping != 0;
            if (ImGui::Checkbox("寿命による速度減衰", &v))
                settingsData_->enableLifetimeVelocityDamping = v ? 1 : 0;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("寿命末期に速度が 0 に近づきます");
            if (v) {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.35f, 0.15f, 0.5f));
                ImGui::DragFloat("開始タイミング##ld", &settingsData_->lifetimeVelocityDampingStart, 0.01f, 0.0f, 1.0f, "%.2f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("0.0=最初から / 1.0=最後のみ\n推奨: 0.5-0.8");
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }
        }

        ImGui::PopStyleColor(); // CheckMark
        ImGui::Unindent();
    }

    // =======================================================
    // 4. 放射状速度（オレンジ系）
    // =======================================================
    PushSectionColor(ImVec4(0.85f, 0.5f, 0.1f, 1.0f));
    bool openRadial = ImGui::CollapsingHeader("  放射状速度");
    PopSectionColor();
    if (openRadial) {
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));

        bool v = settingsData_->enableRadialVelocity != 0;
        if (ImGui::Checkbox("放射状速度を有効化", &v))
            settingsData_->enableRadialVelocity = v ? 1 : 0;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("中心点から放射状に飛び散る速度\n花火・爆発の演出に最適");

        if (v) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.25f, 0.05f, 0.5f));
            ImGui::DragFloat("放射強度##rs", &settingsData_->radialVelocityStrength, 0.1f, 0.1f, 20.0f);
            ImGui::DragFloat("ランダム性##rr", &settingsData_->radialVelocityRandomness, 0.01f, 0.0f, 1.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("0=完全放射状 / 1=完全ランダム\n推奨: 0.1-0.3");
            ImGui::DragFloat3("放射中心##rc", &settingsData_->radialVelocityCenter.x, 0.1f);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::TextDisabled("プリセット:");
            ImGui::SameLine();
            if (ImGui::SmallButton("花火")) {
                settingsData_->enableRadialVelocity = 1;
                settingsData_->radialVelocityStrength = 5.0f;
                settingsData_->radialVelocityRandomness = 0.2f;
                settingsData_->enableGravity = 1;
                settingsData_->gravity = {0, -9.8f, 0};
                settingsData_->enableVelocityDamping = 1;
                settingsData_->velocityDampingFactor = 0.95f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("爆発")) {
                settingsData_->enableRadialVelocity = 1;
                settingsData_->radialVelocityStrength = 8.0f;
                settingsData_->radialVelocityRandomness = 0.3f;
                settingsData_->enableGravity = 1;
                settingsData_->gravity = {0, -9.8f, 0};
                settingsData_->enableLifetimeVelocityDamping = 1;
                settingsData_->lifetimeVelocityDampingStart = 0.7f;
            }
        }

        ImGui::PopStyleColor();
        ImGui::Unindent();
    }

    // =======================================================
    // 5. ギャザー（紫系）
    // =======================================================
    PushSectionColor(ImVec4(0.6f, 0.2f, 0.8f, 1.0f));
    bool openGather = ImGui::CollapsingHeader("  ギャザー（集合）");
    PopSectionColor();
    if (openGather) {
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.5f, 1.0f, 1.0f));

        bool v = settingsData_->enableGather != 0;
        if (ImGui::Checkbox("ギャザーを有効化", &v))
            settingsData_->enableGather = v ? 1 : 0;

        if (v) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.1f, 0.4f, 0.5f));
            ImGui::DragFloat("開始タイミング##gs", &settingsData_->gatherStartRatio, 0.01f, 0.0f, 1.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("寿命の何%%から引き寄せを開始するか");
            ImGui::DragFloat("ギャザー強度##gstr", &settingsData_->gatherStrength, 0.1f, 0.0f, 20.0f);
            ImGui::DragFloat3("目標座標##gt", &settingsData_->gatherTargetOffset.x, 0.1f);
            ImGui::PopStyleColor();
            bool gft = settingsData_->enableGatherForTrail != 0;
            if (ImGui::Checkbox("トレイルにも適用##gft", &gft))
                settingsData_->enableGatherForTrail = gft ? 1 : 0;
        }

        ImGui::PopStyleColor();
        ImGui::Unindent();
    }

    // =======================================================
    // 6. Vortex（水色系）
    // =======================================================
    PushSectionColor(ImVec4(0.1f, 0.65f, 0.75f, 1.0f));
    bool openVortex = ImGui::CollapsingHeader("  渦巻き（Vortex）");
    PopSectionColor();
    if (openVortex) {
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.3f, 0.9f, 1.0f, 1.0f));

        bool v = settingsData_->enableVortex != 0;
        if (ImGui::Checkbox("渦巻きを有効化", &v))
            settingsData_->enableVortex = v ? 1 : 0;

        if (v) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.3f, 0.4f, 0.5f));
            ImGui::DragFloat("回転強度##vstr", &settingsData_->vortexStrength, 0.1f);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("+ で正回転 / - で逆回転");
            ImGui::DragFloat3("目標座標##vt", &settingsData_->vortexTargetOffset.x, 0.1f);
            ImGui::PopStyleColor();

            // 軸プリセット
            ImGui::Text("回転軸:");
            ImGui::SameLine();
            if (ImGui::SmallButton("X##vx"))
                settingsData_->vortexAxis = {1, 0, 0};
            ImGui::SameLine();
            if (ImGui::SmallButton("Y##vy"))
                settingsData_->vortexAxis = {0, 1, 0};
            ImGui::SameLine();
            if (ImGui::SmallButton("Z##vz"))
                settingsData_->vortexAxis = {0, 0, 1};
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.3f, 0.4f, 0.5f));
            ImGui::DragFloat3("軸ベクトル##vax", &settingsData_->vortexAxis.x, 0.05f, -1.0f, 1.0f);
            ImGui::PopStyleColor();

            bool vft = settingsData_->enableVortexForTrail != 0;
            if (ImGui::Checkbox("トレイルにも適用##vft", &vft))
                settingsData_->enableVortexForTrail = vft ? 1 : 0;
        }

        ImGui::PopStyleColor();
        ImGui::Unindent();
    }

    // =======================================================
    // 7. カールノイズ（シアン系）
    // =======================================================
    PushSectionColor(ImVec4(0.0f, 0.8f, 0.7f, 1.0f));
    bool openCurl = ImGui::CollapsingHeader("  カールノイズ");
    PopSectionColor();
    if (openCurl) {
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 1.0f, 0.9f, 1.0f));

        bool v = settingsData_->enableCurlNoise != 0;
        if (ImGui::Checkbox("カールノイズを有効化", &v))
            settingsData_->enableCurlNoise = v ? 1 : 0;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("発散ゼロの速度場で流体的な動きを実現\nvelocity を完全置き換えするため\n他の速度系エフェクトと排他的に使うと綺麗です");

        if (v) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.35f, 0.3f, 0.5f));

            ImGui::DragFloat("スケール##cns", &settingsData_->curlNoiseScale, 0.01f, 0.01f, 5.0f, "%.3f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("小 → 大きくゆったりした渦\n大 → 細かい乱流");
            ImGui::DragFloat("強度##cnstr", &settingsData_->curlNoiseStrength, 0.1f, 0.0f, 20.0f, "%.2f");
            ImGui::DragFloat("時間変化##cntm", &settingsData_->curlNoiseTimeScale, 0.01f, 0.0f, 2.0f, "%.3f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("0.0 = 固定フロー / 大 = 激しく変化");

            ImGui::PopStyleColor();

            int oct = static_cast<int>(settingsData_->curlNoiseOctaves);
            if (ImGui::SliderInt("オクターブ##cno", &oct, 1, 4))
                settingsData_->curlNoiseOctaves = static_cast<uint32_t>(oct);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("1=軽量なめらか / 4=複雑（負荷増）");

            // 引き戻し
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.9f, 0.85f, 1.0f));
            ImGui::TextUnformatted("引き戻し設定");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.35f, 0.3f, 0.5f));
            ImGui::DragFloat("引き戻し強度##cna", &settingsData_->curlNoiseAttractStrength, 0.01f, 0.0f, 5.0f, "%.3f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("0 = 無効\n大きいほどエミッター付近に密集して流れる");
            ImGui::DragFloat3("引き戻し中心##cnac", &settingsData_->curlNoiseAttractCenter.x, 0.1f);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("通常はエミッターと同じ座標に設定\nコードから毎フレーム SetCurlNoiseAttractCenter() で更新推奨");
            ImGui::PopStyleColor();

            // プリセット
            ImGui::Spacing();
            ImGui::TextDisabled("プリセット:");
            ImGui::SameLine();
            if (ImGui::SmallButton("煙・霧")) {
                settingsData_->curlNoiseScale = 0.4f;
                settingsData_->curlNoiseStrength = 1.5f;
                settingsData_->curlNoiseTimeScale = 0.15f;
                settingsData_->curlNoiseOctaves = 2;
                settingsData_->curlNoiseAttractStrength = 0.3f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("炎・乱流")) {
                settingsData_->curlNoiseScale = 1.2f;
                settingsData_->curlNoiseStrength = 4.0f;
                settingsData_->curlNoiseTimeScale = 0.6f;
                settingsData_->curlNoiseOctaves = 3;
                settingsData_->curlNoiseAttractStrength = 0.8f;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("水流")) {
                settingsData_->curlNoiseScale = 0.7f;
                settingsData_->curlNoiseStrength = 2.5f;
                settingsData_->curlNoiseTimeScale = 0.25f;
                settingsData_->curlNoiseOctaves = 2;
                settingsData_->curlNoiseAttractStrength = 0.5f;
            }
        }

        ImGui::PopStyleColor();
        ImGui::Unindent();
    }

    // =======================================================
    // 8. トレイル（緑系）
    // =======================================================
    PushSectionColor(ImVec4(0.2f, 0.7f, 0.35f, 1.0f));
    bool openTrail = ImGui::CollapsingHeader("  トレイル");
    PopSectionColor();
    if (openTrail) {
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.4f, 1.0f, 0.6f, 1.0f));

        bool v = settingsData_->enableTrail != 0;
        if (ImGui::Checkbox("トレイルを有効化", &v))
            settingsData_->enableTrail = v ? 1 : 0;

        if (v) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.3f, 0.15f, 0.5f));

            // 基本
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.9f, 0.7f, 1.0f));
            ImGui::TextUnformatted("基本設定");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::DragFloat("生成間隔距離##tsd", &settingsData_->trailSpawnDistance, 0.01f, 0.01f, 1.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("この距離ごとにトレイルを生成\n小さいほど滑らか（推奨: 0.05-0.15）");
            int maxT = static_cast<int>(settingsData_->maxTrailPerParticle);
            if (ImGui::DragInt("最大数/親##tmax", &maxT, 1, 1, 20))
                settingsData_->maxTrailPerParticle = static_cast<uint32_t>(maxT);

            // 特性
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.9f, 0.7f, 1.0f));
            ImGui::TextUnformatted("トレイル特性");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::DragFloat("寿命倍率##tlt", &settingsData_->trailLifeTimeScale, 0.05f, 0.3f, 3.0f);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("親の残り寿命に対する倍率（推奨: 0.8-1.5）");
            ImGui::DragFloat("最小寿命(s)##tmn", &settingsData_->trailMinLifeTime, 0.05f, 0.1f, 2.0f);
            ImGui::DragFloat3("スケール倍率##tsc", &settingsData_->trailScaleMultiplier.x, 0.01f, 0.1f, 2.0f);
            ImGui::ColorEdit4("色倍率##tco", &settingsData_->trailColorMultiplier.x);

            // 速度
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.9f, 0.7f, 1.0f));
            ImGui::TextUnformatted("速度設定");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::PopStyleColor(); // FrameBg

            bool inh = settingsData_->trailInheritVelocity != 0;
            if (ImGui::Checkbox("親の速度を継承##tiv", &inh))
                settingsData_->trailInheritVelocity = inh ? 1 : 0;
            if (inh) {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.3f, 0.15f, 0.5f));
                ImGui::DragFloat("速度倍率##tvs", &settingsData_->trailVelocityScale, 0.01f, 0.0f, 2.0f);
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }
        }

        ImGui::PopStyleColor(); // CheckMark
        ImGui::Unindent();
    }

  // =======================================================
    // 9. Debug Info（常時表示）
    // =======================================================

    // 循環バッファ定数
    static const int kHistorySize = 256;

    ImGui::Spacing();
    ImGui::Separator();
    {
        // --- パーティクル数を読み取り ---
        int32_t headVal = 0, tailVal = 0;
        int32_t *p = nullptr;
        D3D12_RANGE r = {0, sizeof(int32_t)};
        if (SUCCEEDED(freeListIndexReadbackBuffer_->Map(0, &r, reinterpret_cast<void **>(&p)))) {
            headVal = *p;
            freeListIndexReadbackBuffer_->Unmap(0, nullptr);
        }
        if (SUCCEEDED(freeListTrailIndexReadbackBuffer_->Map(0, &r, reinterpret_cast<void **>(&p)))) {
            tailVal = *p;
            freeListTrailIndexReadbackBuffer_->Unmap(0, nullptr);
        }
        int32_t used = settingsData_->maxParticleCount - (tailVal - headVal);
        float rate = (float)used / (float)settingsData_->maxParticleCount;

        // --- 循環バッファへ記録 ---
        static float particleHistory[kHistorySize] = {};
        static float particleRateHistory[kHistorySize] = {};
        static int histOffset = 0;
        particleHistory[histOffset] = (float)used;
        particleRateHistory[histOffset] = rate * 100.0f;
        histOffset = (histOffset + 1) % kHistorySize;

        // --- ProgressBar（従来の使用率バー） ---
        char overlay[64];
        sprintf_s(overlay, "%d / %d  (%.1f%%)", used, (int)settingsData_->maxParticleCount, rate * 100.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 1));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                              rate >= 0.9f ? ImVec4(1.0f, 0.3f, 0.3f, 1) : rate >= 0.7f ? ImVec4(1.0f, 0.9f, 0.2f, 1)
                                                                                        : ImVec4(0.4f, 1.0f, 0.4f, 1));
        ImGui::ProgressBar(rate, ImVec2(-1, 0), overlay);
        ImGui::PopStyleColor(3);

        // --- パーティクル数グラフ ---
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.85f, 1.0f, 1.0f));
        ImGui::TextUnformatted("パーティクル数 (履歴)");
        ImGui::PopStyleColor();

        ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0.08f, 0.08f, 0.12f, 1.0f));
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.05f, 0.05f, 0.09f, 1.0f));
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));

        if (ImPlot::BeginPlot("##ParticleCount", ImVec2(-1, 80),
                              ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoInputs |
                                  ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText)) {
            ImPlot::SetupAxes(nullptr, nullptr,
                              ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoGridLines,
                              ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, kHistorySize, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, (double)settingsData_->maxParticleCount, ImPlotCond_Always);
            ImPlot::PlotLine("##pc", particleHistory, kHistorySize, 1.0, 0.0,
                             ImPlotLineFlags_None, histOffset);
            ImPlot::EndPlot();
        }

        ImPlot::PopStyleColor(3);
    }

    ImGui::PopItemWidth();

    // デバッグ球描画
    if (settingsData_->enableGather)
        DrawLine3D::GetInstance()->DrawSphere(settingsData_->gatherTarget, {1.0f, 0.0f, 1.0f, 1.0f}, 0.1f, 8);
    if (settingsData_->enableVortex)
        DrawLine3D::GetInstance()->DrawSphere(settingsData_->vortexTarget, {0.5f, 1.0f, 0.0f, 1.0f}, 0.1f, 8);

#endif // USE_IMGUI
}
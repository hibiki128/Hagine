#include "ParticleCSGroup.h"
#include <Frame.h>
#include <Graphics/Model/ModelManager.h>

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
    commandList->SetComputeRootConstantBufferView(3, settingsResource_->GetGPUVirtualAddress());
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
    commandList->SetComputeRootConstantBufferView(3, perFrameResource_->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(4, settingsResource_->GetGPUVirtualAddress());
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

    // パーティクル基本設定セクション
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.3f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.6f, 0.4f, 1.0f));

    if (ImGui::CollapsingHeader("パーティクル基本設定")) {
        ImGui::PopStyleColor(3);

        // 出現数設定
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.8f, 1.0f));
        if (ImGui::TreeNode("出現数")) {
            ImGui::PopStyleColor();

            int emitCount = static_cast<int>(settingsData_->emitCount);
            int dynamicMaxCount = CalculateOptimalEmitCount();
            int maxCount = std::min(static_cast<int>(settingsData_->maxParticleCount), dynamicMaxCount);

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.2f, 0.4f));
            if (ImGui::DragInt("出現数（emitCount）", &emitCount, 1, 0, maxCount)) {
                emitCount = std::clamp(emitCount, 0, maxCount);
                settingsData_->emitCount = static_cast<uint32_t>(emitCount);
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.6f, 1.0f));
            ImGui::Text("推奨上限: %d (最大寿命%.2fs / 発生間隔%.2fs)",
                        dynamicMaxCount, settingsData_->lifeTimeMax, frequency_);
            ImGui::Text("絶対上限: %d", static_cast<int>(settingsData_->maxParticleCount));
            ImGui::PopStyleColor();

            ImGui::TreePop();
        } else {
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        // 寿命設定
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.8f, 1.0f));
        if (ImGui::TreeNode("寿命")) {
            ImGui::PopStyleColor();

            ImGui::Text("寿命設定:");
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.2f, 0.4f));
            ImGui::DragFloat("最小寿命", &settingsData_->lifeTimeMin, 0.1f, 0.1f, 10.0f);
            ImGui::DragFloat("最大寿命", &settingsData_->lifeTimeMax, 0.1f, 0.1f, 10.0f);
            ImGui::PopStyleColor();

            if (settingsData_->lifeTimeMin > settingsData_->lifeTimeMax) {
                settingsData_->lifeTimeMax = settingsData_->lifeTimeMin;
            }

            ImGui::TreePop();
        } else {
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        // サイズ設定
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
        if (ImGui::TreeNode("大きさ")) {
            ImGui::PopStyleColor();

            ImGui::Text("大きさ:");
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.4f, 0.2f, 0.4f));
            ImGui::DragFloat("最小サイズ", &settingsData_->scaleMin, 0.01f);
            ImGui::DragFloat("最大サイズ", &settingsData_->scaleMax, 0.01f);
            ImGui::PopStyleColor();

            if (settingsData_->scaleMin > settingsData_->scaleMax) {
                settingsData_->scaleMax = settingsData_->scaleMin;
            }

            ImGui::TreePop();
        } else {
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        // 速度設定
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.0f, 1.0f));
        if (ImGui::TreeNode("速度")) {
            ImGui::PopStyleColor();

            ImGui::Text("速度:");
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.4f, 0.4f));
            ImGui::DragFloat3("最小速度", &settingsData_->velocityMin.x, 0.01f);
            ImGui::DragFloat3("最大速度", &settingsData_->velocityMax.x, 0.01f);
            ImGui::PopStyleColor();

            ImGui::TreePop();
        } else {
            ImGui::PopStyleColor();
        }

        ImGui::Separator();

        // 色彩設定
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.8f, 1.0f));
        if (ImGui::TreeNode("色彩")) {
            ImGui::PopStyleColor();

            bool enableRandomColor = settingsData_->enableRandomColor != 0;
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Checkbox("ランダムカラー", &enableRandomColor)) {
                settingsData_->enableRandomColor = enableRandomColor ? 1 : 0;
            }
            ImGui::PopStyleColor();

            if (!enableRandomColor) {
                ImGui::ColorEdit4("開始時の色", &settingsData_->startColor.x);
                ImGui::ColorEdit4("終了時の色", &settingsData_->endColor.x);
            }

            ImGui::TreePop();
        } else {
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 動作設定をTreeNodeとして追加
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.7f, 1.0f));
        if (ImGui::TreeNode("動作設定")) {
            ImGui::PopStyleColor();

            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.7f, 1.0f));
            ImGui::Text("特殊効果:");
            ImGui::PopStyleColor();

            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.6f, 0.8f, 0.6f, 1.0f));
            bool enableLifetimeScale = settingsData_->enableLifetimeScale != 0;
            if (ImGui::Checkbox("寿命で小さくなる", &enableLifetimeScale)) {
                settingsData_->enableLifetimeScale = enableLifetimeScale ? 1 : 0;
            }

            bool enableSinScale = settingsData_->enableSinScale != 0;
            if (ImGui::Checkbox("Sin波で拡縮", &enableSinScale)) {
                settingsData_->enableSinScale = enableSinScale ? 1 : 0;
            }

            bool enableGravity = settingsData_->enableGravity != 0;
            if (ImGui::Checkbox("重力を有効化", &enableGravity)) {
                settingsData_->enableGravity = enableGravity ? 1 : 0;
            }
            ImGui::PopStyleColor();

            if (enableGravity) {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.4f, 0.6f));
                ImGui::DragFloat3("重力ベクトル##Gravity", &settingsData_->gravity.x, 0.1f);
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }

            if (enableSinScale) {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.4f, 0.6f));
                ImGui::DragFloat("周波数##SinFreq", &settingsData_->sinScaleFrequency, 0.1f);
                ImGui::DragFloat("振幅##SinAmp", &settingsData_->sinScaleAmplitude, 0.01f);
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("パーティクルが時間経過と共に小さくなります");
            }

            ImGui::Spacing();
            ImGui::Separator();

            // ブレンドモード設定
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.7f, 1.0f));
            ImGui::Text("ブレンドモード:");
            ImGui::PopStyleColor();

            const char *blendModeNames[] = {
                "なし",
                "通常",
                "加算",
                "減算",
                "乗算",
                "スクリーン"};

            int currentBlendMode = static_cast<int>(particleGroupData_.blendMode);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.4f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.5f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.6f, 0.8f));
            if (ImGui::Combo("##BlendMode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
                particleGroupData_.blendMode = static_cast<BlendMode>(currentBlendMode);
            }
            ImGui::PopStyleColor(3);

            ImGui::TreePop();
        } else {
            ImGui::PopStyleColor();
        }

    } else {
        ImGui::PopStyleColor(3);
    }
#endif // USE_IMGUI
}

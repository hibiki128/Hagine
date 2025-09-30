#define NOMINMAX
#include "ParticleCSEmitter.h"
#include "ParticleCSGroupManager.h"
#include <Frame.h>
#include <Line/DrawLine3D.h>
#include <Particle/ParticleCommon.h>
#include <regex>

void ParticleCSEmitter::Initialize(const std::string &name) {
    particleCommon_ = ParticleCommon::GetInstance();
    dxCommon_ = ParticleCommon::GetInstance()->GetDxCommon();
    commandList = dxCommon_->GetCommandList().Get();
    srvManager_ = SrvManager::GetInstance();
    name_ = name;
    CreateEmitterMeshResource();
    LoadSetting();
}

void ParticleCSEmitter::Initialize(const std::string &name, const std::string &modelPath) {
    Initialize(name);
    modelPath_ = modelPath;
    LoadModel(modelPath);
    CreateModelTriangles();
}

void ParticleCSEmitter::Initialize(const std::string &name, PrimitiveType primitiveType) {
    Initialize(name);
    primitiveType_ = primitiveType;
    LoadPrimitiveModel(primitiveType);
    CreateModelTriangles();
}

void ParticleCSEmitter::Draw(const ViewProjection &vp) {
    DrawEmitter();

    for (auto &group : particleGroups_) {
        group->Update(vp);
        dxCommon_->TransitionUAVBarrier(group->GetOutputParticleResource().Get());
        EmitterDisPatch();
        group->UpdateParticleCSDisPatch();
        dxCommon_->TransitionSRVBarrier();
        particleCommon_->GPUDrawCommonSetting(group->GetParticleGroupData().blendMode);
        const auto &meshes = group->GetModelData().meshes;
        for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
            D3D12_INDEX_BUFFER_VIEW indexBufferView = group->GetIndexBufferView();
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView = group->GetVertexBufferView();
            commandList->IASetIndexBuffer(&indexBufferView);
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            commandList->SetGraphicsRootConstantBufferView(0, group->GetPerViewResource()->GetGPUVirtualAddress());
            srvManager_->SetGraphicsRootDescriptorTable(1, group->GetOutputParticleSrvForVSIndex());
            srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(group->GetParticleGroupData().materials[meshIndex].textureFilePath));
            commandList->SetGraphicsRootConstantBufferView(3, group->GetMaterialResource()->GetGPUVirtualAddress());
            commandList->DrawIndexedInstanced(UINT(meshes[meshIndex].indices.size()), group->GetSettingsData()->maxParticleCount, 0, 0, 0);
        }
    }
}

void ParticleCSEmitter::LoadModel(const std::string &modelPath) {
    ModelManager::GetInstance()->LoadModel(modelPath);
    model_ = ModelManager::GetInstance()->FindModel(modelPath);
    if (model_) {
        modelData_ = model_->GetModelData();
    }
}

void ParticleCSEmitter::LoadPrimitiveModel(PrimitiveType type) {
    std::string modelKey = ModelManager::GetInstance()->CreatePrimitiveModel(type, "");
    model_ = ModelManager::GetInstance()->FindModel(modelKey);
    if (model_) {
        modelData_ = model_->GetModelData();
    }
}

void ParticleCSEmitter::Update() {
    if (isAuto_) {
        EmitterUpdate();
    } else {
            emitterMeshData_->emit = 0;
    }
}

void ParticleCSEmitter::DrawEmitter() {
    if (!isVisible_)
        return;

    if (emitterMeshData_->triangleCount == 0) {
        // 球体エミッター（triangleCountが0の場合）
        Vector3 center = emitterMeshData_->translate;
        Vector3 scale = emitterMeshData_->scale;
        Vector4 color = {1.0f, 1.0f, 0.0f, 1.0f};

        float maxRadius = std::max(std::max(scale.x, scale.y), scale.z);
        DrawLine3D::GetInstance()->DrawSphere(center, color, maxRadius, 16);

    } else {
        // メッシュエミッター
        Vector4 color = {0.0f, 1.0f, 0.0f, 1.0f};
        Vector3 translate = emitterMeshData_->translate;
        Vector3 rotation = emitterMeshData_->rotation;
        Vector3 scale = emitterMeshData_->scale;

        Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
        Matrix4x4 rotateMatrixX = MakeRotateXMatrix(rotation.x);
        Matrix4x4 rotateMatrixY = MakeRotateYMatrix(rotation.y);
        Matrix4x4 rotateMatrixZ = MakeRotateZMatrix(rotation.z);
        Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

        Matrix4x4 transformMatrix = translateMatrix * rotateMatrixZ * rotateMatrixY * rotateMatrixX * scaleMatrix;
    }
}

void ParticleCSEmitter::AddParticleGroup(ParticleCSGroup *group) {
    if (!group)
        return;
    const std::string &name = group->GetGroupName();
    ParticleCSGroup *independentGroup = ParticleCSGroupManager::GetInstance()->GetIndependentParticleGroup(name);
    if (!independentGroup) {
        return;
    }
    independentGroup->SetSettingData(*group->GetSettingsData());
    independentGroup->SetBlendMode(group->GetParticleGroupData().blendMode);
    particleGroups_.push_back(independentGroup);
    particleGroupNames_.insert(name);
}

void ParticleCSEmitter::RemoveParticleGroup(const std::string &groupName) {
    auto it = std::remove_if(particleGroups_.begin(), particleGroups_.end(),
                             [&](ParticleCSGroup *group) {
                                 return group->GetGroupName() == groupName;
                             });
    if (it != particleGroups_.end()) {
        particleGroups_.erase(it, particleGroups_.end());
    }
    particleGroupNames_.erase(groupName);
}

void ParticleCSEmitter::EmitterUpdate() {
        emitterMeshData_->frequencyTime += Frame::DeltaTime();
        if (emitterMeshData_->frequency <= emitterMeshData_->frequencyTime) {
            emitterMeshData_->frequencyTime -= emitterMeshData_->frequency;
            emitterMeshData_->emit = 1;
        } else {
            emitterMeshData_->emit = 0;
        }
}
void ParticleCSEmitter::CreateEmitterMeshResource() {
    emitterMeshResource_ = dxCommon_->CreateBufferResource(sizeof(EmitterMesh));
    emitterMeshResource_->Map(0, nullptr, reinterpret_cast<void **>(&emitterMeshData_));
    emitterMeshData_->frequency = 0.5f;
    emitterMeshData_->frequencyTime = 0.0f;
    emitterMeshData_->translate = Vector3(0.0f, 0.0f, 0.0f);
    emitterMeshData_->rotation = Vector3(0.0f, 0.0f, 0.0f); // 回転を初期化
    emitterMeshData_->scale = Vector3(1.0f, 1.0f, 1.0f);    // スケールを初期化
    emitterMeshData_->triangleCount = 0;
    emitterMeshData_->emit = 0;
}

void ParticleCSEmitter::EmitterDisPatch() {
    particleCommon_->ComputeEmitterDrawCommonSetting();

    uint32_t groupIndex = 0;
    for (auto &group : particleGroups_) {
        group->GetPerFrameData()->groupId = groupIndex;

        commandList->SetComputeRootDescriptorTable(0, group->GetOutputParticleSrvHandle().second);
        commandList->SetComputeRootDescriptorTable(1, group->GetFreeListIndexSrvHandle().second);
        commandList->SetComputeRootDescriptorTable(2, group->GetFreeListSrvHandle().second);

        commandList->SetComputeRootConstantBufferView(3, emitterMeshResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(4, group->GetPerFrameResource()->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(5, group->GetSettingsResource()->GetGPUVirtualAddress());

    if (emitterMeshData_->triangleCount > 0 && surfacePointResource_) {
            commandList->SetComputeRootDescriptorTable(6, surfacePointSrvHandle_.second);
        }
        int dispatchCount = (group->GetSettingsData()->emitCount + threadGroupSize_ - 1) / threadGroupSize_;
        commandList->Dispatch(dispatchCount, 1, 1);

        groupIndex++;
    }
}

std::unique_ptr<ParticleCSEmitter> ParticleCSEmitter::Clone() const {
    auto newEmitter = std::make_unique<ParticleCSEmitter>();

    auto &nameCounter = GetNameCounter();
    std::string baseName = name_;
    std::regex suffixRegex("_(\\d+)$");
    baseName = std::regex_replace(baseName, suffixRegex, "");

    int &counter = nameCounter[baseName];
    ++counter;

    std::string newName = baseName + "_" + std::to_string(counter);

    newEmitter->Initialize(baseName);
    newEmitter->SetName(newName);
    newEmitter->LoadCloneSetting();
    newEmitter->SetActive(this->isActive_);
    newEmitter->isAuto_ = this->isAuto_;
    newEmitter->isVisible_ = this->isVisible_;

    *newEmitter->emitterMeshData_ = *this->emitterMeshData_;

    return newEmitter;
}

void ParticleCSEmitter::CreateSurfacePointSRV() {
    if (surfacePointResource_ && !surfacePoints_.empty()) {
        surfacePointSrvIndex_ = srvManager_->Allocate() + 1;
        surfacePointSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(surfacePointSrvIndex_);
        surfacePointSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(surfacePointSrvIndex_);
        srvManager_->CreateSRVforStructuredBuffer(surfacePointSrvIndex_, surfacePointResource_.Get(),
                                                  static_cast<uint32_t>(surfacePoints_.size()), sizeof(SurfacePoint));
    }
}

void ParticleCSEmitter::CreateModelTriangles() {
    if (modelData_.meshes.empty())
        return;

    surfacePoints_.clear();

    // 三角形から表面の点を生成
    const int pointsPerTriangle = 10; // 1つの三角形あたりの点数

    for (const auto &mesh : modelData_.meshes) {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            uint32_t i0 = mesh.indices[i];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];

            if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size())
                continue;

            Vector3 v0(mesh.vertices[i0].position.x, mesh.vertices[i0].position.y, mesh.vertices[i0].position.z);
            Vector3 v1(mesh.vertices[i1].position.x, mesh.vertices[i1].position.y, mesh.vertices[i1].position.z);
            Vector3 v2(mesh.vertices[i2].position.x, mesh.vertices[i2].position.y, mesh.vertices[i2].position.z);

            // 三角形の面積を計算
            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;
            Vector3 crossProd = edge1.Cross(edge2);
            float area = crossProd.Length() * 0.5f;

            // 面積に応じて点の数を調整
            int numPoints = std::max(1, static_cast<int>(area * pointsPerTriangle * 100.0f));

            // 三角形内にランダムに点を配置
            for (int p = 0; p < numPoints; ++p) {
                float r1 = static_cast<float>(rand()) / RAND_MAX;
                float r2 = static_cast<float>(rand()) / RAND_MAX;

                if (r1 + r2 > 1.0f) {
                    r1 = 1.0f - r1;
                    r2 = 1.0f - r2;
                }
                float r3 = 1.0f - r1 - r2;

                Vector3 point = v0 * r1 + v1 * r2 + v2 * r3;

                SurfacePoint sp;
                sp.position = point;
                sp.padding = 0.0f;
                surfacePoints_.push_back(sp);
            }
        }
    }

    if (!surfacePoints_.empty()) {
        // サーフェスポイントのリソース作成
        size_t bufferSize = sizeof(SurfacePoint) * surfacePoints_.size();
        surfacePointResource_ = dxCommon_->CreateBufferResource(bufferSize);
        surfacePointResource_->Map(0, nullptr, reinterpret_cast<void **>(&surfacePointData_));
        std::memcpy(surfacePointData_, surfacePoints_.data(), bufferSize);
        CreateSurfacePointSRV();

        emitterMeshData_->triangleCount = static_cast<uint32_t>(surfacePoints_.size());
    }
}

void ParticleCSEmitter::SaveSetting() {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleCS", name_);
    std::unique_ptr<DataHandler> groupData;

    data->Save("isAuto", isAuto_);
    data->Save("isVisible", isVisible_);

    data->Save("frequency", emitterMeshData_->frequency);
    data->Save("frequencyTime", emitterMeshData_->frequencyTime);
    data->Save<Vector3>("translate", emitterMeshData_->translate);
    data->Save<Vector3>("rotation", emitterMeshData_->rotation);
    data->Save<Vector3>("scale", emitterMeshData_->scale);
    data->Save("modelPath", modelPath_);
    data->Save("primitiveType", static_cast<int>(primitiveType_));

    data->Save("particleGroupCount", static_cast<int>(particleGroups_.size()));
    int index = 0;
    for (auto &group : particleGroups_) {
        data->Save("particleGroup_" + index, group->GetGroupName());
        groupData = std::make_unique<DataHandler>("ParticleCSGroup", group->GetGroupName());
        groupData->Save("minLifetime", group->GetSettingsData()->lifeTimeMin);
        groupData->Save("maxLifetime", group->GetSettingsData()->lifeTimeMax);
        groupData->Save("minScale", group->GetSettingsData()->scaleMin);
        groupData->Save("maxScale", group->GetSettingsData()->scaleMax);
        groupData->Save("minVelocity", group->GetSettingsData()->velocityMin);
        groupData->Save("maxVelocity", group->GetSettingsData()->velocityMax);
        groupData->Save("startColor", group->GetSettingsData()->startColor);
        groupData->Save("endColor", group->GetSettingsData()->endColor);
        groupData->Save("isLifetimeScale", group->GetSettingsData()->enableLifetimeScale);
        groupData->Save("isRandomColor", group->GetSettingsData()->enableRandomColor);
        groupData->Save("emitCount", group->GetSettingsData()->emitCount);
        groupData->Save("blendMode", group->GetParticleGroupData().blendMode);
        index++;
    }
}

void ParticleCSEmitter::LoadSetting() {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleCS", name_);
    std::unique_ptr<DataHandler> groupData;

    isAuto_ = data->Load("isAuto", false);
    isVisible_ = data->Load("isVisible", true);

    emitterMeshData_->frequency = data->Load("frequency", 0.1f);
    emitterMeshData_->frequencyTime = data->Load("frequencyTime", 0.0f);
    emitterMeshData_->translate = data->Load<Vector3>("translate", Vector3(0.0f, 0.0f, 0.0f));
    emitterMeshData_->rotation = data->Load<Vector3>("rotation", Vector3(0.0f, 0.0f, 0.0f));
    emitterMeshData_->scale = data->Load<Vector3>("scale", Vector3(1.0f, 1.0f, 1.0f));

    modelPath_ = data->Load("modelPath", std::string(""));
    primitiveType_ = static_cast<PrimitiveType>(data->Load("primitiveType", static_cast<int>(PrimitiveType::None)));

    if (!modelPath_.empty()) {
        LoadModel(modelPath_);
        CreateModelTriangles();
    } else if (primitiveType_ != PrimitiveType::None) {
        LoadPrimitiveModel(primitiveType_);
        CreateModelTriangles();
    }
    // Load particle groups (rest remains the same)
    groupNum_ = data->Load("particleGroupCount", 0);
    for (int i = 0; i < groupNum_; i++) {
        auto group = ParticleCSGroupManager::GetInstance()->GetIndependentParticleGroup(data->Load("particleGroup_" + i, std::string("")));
        groupData = std::make_unique<DataHandler>("ParticleCSGroup", group->GetGroupName());
        group->SetSettingData({groupData->Load("minLifetime", 1.0f),
                               groupData->Load("maxLifetime", 1.0f),
                               groupData->Load("minScale", 1.0f),
                               groupData->Load("maxScale", 1.0f),
                               groupData->Load<Vector3>("minVelocity", {0.0f, 0.0f, 0.0f}),
                               {},
                               groupData->Load<Vector3>("maxVelocity", {0.0f, 0.0f, 0.0f}),
                               {},
                               groupData->Load("startColor", Vector4(1.0f, 1.0f, 1.0f, 1.0f)),
                               groupData->Load("endColor", Vector4(1.0f, 1.0f, 1.0f, 0.0f)),
                               groupData->Load<uint32_t>("isLifetimeScale", 0),
                               groupData->Load<uint32_t>("isRandomColor", 0),
                               uint32_t(groupData->Load("emitCount", 10)),
                               group->GetMaxParticleCount()});
        group->SetBlendMode(static_cast<BlendMode>(groupData->Load<int>("blendMode", static_cast<int>(BlendMode::kAdd))));
        if (group) {
            AddParticleGroup(group);
        }
    }
}

void ParticleCSEmitter::LoadCloneSetting() {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleCS", name_);
    if (!data->Exists()) {
        return;
    }

    isAuto_ = data->Load("isAuto", false);
    isVisible_ = data->Load("isVisible", true);

    emitterMeshData_->frequency = data->Load("frequency", 0.1f);
    emitterMeshData_->frequencyTime = data->Load("frequencyTime", 0.0f);
    emitterMeshData_->translate = data->Load<Vector3>("translate", Vector3(0.0f, 0.0f, 0.0f));
    emitterMeshData_->rotation = data->Load<Vector3>("rotation", Vector3(0.0f, 0.0f, 0.0f));
    emitterMeshData_->scale = data->Load<Vector3>("scale", Vector3(1.0f, 1.0f, 1.0f));

    modelPath_ = data->Load("modelPath", std::string(""));
    primitiveType_ = static_cast<PrimitiveType>(data->Load("primitiveType", static_cast<int>(PrimitiveType::None)));

    if (!modelPath_.empty()) {
        LoadModel(modelPath_);
        CreateModelTriangles();
    } else if (primitiveType_ != PrimitiveType::None) {
        LoadPrimitiveModel(primitiveType_);
        CreateModelTriangles();
    }
}

void ParticleCSEmitter::DrawImGui() {
    if (ImGui::BeginTabBar("EmitterTabBar")) {
        if (ImGui::BeginTabItem(name_.c_str())) {
            ImGuiStyle &style = ImGui::GetStyle();
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.15f, 1.00f));

            // エミッターデータセクション
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.2f, 0.2f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.3f, 0.3f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.6f, 0.4f, 0.4f, 1.0f));

            if (ImGui::CollapsingHeader("エミッターデータ##EmitterData")) {
                ImGui::PopStyleColor(3);

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.6f, 1.0f));
                ImGui::Text("エミッター設定:");
                ImGui::PopStyleColor();

                ImGui::Separator();

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.2f, 0.5f));

                    ImGui::DragFloat("発生間隔##Freq", &emitterMeshData_->frequency, 0.001f, 0.001f, 10.0f);
                    ImGui::DragFloat3("エミッタの座標##Translate", &emitterMeshData_->translate.x, 0.1f);

                    Vector3 rotationDegrees = {
                        emitterMeshData_->rotation.x * 180.0f / 3.14159f,
                        emitterMeshData_->rotation.y * 180.0f / 3.14159f,
                        emitterMeshData_->rotation.z * 180.0f / 3.14159f};
                    if (ImGui::DragFloat3("エミッタの回転##Rotation", &rotationDegrees.x, 1.0f, -360.0f, 360.0f)) {
                        emitterMeshData_->rotation.x = rotationDegrees.x * 3.14159f / 180.0f;
                        emitterMeshData_->rotation.y = rotationDegrees.y * 3.14159f / 180.0f;
                        emitterMeshData_->rotation.z = rotationDegrees.z * 3.14159f / 180.0f;
                    }

                    ImGui::DragFloat3("エミッタの大きさ##Scale", &emitterMeshData_->scale.x, 0.1f);

                    // モデル情報表示
                    if (emitterMeshData_->triangleCount > 0) {
                        ImGui::Spacing();
                        ImGui::Text("三角形数: %d", emitterMeshData_->triangleCount);
                        if (!modelPath_.empty()) {
                            ImGui::Text("モデル: %s", modelPath_.c_str());
                        } else if (primitiveType_ != PrimitiveType::None) {
                            ImGui::Text("プリミティブタイプ");
                        }
                    } 

                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                ImGui::Checkbox("自動更新##Auto", &isAuto_);
                ImGui::Checkbox("エミッター表示##Visible", &isVisible_);
                ImGui::PopStyleColor();
            } else {
                ImGui::PopStyleColor(3);
            }

            ImGui::Spacing();

            // パーティクルグループ設定セクション（既存のコードと同じ）
            if (!particleGroups_.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.2f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.3f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.6f, 0.4f, 1.0f));

                if (ImGui::CollapsingHeader("パーティクルグループ設定##GroupSettings")) {
                    ImGui::PopStyleColor(3);

                    static int selectedGroupIndex = 0;
                    if (selectedGroupIndex >= static_cast<int>(particleGroups_.size())) {
                        selectedGroupIndex = 0;
                    }

                    std::vector<std::string> groupNames;
                    for (const auto &group : particleGroups_) {
                        groupNames.push_back(group->GetGroupName());
                    }

                    std::vector<const char *> groupNameCStrs;
                    for (auto &n : groupNames)
                        groupNameCStrs.push_back(n.c_str());

                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.3f, 0.4f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 0.9f));

                    ImGui::SetNextItemWidth(200.0f);
                    ImGui::Combo("選択中のグループ##GroupCombo", &selectedGroupIndex, groupNameCStrs.data(), (int)groupNameCStrs.size());

                    ImGui::PopStyleColor(3);

                    if (selectedGroupIndex >= 0 && selectedGroupIndex < static_cast<int>(particleGroups_.size())) {
                        ImGui::Separator();
                        particleGroups_[selectedGroupIndex]->SetFrequency(emitterMeshData_->frequency);
                        particleGroups_[selectedGroupIndex]->DrawImGui();
                    }
                } else {
                    ImGui::PopStyleColor(3);
                }
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 0.6f, 1.0f));
                ImGui::Text("GPUパーティクルグループがありません");
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // グループ管理セクション
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.3f, 0.5f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.4f, 0.6f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.5f, 0.7f, 1.0f));

            if (ImGui::CollapsingHeader("GPUグループ管理##GPUGroupManagement")) {
                ImGui::PopStyleColor(3);

                ImGui::Spacing();

                auto allGroups = ParticleCSGroupManager::GetInstance()->GetParticleGroups();

                std::vector<std::string> availableNames;
                std::vector<std::string> attachedNames;

                for (auto *group : allGroups) {
                    const std::string &name = group->GetGroupName();
                    if (particleGroupNames_.contains(name)) {
                        attachedNames.push_back(name);
                    } else {
                        availableNames.push_back(name);
                    }
                }

                static std::vector<int> leftSelected;
                static std::vector<int> rightSelected;

                std::vector<const char *> availableItems;
                for (auto &name : availableNames)
                    availableItems.push_back(name.c_str());

                std::vector<const char *> attachedItems;
                for (auto &name : attachedNames)
                    attachedItems.push_back(name.c_str());

                leftSelected.erase(std::remove_if(leftSelected.begin(), leftSelected.end(),
                                                  [&](int i) { return i >= (int)availableNames.size(); }),
                                   leftSelected.end());
                rightSelected.erase(std::remove_if(rightSelected.begin(), rightSelected.end(),
                                                   [&](int i) { return i >= (int)attachedNames.size(); }),
                                    rightSelected.end());

                float width = ImGui::GetContentRegionAvail().x;
                float halfWidth = width * 0.45f;

                // ヘッダーテキストのスタイル
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
                ImGui::Text("利用可能なGPUグループ");
                ImGui::SameLine(width - halfWidth - 50);
                ImGui::Text("アタッチ済みGPUグループ");
                ImGui::PopStyleColor();

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));

                // 左リスト用のスタイル設定
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.15f, 0.2f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.4f, 0.5f, 0.8f));

                ImGui::BeginChild("gpu_available_groups##GPUAvailableGroups", ImVec2(halfWidth, 200), true);

                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.7f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));

                if (availableItems.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::Text("利用可能なGPUグループがありません");
                    ImGui::PopStyleColor();
                } else {
                    for (int i = 0; i < availableItems.size(); ++i) {
                        bool selected = std::find(leftSelected.begin(), leftSelected.end(), i) != leftSelected.end();
                        std::string selectableId = std::string(availableItems[i]) + "##GPUAvailable" + std::to_string(i);
                        if (ImGui::Selectable(selectableId.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                            if (!ImGui::GetIO().KeyCtrl)
                                leftSelected.clear();

                            auto it = std::find(leftSelected.begin(), leftSelected.end(), i);
                            if (it != leftSelected.end())
                                leftSelected.erase(it);
                            else
                                leftSelected.push_back(i);

                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                                auto group = ParticleCSGroupManager::GetInstance()->GetParticleCSGroup(availableNames[i]);
                                AddParticleGroup(group);
                                leftSelected.clear();
                            }
                        }
                    }
                }

                ImGui::PopStyleColor(3);
                ImGui::EndChild();

                ImGui::SameLine();

                // 中央のボタン群
                ImGui::BeginGroup();
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 12));

                // ボタンのスタイル設定
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));

                bool canMoveRight = !leftSelected.empty();
                if (!canMoveRight) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                }

                if (ImGui::Button("追加 >>##GPUAddButton", ImVec2(80, 35)) && canMoveRight) {
                    for (int idx : leftSelected) {
                        auto group = ParticleCSGroupManager::GetInstance()->GetParticleCSGroup(availableNames[idx]);
                        AddParticleGroup(group);
                    }
                    leftSelected.clear();
                }

                if (!canMoveRight) {
                    ImGui::PopStyleColor(3);
                }

                bool canMoveLeft = !rightSelected.empty();
                if (!canMoveLeft) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                }

                if (ImGui::Button("<< 削除##GPURemoveButton", ImVec2(80, 35)) && canMoveLeft) {
                    for (int idx : rightSelected) {
                        RemoveParticleGroup(attachedNames[idx]);
                    }
                    rightSelected.clear();
                }

                if (!canMoveLeft) {
                    ImGui::PopStyleColor(3);
                }

                ImGui::PopStyleColor(3); // Button colors
                ImGui::PopStyleVar();
                ImGui::EndGroup();

                ImGui::SameLine();

                ImGui::BeginChild("gpu_attached_groups##GPUAttachedGroups", ImVec2(halfWidth, 200), true);

                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.4f, 0.2f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.7f, 0.5f, 0.3f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.8f, 0.6f, 0.4f, 1.0f));

                if (attachedItems.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    ImGui::Text("アタッチされたGPUグループがありません");
                    ImGui::PopStyleColor();
                } else {
                    for (int i = 0; i < attachedItems.size(); ++i) {
                        bool selected = std::find(rightSelected.begin(), rightSelected.end(), i) != rightSelected.end();
                        std::string selectableId = std::string(attachedItems[i]) + "##GPUAttached" + std::to_string(i);
                        if (ImGui::Selectable(selectableId.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                            if (!ImGui::GetIO().KeyCtrl)
                                rightSelected.clear();

                            auto it = std::find(rightSelected.begin(), rightSelected.end(), i);
                            if (it != rightSelected.end())
                                rightSelected.erase(it);
                            else
                                rightSelected.push_back(i);

                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                                RemoveParticleGroup(attachedNames[i]);
                                rightSelected.clear();
                            }
                        }
                    }
                }

                ImGui::PopStyleColor(3);
                ImGui::EndChild();

                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar();

                ImGui::Spacing();

                // 操作説明
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::Text("操作: Ctrlキー + クリックで複数選択, ダブルクリックで追加/削除");
                ImGui::PopStyleColor();

            } else {
                ImGui::PopStyleColor(3);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // ファイル操作セクション
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.3f, 0.2f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.4f, 0.3f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.6f, 0.5f, 0.4f, 1.0f));

            if (ImGui::CollapsingHeader("GPUファイル操作##GPUFileOperations")) {
                ImGui::PopStyleColor(3);

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

                if (ImGui::Button("GPU設定を保存##GPUSaveButton", ImVec2(120, 35))) {
                    SaveSetting();
                    MessageBoxA(NULL, "Success Save!", "ParticleCSEmitter", MB_OK | MB_ICONINFORMATION);
                }
                ImGui::PopStyleColor(3);

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("現在のGPUパーティクル設定をファイルに保存します");
                }

                ImGui::Spacing();

            } else {
                ImGui::PopStyleColor(3);
            }

            // メインウィンドウの背景色をポップ
            ImGui::PopStyleColor();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

#define NOMINMAX
#include "ParticleCSEmitter.h"
#include "ParticleCSGroupManager.h"
#include <Frame.h>
#include <Line/DrawLine3D.h>
#include <Particle/ParticleCommon.h>
#include <random>
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

    // çƒä½“ã‚¨ãƒŸãƒƒã‚¿ãƒ¼ã®å ´åˆ
    if (emitterMeshData_->triangleCount == 0) {
        Vector3 center = emitterMeshData_->translate;
        Vector3 scale = emitterMeshData_->scale;
        Vector4 color = {1.0f, 1.0f, 0.0f, 1.0f};

        float maxRadius = std::max(std::max(scale.x, scale.y), scale.z);
        DrawLine3D::GetInstance()->DrawSphere(center, color, maxRadius, 16);

    } else {
        // ãƒ¡ãƒƒã‚·ãƒ¥ã‚¨ãƒŸãƒƒã‚¿ãƒ¼ã®ãƒˆãƒ©ãƒ³ã‚¹ãƒ•ã‚©ãƒ¼ãƒ è¡Œåˆ—ã‚’ä½œæˆ
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

        // ä¸‰è§’å½¢ã®å¯è¦–åŒ–
        for (const auto &tri : triangleInfoList_) {
            Vector3 v0 = tri.v0;
            Vector3 v1 = tri.v1;
            Vector3 v2 = tri.v2;

            // ãƒ¯ãƒ¼ãƒ«ãƒ‰ç©ºé–“ã¸å¤‰æ›
            v0 = Transformation(v0, transformMatrix);
            v1 = Transformation(v1, transformMatrix);
            v2 = Transformation(v2, transformMatrix);

            // ä¸‰è§’å½¢ã®è¾ºã‚’ç·šã§æç”»
            DrawLine3D::GetInstance()->SetPoints(v0, v1);
            DrawLine3D::GetInstance()->SetPoints(v1, v2);
            DrawLine3D::GetInstance()->SetPoints(v2, v0);
        }
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
    emitterMeshData_->rotation = Vector3(0.0f, 0.0f, 0.0f); // å›žè»¢ã‚’åˆæœŸåŒ–
    emitterMeshData_->scale = Vector3(1.0f, 1.0f, 1.0f);    // ã‚¹ã‚±ãƒ¼ãƒ«ã‚’åˆæœŸåŒ–
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

        if (emitterMeshData_->triangleCount > 0 && triangleInfoResource_ && triangleCDFResource_) {
            commandList->SetComputeRootDescriptorTable(6, triangleInfoSrvHandle_.second);
            commandList->SetComputeRootDescriptorTable(7, triangleCDFSrvHandle_.second);
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

void ParticleCSEmitter::CreateModelTriangles() {
    if (modelData_.meshes.empty())
        return;

    triangleInfoList_.clear();
    triangleCDF_.clear();
    std::vector<float> triangleAreas;

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

            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;
            Vector3 crossProd = edge1.Cross(edge2);
            float area = crossProd.Length() * 0.5f;

            if (area > 1e-6f) {
                triangleAreas.push_back(area);

                TriangleInfo triInfo;
                triInfo.v0 = v0;
                triInfo.v1 = v1;
                triInfo.v2 = v2;
                triInfo.padding0 = 0.0f;
                triInfo.padding1 = 0.0f;
                triInfo.padding2 = 0.0f;

                triangleInfoList_.push_back(triInfo);
            }
        }
    }

    if (triangleInfoList_.empty())
        return;

    std::vector<size_t> indices(triangleInfoList_.size());
    for (size_t i = 0; i < indices.size(); i++) {
        indices[i] = i;
    }

    std::vector<TriangleInfo> shuffledTriangles;
    std::vector<float> shuffledAreas;
    shuffledTriangles.reserve(triangleInfoList_.size());
    shuffledAreas.reserve(triangleAreas.size());

    for (size_t idx : indices) {
        shuffledTriangles.push_back(triangleInfoList_[idx]);
        shuffledAreas.push_back(triangleAreas[idx]);
    }

    triangleInfoList_ = std::move(shuffledTriangles);
    triangleAreas = std::move(shuffledAreas);

    float totalArea = 0.0f;
    for (float area : triangleAreas) {
        totalArea += area;
    }

    triangleCDF_.resize(triangleAreas.size());
    float accum = 0.0f;
    for (size_t i = 0; i < triangleAreas.size(); i++) {
        accum += triangleAreas[i] / totalArea;
        triangleCDF_[i] = accum;
    }

    size_t triangleInfoBufferSize = sizeof(TriangleInfo) * triangleInfoList_.size();
    triangleInfoResource_ = dxCommon_->CreateBufferResource(triangleInfoBufferSize);
    triangleInfoResource_->Map(0, nullptr, reinterpret_cast<void **>(&triangleInfoData_));
    std::memcpy(triangleInfoData_, triangleInfoList_.data(), triangleInfoBufferSize);

    triangleInfoSrvIndex_ = srvManager_->Allocate() + 1;
    triangleInfoSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(triangleInfoSrvIndex_);
    triangleInfoSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(triangleInfoSrvIndex_);
    srvManager_->CreateSRVforStructuredBuffer(triangleInfoSrvIndex_, triangleInfoResource_.Get(),
                                              static_cast<uint32_t>(triangleInfoList_.size()), sizeof(TriangleInfo));

    size_t cdfBufferSize = sizeof(float) * triangleCDF_.size();
    triangleCDFResource_ = dxCommon_->CreateBufferResource(cdfBufferSize);
    triangleCDFResource_->Map(0, nullptr, reinterpret_cast<void **>(&triangleCDFData_));
    std::memcpy(triangleCDFData_, triangleCDF_.data(), cdfBufferSize);

    triangleCDFSrvIndex_ = srvManager_->Allocate() + 1;
    triangleCDFSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(triangleCDFSrvIndex_);
    triangleCDFSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(triangleCDFSrvIndex_);
    srvManager_->CreateSRVforStructuredBuffer(triangleCDFSrvIndex_, triangleCDFResource_.Get(),
                                              static_cast<uint32_t>(triangleCDF_.size()), sizeof(float));

    emitterMeshData_->triangleCount = static_cast<uint32_t>(triangleInfoList_.size());
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

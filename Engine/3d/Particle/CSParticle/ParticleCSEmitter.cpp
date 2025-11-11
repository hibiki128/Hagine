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
    CreateModelEdges();
}

void ParticleCSEmitter::Initialize(const std::string &name, PrimitiveType primitiveType) {
    Initialize(name);
    primitiveType_ = primitiveType;
    LoadPrimitiveModel(primitiveType);
    CreateModelTriangles();
    CreateModelEdges();
}

void ParticleCSEmitter::Draw(const ViewProjection &vp) {
    DrawEmitter();

    for (auto &group : particleGroups_) {
        group->Update(vp);
        dxCommon_->TransitionUAVBarrier(group->GetOutputParticleResource().Get());
        EmitterDisPatch();
        group->UpdateParticleCSDisPatch();
        group->CountAliveParticles();
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
    Vector3 translate = emitterMeshData_->translate;
    Quaternion rotation = emitterMeshData_->rotation;
    Vector3 scale = emitterMeshData_->scale;

    Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
    Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(rotation);
    Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);
    Matrix4x4 transformMatrix = MakeAffineMatrix(scale, rotation, translate);

    if (emitterMeshData_->emitFromSurface == 2 && !edgeInfoList_.empty()) {
        Vector4 color = {1.0f, 0.5f, 0.0f, 1.0f};
        for (const auto &edge : edgeInfoList_) {
            Vector3 v0 = Transformation(edge.v0, transformMatrix);
            Vector3 v1 = Transformation(edge.v1, transformMatrix);
            DrawLine3D::GetInstance()->SetPoints(v0, v1);
        }
    } else if (!triangleInfoList_.empty()) {
        Vector4 color = {0.0f, 1.0f, 0.0f, 1.0f};
        for (const auto &tri : triangleInfoList_) {
            Vector3 v0 = Transformation(tri.v0, transformMatrix);
            Vector3 v1 = Transformation(tri.v1, transformMatrix);
            Vector3 v2 = Transformation(tri.v2, transformMatrix);

            DrawLine3D::GetInstance()->SetPoints(v0, v1);
            DrawLine3D::GetInstance()->SetPoints(v1, v2);
            DrawLine3D::GetInstance()->SetPoints(v2, v0);
        }
    } else {
        Vector3 center = emitterMeshData_->translate;
        Vector4 color = {1.0f, 1.0f, 0.0f, 1.0f};
        float maxRadius = std::max(std::max(scale.x, scale.y), scale.z);
        DrawLine3D::GetInstance()->DrawSphere(center, color, maxRadius, 16);
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
    emitterMeshData_->rotation = Quaternion::IdentityQuaternion();
    emitterMeshData_->scale = Vector3(1.0f, 1.0f, 1.0f);
    emitterMeshData_->triangleCount = 0;
    emitterMeshData_->emit = 0;
    emitterMeshData_->emitFromSurface = 1;
    emitterMeshData_->edgeCount = 0;
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

        // 三角形情報を設定
        if (emitterMeshData_->triangleCount > 0 && triangleInfoResource_ && triangleCDFResource_) {
            commandList->SetComputeRootDescriptorTable(6, triangleInfoSrvHandle_.second);
            commandList->SetComputeRootDescriptorTable(7, triangleCDFSrvHandle_.second);
        }

        // エッジ情報を設定
        if (emitterMeshData_->edgeCount > 0 && edgeInfoResource_) {
            commandList->SetComputeRootDescriptorTable(8, edgeInfoSrvHandle_.second);
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

    // 最後の値を強制的に1.0にして誤差を修正
    if (!triangleCDF_.empty()) {
        triangleCDF_.back() = 1.0f;
    }

    int histogram[10] = {0};
    for (float cdf : triangleCDF_) {
        int bucket = static_cast<int>(cdf * 10.0f);
        if (bucket >= 10)
            bucket = 9;
        histogram[bucket]++;
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

void ParticleCSEmitter::CreateModelEdges() {
    if (modelData_.meshes.empty())
        return;

    edgeInfoList_.clear();

    std::map<std::pair<uint32_t, uint32_t>, int> edgeMap;

    for (const auto &mesh : modelData_.meshes) {
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            uint32_t i0 = mesh.indices[i];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];

            if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size())
                continue;

            std::array<std::pair<uint32_t, uint32_t>, 3> edges = {{{std::min(i0, i1), std::max(i0, i1)},
                                                                   {std::min(i1, i2), std::max(i1, i2)},
                                                                   {std::min(i2, i0), std::max(i2, i0)}}};

            for (const auto &edge : edges) {
                edgeMap[edge]++;
            }
        }
    }

    bool isClosedMesh = true;
    for (const auto &[edge, count] : edgeMap) {
        if (count == 1) {
            isClosedMesh = false;
            break;
        }
    }

    for (const auto &[edge, count] : edgeMap) {
        if (!isClosedMesh && count != 1)
            continue;

        uint32_t idx0 = edge.first;
        uint32_t idx1 = edge.second;

        Vector3 v0, v1;
        bool found = false;
        for (const auto &mesh : modelData_.meshes) {
            if (idx0 < mesh.vertices.size() && idx1 < mesh.vertices.size()) {
                v0 = Vector3(mesh.vertices[idx0].position.x,
                             mesh.vertices[idx0].position.y,
                             mesh.vertices[idx0].position.z);
                v1 = Vector3(mesh.vertices[idx1].position.x,
                             mesh.vertices[idx1].position.y,
                             mesh.vertices[idx1].position.z);
                found = true;
                break;
            }
        }

        if (found) {
            EdgeInfo edgeInfo;
            edgeInfo.v0 = v0;
            edgeInfo.v1 = v1;
            edgeInfo.padding0 = 0.0f;
            edgeInfo.padding1 = 0.0f;
            edgeInfoList_.push_back(edgeInfo);
        }
    }

    if (edgeInfoList_.empty())
        return;

    size_t edgeInfoBufferSize = sizeof(EdgeInfo) * edgeInfoList_.size();
    edgeInfoResource_ = dxCommon_->CreateBufferResource(edgeInfoBufferSize);
    edgeInfoResource_->Map(0, nullptr, reinterpret_cast<void **>(&edgeInfoData_));
    std::memcpy(edgeInfoData_, edgeInfoList_.data(), edgeInfoBufferSize);

    edgeInfoSrvIndex_ = srvManager_->Allocate() + 1;
    edgeInfoSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(edgeInfoSrvIndex_);
    edgeInfoSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(edgeInfoSrvIndex_);
    srvManager_->CreateSRVforStructuredBuffer(edgeInfoSrvIndex_, edgeInfoResource_.Get(),
                                              static_cast<uint32_t>(edgeInfoList_.size()), sizeof(EdgeInfo));

    emitterMeshData_->edgeCount = static_cast<uint32_t>(edgeInfoList_.size());
}

size_t ParticleCSEmitter::GetTotalAliveParticles() {
    size_t total = 0;
    for (auto &group : particleGroups_) {
        total += group->GetAliveParticleCount();
    }
    return total;
}

std::vector<ParticleCSEmitter::GroupStatistics> ParticleCSEmitter::GetGroupStatistics() {
    std::vector<GroupStatistics> stats;

    for (auto &group : particleGroups_) {
        GroupStatistics stat;
        stat.groupName = group->GetGroupName();
        stat.aliveCount = group->GetAliveParticleCount();
        stats.push_back(stat);
    }

    return stats;
}

void ParticleCSEmitter::SaveSetting() {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleCS", name_);

    data->Save("isAuto", isAuto_);
    data->Save("isVisible", isVisible_);
    data->Save("frequency", emitterMeshData_->frequency);
    data->Save("frequencyTime", emitterMeshData_->frequencyTime);
    data->Save<Vector3>("translate", emitterMeshData_->translate);
    data->Save<Quaternion>("rotation", emitterMeshData_->rotation);
    data->Save<Vector3>("scale", emitterMeshData_->scale);
    data->Save("emitFromSurface", emitterMeshData_->emitFromSurface);
    data->Save("modelPath", modelPath_);
    data->Save("primitiveType", static_cast<int>(primitiveType_));

    data->Save("particleGroupCount", static_cast<int>(particleGroups_.size()));

    for (int i = 0; i < particleGroups_.size(); i++) {
        auto &group = particleGroups_[i];
        std::string prefix = "group_" + std::to_string(i) + "_";

        data->Save(prefix + "name", group->GetGroupName());
        data->Save(prefix + "minLifetime", group->GetSettingsData()->lifeTimeMin);
        data->Save(prefix + "maxLifetime", group->GetSettingsData()->lifeTimeMax);
        data->Save(prefix + "minScale", group->GetSettingsData()->scaleMin);
        data->Save(prefix + "maxScale", group->GetSettingsData()->scaleMax);
        data->Save(prefix + "minVelocity", group->GetSettingsData()->velocityMin);
        data->Save(prefix + "maxVelocity", group->GetSettingsData()->velocityMax);
        data->Save(prefix + "startColor", group->GetSettingsData()->startColor);
        data->Save(prefix + "endColor", group->GetSettingsData()->endColor);
        data->Save(prefix + "enableLifetimeScale", group->GetSettingsData()->enableLifetimeScale);
        data->Save(prefix + "enableRandomColor", group->GetSettingsData()->enableRandomColor);
        data->Save(prefix + "enableSinScale", group->GetSettingsData()->enableSinScale);
        data->Save(prefix + "sinScaleFrequency", group->GetSettingsData()->sinScaleFrequency);
        data->Save(prefix + "sinScaleAmplitude", group->GetSettingsData()->sinScaleAmplitude);
        data->Save(prefix + "emitCount", group->GetSettingsData()->emitCount);
        data->Save(prefix + "enableGravity", group->GetSettingsData()->enableGravity);
        data->Save(prefix + "gravity", group->GetSettingsData()->gravity);
        data->Save(prefix + "blendMode", static_cast<int>(group->GetParticleGroupData().blendMode));
    }
}

void ParticleCSEmitter::LoadSetting() {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleCS", name_);

    isAuto_ = data->Load("isAuto", false);
    isVisible_ = data->Load("isVisible", true);
    emitterMeshData_->frequency = data->Load("frequency", 0.1f);
    emitterMeshData_->frequencyTime = data->Load("frequencyTime", 0.0f);
    emitterMeshData_->translate = data->Load<Vector3>("translate", Vector3(0.0f, 0.0f, 0.0f));
    emitterMeshData_->rotation = data->Load<Quaternion>("rotation", Quaternion::IdentityQuaternion());
    emitterMeshData_->scale = data->Load<Vector3>("scale", Vector3(1.0f, 1.0f, 1.0f));
    emitterMeshData_->emitFromSurface = data->Load<uint32_t>("emitFromSurface", 1);

    modelPath_ = data->Load("modelPath", std::string(""));
    primitiveType_ = static_cast<PrimitiveType>(data->Load("primitiveType", static_cast<int>(PrimitiveType::None)));

    if (!modelPath_.empty()) {
        LoadModel(modelPath_);
        CreateModelTriangles();
    } else if (primitiveType_ != PrimitiveType::None) {
        LoadPrimitiveModel(primitiveType_);
        CreateModelTriangles();
        CreateModelEdges();
    }

    groupNum_ = data->Load("particleGroupCount", 0);
    for (int i = 0; i < groupNum_; i++) {
        std::string prefix = "group_" + std::to_string(i) + "_";
        std::string groupName = data->Load(prefix + "name", std::string(""));

        auto group = ParticleCSGroupManager::GetInstance()->GetIndependentParticleGroup(groupName);
        if (!group)
            continue;

        ParticleCSSettings settings;
        settings.lifeTimeMin = data->Load(prefix + "minLifetime", 1.0f);
        settings.lifeTimeMax = data->Load(prefix + "maxLifetime", 1.0f);
        settings.scaleMin = data->Load(prefix + "minScale", 1.0f);
        settings.scaleMax = data->Load(prefix + "maxScale", 1.0f);
        settings.velocityMin = data->Load<Vector3>(prefix + "minVelocity", {0.0f, 0.0f, 0.0f});
        settings.velocityMax = data->Load<Vector3>(prefix + "maxVelocity", {0.0f, 0.0f, 0.0f});
        settings.startColor = data->Load(prefix + "startColor", Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        settings.endColor = data->Load(prefix + "endColor", Vector4(1.0f, 1.0f, 1.0f, 0.0f));
        settings.enableLifetimeScale = data->Load<uint32_t>(prefix + "enableLifetimeScale", 0);
        settings.enableRandomColor = data->Load<uint32_t>(prefix + "enableRandomColor", 0);
        settings.enableSinScale = data->Load<uint32_t>(prefix + "enableSinScale", 0);
        settings.sinScaleFrequency = data->Load(prefix + "sinScaleFrequency", 5.0f);
        settings.sinScaleAmplitude = data->Load(prefix + "sinScaleAmplitude", 0.3f);
        settings.emitCount = data->Load<uint32_t>(prefix + "emitCount", 10);
        settings.enableGravity = data->Load<uint32_t>(prefix + "enableGravity", false);
        settings.gravity = data->Load<Vector3>(prefix + "gravity", {0.0f, 0.0f, 0.0f});
        settings.maxParticleCount = group->GetMaxParticleCount();

        group->SetSettingData(settings);
        group->SetBlendMode(static_cast<BlendMode>(data->Load<int>(prefix + "blendMode", static_cast<int>(BlendMode::kAdd))));

        AddParticleGroup(group);
    }
}

void ParticleCSEmitter::LoadCloneSetting() {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleCS", name_);
    if (!data->Exists()) {
        return;
    } else {
        particleGroups_.clear();
        particleGroupNames_.clear();
    }

    isAuto_ = data->Load("isAuto", false);
    isVisible_ = data->Load("isVisible", true);
    emitterMeshData_->frequency = data->Load("frequency", 0.1f);
    emitterMeshData_->frequencyTime = data->Load("frequencyTime", 0.0f);
    emitterMeshData_->translate = data->Load<Vector3>("translate", Vector3(0.0f, 0.0f, 0.0f));
    emitterMeshData_->rotation = data->Load<Quaternion>("rotation", Quaternion::IdentityQuaternion());
    emitterMeshData_->scale = data->Load<Vector3>("scale", Vector3(1.0f, 1.0f, 1.0f));
    emitterMeshData_->emitFromSurface = data->Load<uint32_t>("emitFromSurface", 1);

    modelPath_ = data->Load("modelPath", std::string(""));
    primitiveType_ = static_cast<PrimitiveType>(data->Load("primitiveType", static_cast<int>(PrimitiveType::None)));

    if (!modelPath_.empty()) {
        LoadModel(modelPath_);
        CreateModelTriangles();
        CreateModelEdges();
    } else if (primitiveType_ != PrimitiveType::None) {
        LoadPrimitiveModel(primitiveType_);
        CreateModelTriangles();
        CreateModelEdges();
    }

    groupNum_ = data->Load("particleGroupCount", 0);
    for (int i = 0; i < groupNum_; i++) {
        std::string prefix = "group_" + std::to_string(i) + "_";
        std::string groupName = data->Load(prefix + "name", std::string(""));

        auto group = ParticleCSGroupManager::GetInstance()->GetIndependentParticleGroup(groupName);
        if (!group)
            continue;

        ParticleCSSettings settings;
        settings.lifeTimeMin = data->Load(prefix + "minLifetime", 1.0f);
        settings.lifeTimeMax = data->Load(prefix + "maxLifetime", 1.0f);
        settings.scaleMin = data->Load(prefix + "minScale", 1.0f);
        settings.scaleMax = data->Load(prefix + "maxScale", 1.0f);
        settings.velocityMin = data->Load<Vector3>(prefix + "minVelocity", {0.0f, 0.0f, 0.0f});
        settings.velocityMax = data->Load<Vector3>(prefix + "maxVelocity", {0.0f, 0.0f, 0.0f});
        settings.startColor = data->Load(prefix + "startColor", Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        settings.endColor = data->Load(prefix + "endColor", Vector4(1.0f, 1.0f, 1.0f, 0.0f));
        settings.enableLifetimeScale = data->Load<uint32_t>(prefix + "enableLifetimeScale", 0);
        settings.enableRandomColor = data->Load<uint32_t>(prefix + "enableRandomColor", 0);
        settings.enableSinScale = data->Load<uint32_t>(prefix + "enableSinScale", 0);
        settings.sinScaleFrequency = data->Load(prefix + "sinScaleFrequency", 5.0f);
        settings.sinScaleAmplitude = data->Load(prefix + "sinScaleAmplitude", 0.3f);
        settings.emitCount = data->Load<uint32_t>(prefix + "emitCount", 10);
        settings.enableGravity = data->Load<uint32_t>(prefix + "enableGravity", false);
        settings.gravity = data->Load<Vector3>(prefix + "gravity", {0.0f, 0.0f, 0.0f});
        settings.maxParticleCount = group->GetMaxParticleCount();

        group->SetSettingData(settings);
        group->SetBlendMode(static_cast<BlendMode>(data->Load<int>(prefix + "blendMode", static_cast<int>(BlendMode::kAdd))));

        AddParticleGroup(group);
    }
}

void ParticleCSEmitter::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::BeginTabBar("EmitterTabBar")) {
        if (ImGui::BeginTabItem(name_.c_str())) {
            ImGuiStyle &style = ImGui::GetStyle();
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.15f, 1.00f));

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

                Vector3 currentEuler = emitterMeshData_->rotation.ToEulerDegrees();
                ImGui::Text("現在の回転: %.1f° %.1f° %.1f°", currentEuler.x, currentEuler.y, currentEuler.z);

                static Vector3 deltaRotation = {0.0f, 0.0f, 0.0f};
                if (ImGui::DragFloat3("##EmitterRotation", &deltaRotation.x, 0.1f, -10.0f, 10.0f, "%.1f°")) {
                    Quaternion currentRotation = emitterMeshData_->rotation;
                    Quaternion deltaQuatX = Quaternion::FromAxisAngle(Vector3(1, 0, 0), deltaRotation.x * std::numbers::pi_v<float> / 180.0f);
                    Quaternion deltaQuatY = Quaternion::FromAxisAngle(Vector3(0, 1, 0), deltaRotation.y * std::numbers::pi_v<float> / 180.0f);
                    Quaternion deltaQuatZ = Quaternion::FromAxisAngle(Vector3(0, 0, 1), deltaRotation.z * std::numbers::pi_v<float> / 180.0f);
                    Quaternion deltaQuat = deltaQuatY * deltaQuatX * deltaQuatZ;
                    Quaternion newRotation = currentRotation * deltaQuat;
                    emitterMeshData_->rotation = newRotation.Normalize();
                    deltaRotation = {0.0f, 0.0f, 0.0f};
                }

                ImGui::SameLine();
                if (ImGui::Button("リセット##EmitterRotation")) {
                    emitterMeshData_->rotation = Quaternion::IdentityQuaternion();
                    deltaRotation = {0.0f, 0.0f, 0.0f};
                }

                ImGui::DragFloat3("エミッタの大きさ##Scale", &emitterMeshData_->scale.x, 0.1f);

                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::Separator();

                // 発生位置設定（ラジオボタンで3択）
                if (emitterMeshData_->triangleCount > 0 || emitterMeshData_->edgeCount > 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.6f, 1.0f));
                    ImGui::Text("発生位置:");
                    ImGui::PopStyleColor();

                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.4f, 0.6f));
                    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));

                    int emitMode = static_cast<int>(emitterMeshData_->emitFromSurface);

                    ImGui::RadioButton("内部から発生##EmitInternal", &emitMode, 0);
                    ImGui::SameLine();
                    ImGui::RadioButton("表面から発生##EmitSurface", &emitMode, 1);
                    ImGui::SameLine();
                    ImGui::RadioButton("線上から発生##EmitEdge", &emitMode, 2);

                    emitterMeshData_->emitFromSurface = static_cast<uint32_t>(emitMode);

                    ImGui::PopStyleColor(2);

                    // ツールチップ
                    if (ImGui::IsItemHovered()) {
                        const char *tooltip = "";
                        if (emitMode == 0)
                            tooltip = "メッシュの内側全体からパーティクルが発生します";
                        else if (emitMode == 1)
                            tooltip = "メッシュの表面からパーティクルが発生します";
                        else if (emitMode == 2)
                            tooltip = "メッシュのエッジ（線）上からパーティクルが発生します";
                        ImGui::SetTooltip("%s", tooltip);
                    }
                }

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.2f, 0.5f));

                // モデル情報表示
                if (emitterMeshData_->triangleCount > 0) {
                    ImGui::Spacing();
                    ImGui::Text("三角形数: %d", emitterMeshData_->triangleCount);
                    if (emitterMeshData_->edgeCount > 0) {
                        ImGui::Text("エッジ数: %d", emitterMeshData_->edgeCount);
                    }
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
#endif // USE_IMGUI
}


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
    CreateEmitterSphereResource();
    CreateEmitterSettingsResource();
    LoadSetting();
}

void ParticleCSEmitter::Initialize(const std::string &name, const std::string &modelPath, EmitterType type) {
    Initialize(name);
    currentEmitterType_ = type;
    modelPath_ = modelPath;

    if (type == EmitterType::Mesh) {
        LoadModel(modelPath);
        CreateModelTriangles();
        CreateEmitterMeshResource();
    }

    CreateEmitterSettingsResource();
}

void ParticleCSEmitter::Initialize(const std::string &name, PrimitiveType primitiveType, EmitterType type) {
    Initialize(name);

    currentEmitterType_ = type;
    primitiveType_ = primitiveType;

    if (type == EmitterType::Mesh) {
        LoadPrimitiveModel(primitiveType);
        CreateModelTriangles();
        CreateEmitterMeshResource();
    }

    CreateEmitterSettingsResource();
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
        if (emitterMeshData_) {
            emitterMeshData_->emit = 0;
        }
        if (emitterSphereData_) {
            emitterSphereData_->emit = 0;
        }
    }
}

void ParticleCSEmitter::DrawEmitter() {
    if (!isVisible_)
        return;

    if (currentEmitterType_ == EmitterType::Sphere && emitterSphereData_) {
        Vector3 center = emitterSphereData_->translate;
        Vector3 radius = emitterSphereData_->radius;
        Vector3 rotation = emitterSphereData_->rotation;
        Vector3 scale = emitterSphereData_->scale;
        Vector4 color = {1.0f, 1.0f, 0.0f, 1.0f}; // Yellow

        // スケールを適用した半径で楕円体を描画
        Vector3 scaledRadius = {
            radius.x * scale.x,
            radius.y * scale.y,
            radius.z * scale.z};

        // 回転とスケールを考慮した楕円体として描画
        // 簡単な実装として、X軸方向の半径を使用
        float maxRadius = max(max(scaledRadius.x, scaledRadius.y), scaledRadius.z);
        DrawLine3D::GetInstance()->DrawSphere(center, color, maxRadius, 16);

    } else if (currentEmitterType_ == EmitterType::Mesh && !triangles_.empty() && emitterMeshData_) {
        Vector4 color = {0.0f, 1.0f, 0.0f, 1.0f}; // Green for mesh emitters
        Vector3 translate = emitterMeshData_->translate;
        Vector3 rotation = emitterMeshData_->rotation;
        Vector3 scale = emitterMeshData_->scale;

        // 変換行列を作成
        Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
        Matrix4x4 rotateMatrixX = MakeRotateXMatrix(rotation.x);
        Matrix4x4 rotateMatrixY = MakeRotateYMatrix(rotation.y);
        Matrix4x4 rotateMatrixZ = MakeRotateZMatrix(rotation.z);
        Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

        // 変換行列の合成：スケール -> 回転 -> 平行移動
        Matrix4x4 transformMatrix = translateMatrix * rotateMatrixZ * rotateMatrixY * rotateMatrixX * scaleMatrix;

        // 各三角形の頂点を変換して描画
        for (const auto &triangle : triangles_) {
            // 各頂点を変換
            Vector4 v0_4 = {triangle.v0.x, triangle.v0.y, triangle.v0.z, 1.0f};
            Vector4 v1_4 = {triangle.v1.x, triangle.v1.y, triangle.v1.z, 1.0f};
            Vector4 v2_4 = {triangle.v2.x, triangle.v2.y, triangle.v2.z, 1.0f};

            v0_4 = Transformation(v0_4, transformMatrix);
            v1_4 = Transformation(v1_4, transformMatrix);
            v2_4 = Transformation(v2_4, transformMatrix);

            Vector3 v0 = {v0_4.x, v0_4.y, v0_4.z};
            Vector3 v1 = {v1_4.x, v1_4.y, v1_4.z};
            Vector3 v2 = {v2_4.x, v2_4.y, v2_4.z};

            // 三角形の3つの辺を描画
            DrawLine3D::GetInstance()->SetPoints(v0, v1, color);
            DrawLine3D::GetInstance()->SetPoints(v1, v2, color);
            DrawLine3D::GetInstance()->SetPoints(v2, v0, color);
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
    if (currentEmitterType_ == EmitterType::Sphere && emitterSphereData_) {
        emitterSphereData_->frequencyTime += Frame::DeltaTime();
        if (emitterSphereData_->frequency <= emitterSphereData_->frequencyTime) {
            emitterSphereData_->frequencyTime -= emitterSphereData_->frequency;
            emitterSphereData_->emit = 1;
        } else {
            emitterSphereData_->emit = 0;
        }
    } else if (currentEmitterType_ == EmitterType::Mesh && emitterMeshData_) {
        emitterMeshData_->frequencyTime += Frame::DeltaTime();
        if (emitterMeshData_->frequency <= emitterMeshData_->frequencyTime) {
            emitterMeshData_->frequencyTime -= emitterMeshData_->frequency;
            emitterMeshData_->emit = 1;
        } else {
            emitterMeshData_->emit = 0;
        }
    }
}

void ParticleCSEmitter::CreateEmitterSphereResource() {
    emitterSphereResource_ = dxCommon_->CreateBufferResource(sizeof(EmitterSphere));
    emitterSphereResource_->Map(0, nullptr, reinterpret_cast<void **>(&emitterSphereData_));
    emitterSphereData_->frequency = 0.5f;
    emitterSphereData_->frequencyTime = 0.0f;
    emitterSphereData_->translate = Vector3(0.0f, 0.0f, 0.0f);
    emitterSphereData_->radius = Vector3(1.0f, 1.0f, 1.0f);   // Vector3に変更
    emitterSphereData_->rotation = Vector3(0.0f, 0.0f, 0.0f); // 回転を初期化
    emitterSphereData_->scale = Vector3(1.0f, 1.0f, 1.0f);    // スケールを初期化
    emitterSphereData_->emit = 0;
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

        // EmitterSettings
        commandList->SetComputeRootConstantBufferView(3, emitterSettingsResource_->GetGPUVirtualAddress());
        // EmitterSphere
        commandList->SetComputeRootConstantBufferView(4, emitterSphereResource_->GetGPUVirtualAddress());
        // EmitterMesh
        if (emitterMeshResource_) {
            commandList->SetComputeRootConstantBufferView(5, emitterMeshResource_->GetGPUVirtualAddress());
        }
        // Triangle SRV
        if (currentEmitterType_ == EmitterType::Mesh && triangleResource_) {
            commandList->SetComputeRootDescriptorTable(6, triangleSrvHandle_.second);
        }

        commandList->SetComputeRootConstantBufferView(7, group->GetPerFrameResource()->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(8, group->GetSettingsResource()->GetGPUVirtualAddress());

        int dispatchCount = (group->GetSettingsData()->emitCount + threadGroupSize_ - 1) / threadGroupSize_;
        commandList->Dispatch(dispatchCount, 1, 1);

        groupIndex++;
    }
}

std::unique_ptr<ParticleCSEmitter> ParticleCSEmitter::Clone() const {
    auto newEmitter = std::make_unique<ParticleCSEmitter>();

    // 静的関数を使用してnameCounterを取得
    auto &nameCounter = GetNameCounter();

    // ベース名を抽出
    std::string baseName = name_;
    std::regex suffixRegex("_(\\d+)$");
    baseName = std::regex_replace(baseName, suffixRegex, "");

    // 次の番号を決定
    int &counter = nameCounter[baseName];
    ++counter;

    // 新しい名前を作成
    std::string newName = baseName + "_" + std::to_string(counter);

    newEmitter->Initialize(baseName);
    newEmitter->SetName(newName);
    newEmitter->LoadCloneSetting();
    newEmitter->SetActive(this->isActive_);
    newEmitter->isAuto_ = this->isAuto_;
    newEmitter->isVisible_ = this->isVisible_;

    // エミッターリソースを再生成して値だけコピーする
    if (currentEmitterType_ == EmitterType::Sphere) {
        newEmitter->CreateEmitterSphereResource();
        *newEmitter->emitterSphereData_ = *this->emitterSphereData_;
    } else if (currentEmitterType_ == EmitterType::Mesh) {
        newEmitter->CreateEmitterMeshResource();
        *newEmitter->emitterMeshData_ = *this->emitterMeshData_;
    }

    return newEmitter;
}

void ParticleCSEmitter::CreateTriangleResource(const MeshData &meshData) {
    // インデックスから三角形を生成
    triangles_.clear();
    for (size_t i = 0; i < meshData.indices.size(); i += 3) {
        Triangle tri;
        // Vector4からVector3に変換（w成分を除く）
        tri.v0 = Vector3(meshData.vertices[meshData.indices[i]].position.x,
                         meshData.vertices[meshData.indices[i]].position.y,
                         meshData.vertices[meshData.indices[i]].position.z);
        tri.v1 = Vector3(meshData.vertices[meshData.indices[i + 1]].position.x,
                         meshData.vertices[meshData.indices[i + 1]].position.y,
                         meshData.vertices[meshData.indices[i + 1]].position.z);
        tri.v2 = Vector3(meshData.vertices[meshData.indices[i + 2]].position.x,
                         meshData.vertices[meshData.indices[i + 2]].position.y,
                         meshData.vertices[meshData.indices[i + 2]].position.z);
        triangles_.push_back(tri);
    }

    if (!triangles_.empty()) {
        size_t bufferSize = sizeof(Triangle) * triangles_.size();
        triangleResource_ = dxCommon_->CreateBufferResource(bufferSize, true);
        triangleResource_->Map(0, nullptr, reinterpret_cast<void **>(&triangleData_));
        std::memcpy(triangleData_, triangles_.data(), bufferSize);

        // SRVを作成
        CreateTriangleSRV();

        if (emitterMeshData_) {
            emitterMeshData_->triangleCount = static_cast<uint32_t>(triangles_.size());
        }
    }
}

void ParticleCSEmitter::SetMeshData(const MeshData &meshData) {
    if (currentEmitterType_ == EmitterType::Mesh) {
        CreateTriangleResource(meshData);
    }
}

void ParticleCSEmitter::CreateTriangleSRV() {
    if (triangleResource_ && !triangles_.empty()) {
        triangleSrvIndex_ = srvManager_->Allocate() + 1;
        triangleSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(triangleSrvIndex_);
        triangleSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(triangleSrvIndex_);
        srvManager_->CreateSRVforStructuredBuffer(triangleSrvIndex_, triangleResource_.Get(),
                                                  static_cast<uint32_t>(triangles_.size()), sizeof(Triangle));
    }
}

void ParticleCSEmitter::CreateEmitterSettingsResource() {
    emitterSettingsResource_ = dxCommon_->CreateBufferResource(sizeof(EmitterSettings));
    emitterSettingsResource_->Map(0, nullptr, reinterpret_cast<void **>(&emitterSettingsData_));
    emitterSettingsData_->isSphere = (currentEmitterType_ == EmitterType::Sphere) ? 1 : 0;
}

void ParticleCSEmitter::SwitchEmitterType(EmitterType type) {
    currentEmitterType_ = type;
    if (emitterSettingsData_) {
        emitterSettingsData_->isSphere = (type == EmitterType::Sphere) ? 1 : 0;
    }
}

void ParticleCSEmitter::CreateModelTriangles() {
    if (modelData_.meshes.empty())
        return;

    triangles_.clear();

    // Process all meshes in the model
    for (const auto &mesh : modelData_.meshes) {
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            Triangle tri;
            // Convert Vector4 positions to Vector3 (remove w component)
            tri.v0 = Vector3(mesh.vertices[mesh.indices[i]].position.x,
                             mesh.vertices[mesh.indices[i]].position.y,
                             mesh.vertices[mesh.indices[i]].position.z);
            tri.v1 = Vector3(mesh.vertices[mesh.indices[i + 1]].position.x,
                             mesh.vertices[mesh.indices[i + 1]].position.y,
                             mesh.vertices[mesh.indices[i + 1]].position.z);
            tri.v2 = Vector3(mesh.vertices[mesh.indices[i + 2]].position.x,
                             mesh.vertices[mesh.indices[i + 2]].position.y,
                             mesh.vertices[mesh.indices[i + 2]].position.z);
            triangles_.push_back(tri);
        }
    }

    if (!triangles_.empty()) {
        size_t bufferSize = sizeof(Triangle) * triangles_.size();
        triangleResource_ = dxCommon_->CreateBufferResource(bufferSize);
        triangleResource_->Map(0, nullptr, reinterpret_cast<void **>(&triangleData_));
        std::memcpy(triangleData_, triangles_.data(), bufferSize);

        CreateTriangleSRV();

        if (emitterMeshData_) {
            emitterMeshData_->triangleCount = static_cast<uint32_t>(triangles_.size());
        }
    }
}

void ParticleCSEmitter::SaveSetting() {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleCS", name_);
    std::unique_ptr<DataHandler> groupData;

    data->Save("isAuto", isAuto_);
    data->Save("isVisible", isVisible_);
    data->Save("emitterType", static_cast<int>(currentEmitterType_));

    if (currentEmitterType_ == EmitterType::Sphere) {
        data->Save("frequency", emitterSphereData_->frequency);
        data->Save("frequencyTime", emitterSphereData_->frequencyTime);
        data->Save("radius", emitterSphereData_->radius);
        data->Save<Vector3>("translate", emitterSphereData_->translate);
        data->Save<Vector3>("rotation", emitterSphereData_->rotation);
        data->Save<Vector3>("scale", emitterSphereData_->scale);
    } else if (currentEmitterType_ == EmitterType::Mesh) {
        data->Save("frequency", emitterMeshData_->frequency);
        data->Save("frequencyTime", emitterMeshData_->frequencyTime);
        data->Save<Vector3>("translate", emitterMeshData_->translate);
        data->Save<Vector3>("rotation", emitterMeshData_->rotation);
        data->Save<Vector3>("scale", emitterMeshData_->scale);
        data->Save("modelPath", modelPath_);
        data->Save("primitiveType", static_cast<int>(primitiveType_));
    }

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
    currentEmitterType_ = static_cast<EmitterType>(data->Load("emitterType", static_cast<int>(EmitterType::Sphere)));

    if (currentEmitterType_ == EmitterType::Sphere) {
        CreateEmitterSphereResource();
        emitterSphereData_->frequency = data->Load("frequency", 0.1f);
        emitterSphereData_->frequencyTime = data->Load("frequencyTime", 0.0f);
        emitterSphereData_->radius = data->Load("radius", 1.0f);
        emitterSphereData_->translate = data->Load<Vector3>("translate", Vector3(0.0f, 0.0f, 0.0f));
        emitterSphereData_->rotation = data->Load<Vector3>("rotation", Vector3(0.0f, 0.0f, 0.0f));
        emitterSphereData_->scale = data->Load<Vector3>("scale", Vector3(1.0f, 1.0f, 1.0f));
    } else if (currentEmitterType_ == EmitterType::Mesh) {
        CreateEmitterMeshResource();
        emitterMeshData_->frequency = data->Load("frequency", 0.1f);
        emitterMeshData_->frequencyTime = data->Load("frequencyTime", 0.0f);
        emitterMeshData_->translate = data->Load<Vector3>("translate", Vector3(0.0f, 0.0f, 0.0f));
        emitterMeshData_->rotation = data->Load<Vector3>("rotation", Vector3(0.0f, 0.0f, 0.0f));
        emitterMeshData_->scale = data->Load<Vector3>("scale", Vector3(1.0f, 1.0f, 1.0f));

        modelPath_ = data->Load("modelPath", std::string(""));
        primitiveType_ = static_cast<PrimitiveType>(data->Load("primitiveType", static_cast<int>(PrimitiveType::None)));

        if (!modelPath_.empty()) {
            LoadModel(modelPath_);
        } else if (primitiveType_ != PrimitiveType::None) {
            LoadPrimitiveModel(primitiveType_);
        }

        CreateModelTriangles();
    }

    CreateEmitterSettingsResource();

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
    currentEmitterType_ = static_cast<EmitterType>(data->Load("emitterType", static_cast<int>(EmitterType::Sphere)));

    if (currentEmitterType_ == EmitterType::Sphere) {
        CreateEmitterSphereResource();
        emitterSphereData_->frequency = data->Load("frequency", 0.1f);
        emitterSphereData_->frequencyTime = data->Load("frequencyTime", 0.0f);
        emitterSphereData_->radius = data->Load("radius", 1.0f);
        emitterSphereData_->translate = data->Load<Vector3>("translate", Vector3(0.0f, 0.0f, 0.0f));
        emitterSphereData_->rotation = data->Load<Vector3>("rotation", Vector3(0.0f, 0.0f, 0.0f));
        emitterSphereData_->scale = data->Load<Vector3>("scale", Vector3(1.0f, 1.0f, 1.0f));
    } else if (currentEmitterType_ == EmitterType::Mesh) {
        CreateEmitterMeshResource();
        emitterMeshData_->frequency = data->Load("frequency", 0.1f);
        emitterMeshData_->frequencyTime = data->Load("frequencyTime", 0.0f);
        emitterMeshData_->translate = data->Load<Vector3>("translate", Vector3(0.0f, 0.0f, 0.0f));
        emitterMeshData_->rotation = data->Load<Vector3>("rotation", Vector3(0.0f, 0.0f, 0.0f));
        emitterMeshData_->scale = data->Load<Vector3>("scale", Vector3(1.0f, 1.0f, 1.0f));

        modelPath_ = data->Load("modelPath", std::string(""));
        primitiveType_ = static_cast<PrimitiveType>(data->Load("primitiveType", static_cast<int>(PrimitiveType::None)));

        if (!modelPath_.empty()) {
            LoadModel(modelPath_);
        } else if (primitiveType_ != PrimitiveType::None) {
            LoadPrimitiveModel(primitiveType_);
        }

        CreateModelTriangles();
    }

    CreateEmitterSettingsResource();
}

void ParticleCSEmitter::DrawImGui() {
    if (ImGui::BeginTabBar("EmitterTabBar")) {
        if (ImGui::BeginTabItem(name_.c_str())) {
            ImGuiStyle &style = ImGui::GetStyle();
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.15f, 1.00f));

            // エミッタータイプ表示・切り替えセクション
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.6f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.7f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.8f, 1.0f));

            if (ImGui::CollapsingHeader("エミッタータイプ設定##EmitterType")) {
                ImGui::PopStyleColor(3);

                ImGui::Text("現在のエミッタータイプ: %s",
                            (currentEmitterType_ == EmitterType::Sphere) ? "球体" : "メッシュ");

                // エミッタータイプ変更
                static int currentTypeSelection = (currentEmitterType_ == EmitterType::Sphere) ? 0 : 1;
                if (ImGui::RadioButton("球体エミッター", &currentTypeSelection, 0)) {
                    if (currentEmitterType_ != EmitterType::Sphere) {
                        SwitchEmitterType(EmitterType::Sphere);
                    }
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("メッシュエミッター", &currentTypeSelection, 1)) {
                    if (currentEmitterType_ != EmitterType::Mesh) {
                        SwitchEmitterType(EmitterType::Mesh);
                    }
                }

                if (currentEmitterType_ == EmitterType::Mesh) {
                    ImGui::Spacing();
                    ImGui::Text("モデル情報:");
                    if (!modelPath_.empty()) {
                        ImGui::Text("モデルファイル: %s", modelPath_.c_str());
                    } else if (primitiveType_ != PrimitiveType::None) {
                        const char *primitiveNames[] = {"なし", "プレーン", "球", "キューブ", "シリンダー", "リング", "三角形", "円錐", "四角錐"};
                        int typeIndex = static_cast<int>(primitiveType_);
                        if (typeIndex >= 0 && typeIndex < sizeof(primitiveNames) / sizeof(primitiveNames[0])) {
                            ImGui::Text("プリミティブタイプ: %s", primitiveNames[typeIndex]);
                        }
                    }
                    ImGui::Text("三角形数: %d", static_cast<int>(triangles_.size()));
                }
            } else {
                ImGui::PopStyleColor(3);
            }

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

                if (currentEmitterType_ == EmitterType::Sphere && emitterSphereData_) {
                    ImGui::DragFloat("発生間隔##SphereFreq", &emitterSphereData_->frequency, 0.001f, 0.001f, 10.0f);
                    ImGui::DragFloat3("エミッタの座標##SphereTranslate", &emitterSphereData_->translate.x, 0.1f);

                    // 度数法で表示・編集
                    Vector3 rotationDegrees = {
                        emitterSphereData_->rotation.x * 180.0f / 3.14159f,
                        emitterSphereData_->rotation.y * 180.0f / 3.14159f,
                        emitterSphereData_->rotation.z * 180.0f / 3.14159f};
                    if (ImGui::DragFloat3("エミッタの回転##SphereRotation", &rotationDegrees.x, 1.0f, -360.0f, 360.0f)) {
                        // ラジアンに変換して保存
                        emitterSphereData_->rotation.x = rotationDegrees.x * 3.14159f / 180.0f;
                        emitterSphereData_->rotation.y = rotationDegrees.y * 3.14159f / 180.0f;
                        emitterSphereData_->rotation.z = rotationDegrees.z * 3.14159f / 180.0f;
                    }

                    ImGui::DragFloat3("エミッタの半径##SphereRadius", &emitterSphereData_->radius.x, 0.1f);
                } else if (currentEmitterType_ == EmitterType::Mesh && emitterMeshData_) {
                    ImGui::DragFloat("発生間隔##MeshFreq", &emitterMeshData_->frequency, 0.001f, 0.001f, 10.0f);
                    ImGui::DragFloat3("エミッタの座標##MeshTranslate", &emitterMeshData_->translate.x, 0.1f);

                    // 度数法で表示・編集
                    Vector3 rotationDegrees = {
                        radiansToDegrees(emitterMeshData_->rotation.x),
                        radiansToDegrees(emitterMeshData_->rotation.y),
                        radiansToDegrees(emitterMeshData_->rotation.z)};
                    if (ImGui::DragFloat3("エミッタの回転##MeshRotation", &rotationDegrees.x, 1.0f, -360.0f, 360.0f)) {
                        // ラジアンに変換して保存
                        emitterMeshData_->rotation.x = degreesToRadians(rotationDegrees.x);
                        emitterMeshData_->rotation.y = degreesToRadians(rotationDegrees.y);
                        emitterMeshData_->rotation.z = degreesToRadians(rotationDegrees.z);
                    }

                    ImGui::DragFloat3("エミッタの大きさ##MeshScale", &emitterMeshData_->scale.x, 0.1f);
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
                        if (currentEmitterType_ == EmitterType::Sphere) {
                            particleGroups_[selectedGroupIndex]->SetFrequency(emitterSphereData_->frequency);
                        } else if (currentEmitterType_ == EmitterType::Mesh) {
                            particleGroups_[selectedGroupIndex]->SetFrequency(emitterMeshData_->frequency);
                        }
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

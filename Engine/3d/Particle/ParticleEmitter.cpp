#define NOMINMAX
#include "ParticleEmitter.h"
#include "Engine/Frame/Frame.h"
#include "line/DrawLine3D.h"

#include "ParticleGroupManager.h"
#include <set>
// コンストラクタ
ParticleEmitter::ParticleEmitter() {}

void ParticleEmitter::Initialize(std::string name) {
    transform_.Initialize();
    if (!name.empty()) {
        name_ = name;
        datas_ = std::make_unique<DataHandler>("Particle", name_);
        LoadFromJson();
        Manager_ = std::make_unique<ParticleManager>();
        Manager_->Initialize(SrvManager::GetInstance());
        LoadParticleGroup();
        datas_ = std::make_unique<DataHandler>("Particle", name_);
    }
}

// Update関数
void ParticleEmitter::Update() {
    // 経過時間を進める
    elapsedTime_ += Frame::DeltaTime();

    // 発生頻度に基づいてパーティクルを発生させる
    while (elapsedTime_ >= emitFrequency_) {
        Emit();                         // パーティクルを発生させる
        elapsedTime_ -= emitFrequency_; // 過剰に進んだ時間を考慮
    }
}

void ParticleEmitter::UpdateOnce() {
    isActive_ = false;
    if (!isActive_) {
        Emit(); // パーティクルを発生させる
        isActive_ = true;
    }
}

void ParticleEmitter::Draw(const ViewProjection &vp_) {
    if (isAuto_) {
        Update();
    }

    transform_.UpdateMatrix();
    if (Manager_) {
        Manager_->Update(vp_);
        Manager_->Draw();
    }
    DrawEmitter();
}

void ParticleEmitter::DrawEmitter() {
    if (!isVisible_)
        return;

    std::array<Vector3, 8> localVertices = {
        Vector3{-1.0f, -1.0f, -1.0f},
        Vector3{1.0f, -1.0f, -1.0f},
        Vector3{-1.0f, 1.0f, -1.0f},
        Vector3{1.0f, 1.0f, -1.0f},
        Vector3{-1.0f, -1.0f, 1.0f},
        Vector3{1.0f, -1.0f, 1.0f},
        Vector3{-1.0f, 1.0f, 1.0f},
        Vector3{1.0f, 1.0f, 1.0f}};
    std::array<Vector3, 8> worldVertices;
    Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale_, transform_.rotation_, transform_.translation_);
    for (size_t i = 0; i < localVertices.size(); i++) {
        worldVertices[i] = Transformation(localVertices[i], worldMatrix);
    }
    constexpr std::array<std::pair<int, int>, 12> edges = {
        std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0),
        std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4),
        std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)};
    for (const auto &edge : edges) {
        DrawLine3D::GetInstance()->SetPoints(worldVertices[edge.first], worldVertices[edge.second]);
    }
}

// Emit関数
void ParticleEmitter::Emit() {
    if (Manager_) {
        for (auto &[groupName, setting] : particleSettings_) {
            // ここでtransform_の値をParticleSettingに反映
            setting.translate = transform_.translation_;
            setting.rotation = transform_.rotation_;
            setting.scale = transform_.scale_;
            Manager_->SetParticleSetting(groupName, setting);
        }
        Manager_->Emit();
    }
}

#pragma region ImGui関連

void ParticleEmitter::SaveToJson() {
    datas_->Save("emitterTranslation", transform_.translation_);
    datas_->Save("emitterRotation", transform_.rotation_);
    datas_->Save("emitterScale", transform_.scale_);
    datas_->Save("GroupNames", particleGroupNames_);
    datas_->Save("emitFrequency", emitFrequency_);
    datas_->Save("isVisible", isVisible_);
    datas_->Save("isActive", isActive_);
    datas_->Save("isAuto", isAuto_);
    for (const auto &[groupName, setting] : particleSettings_) {
        datas_->Save(groupName + "_translate", setting.translate);
        datas_->Save(groupName + "_rotation", setting.rotation);
        datas_->Save(groupName + "_scale", setting.scale);
        datas_->Save(groupName + "_count", setting.count);
        datas_->Save(groupName + "_lifeTimeMin", setting.lifeTimeMin);
        datas_->Save(groupName + "_lifeTimeMax", setting.lifeTimeMax);
        datas_->Save(groupName + "_alphaMin", setting.alphaMin);
        datas_->Save(groupName + "_alphaMax", setting.alphaMax);
        datas_->Save(groupName + "_scaleMin", setting.scaleMin);
        datas_->Save(groupName + "_scaleMax", setting.scaleMax);
        datas_->Save(groupName + "_velocityMin", setting.velocityMin);
        datas_->Save(groupName + "_velocityMax", setting.velocityMax);
        datas_->Save(groupName + "_startScale", setting.particleStartScale);
        datas_->Save(groupName + "_endScale", setting.particleEndScale);
        datas_->Save(groupName + "_startAcce", setting.startAcce);
        datas_->Save(groupName + "_endAcce", setting.endAcce);
        datas_->Save(groupName + "_startRote", setting.startRote);
        datas_->Save(groupName + "_endRote", setting.endRote);
        datas_->Save(groupName + "_rotateStartMax", setting.rotateStartMax);
        datas_->Save(groupName + "_rotateStartMin", setting.rotateStartMin);
        datas_->Save(groupName + "_rotateVelocityMin", setting.rotateVelocityMin);
        datas_->Save(groupName + "_rotateVelocityMax", setting.rotateVelocityMax);
        datas_->Save(groupName + "_allScaleMin", setting.allScaleMin);
        datas_->Save(groupName + "_allScaleMax", setting.allScaleMax);
        datas_->Save(groupName + "_isRandomScale", setting.isRandomSize);
        datas_->Save(groupName + "_isAllRamdomScale", setting.isRandomAllSize);
        datas_->Save(groupName + "_isRandomColor", setting.isRandomColor);
        datas_->Save(groupName + "_isRandomRotate", setting.isRandomRotate);
        datas_->Save(groupName + "_isBillboard", setting.isBillboard);
        datas_->Save(groupName + "_isAcceMultiply", setting.isAcceMultiply);
        datas_->Save(groupName + "_isSinMove", setting.isSinMove);
        datas_->Save(groupName + "_isFaceDirection", setting.isFaceDirection);
        datas_->Save(groupName + "_isEndScale", setting.isEndScale);
        datas_->Save(groupName + "_isEmitOnEdge", setting.isEmitOnEdge);
        datas_->Save(groupName + "_isGatherMode", setting.isGatherMode);
        datas_->Save(groupName + "_gatherStartRatio", setting.gatherStartRatio);
        datas_->Save(groupName + "_gatherStrength", setting.gatherStrength);
        datas_->Save(groupName + "_gravity", setting.gravity);
        datas_->Save(groupName + "_isBillboard", setting.isBillboard);
        datas_->Save(groupName + "_enableTrail", setting.enableTrail);
        datas_->Save(groupName + "_trailSpawnInterval", setting.trailSpawnInterval);
        datas_->Save(groupName + "_maxTrailParticles", setting.maxTrailParticles);
        datas_->Save(groupName + "_trailLifeScale", setting.trailLifeScale);
        datas_->Save(groupName + "_trailScaleMultiplier", setting.trailScaleMultiplier);
        datas_->Save(groupName + "_trailColorMultiplier", setting.trailColorMultiplier);
        datas_->Save(groupName + "_trailInheritVelocity", setting.trailInheritVelocity);
        datas_->Save(groupName + "_trailVelocityScale", setting.trailVelocityScale);
        datas_->Save(groupName + "_startColor", setting.startColor);
        datas_->Save(groupName + "_endColor", setting.endColor);
        Manager_->SetParticleSetting(groupName, setting);
    }
}

void ParticleEmitter::LoadFromJson() {
    transform_.translation_ = datas_->Load<Vector3>("emitterTranslation", {0, 0, 0});
    transform_.rotation_ = datas_->Load<Vector3>("emitterRotation", {0, 0, 0});
    transform_.scale_ = datas_->Load<Vector3>("emitterScale", {1, 1, 1});
    particleGroupNames_ = datas_->Load<std::vector<std::string>>("GroupNames", {});
    emitFrequency_ = datas_->Load<float>("emitFrequency", 0.1f);
    isVisible_ = datas_->Load<bool>("isVisible", true);
    isActive_ = datas_->Load<bool>("isActive", false);
    isAuto_ = datas_->Load<bool>("isAuto", false);

    for (const auto &groupName : particleGroupNames_) {
        ParticleSetting setting;
        setting.translate = datas_->Load<Vector3>(groupName + "_translate", {0, 0, 0});
        setting.rotation = datas_->Load<Vector3>(groupName + "_rotation", {0, 0, 0});
        setting.scale = datas_->Load<Vector3>(groupName + "_scale", {1, 1, 1});
        setting.count = datas_->Load<uint32_t>(groupName + "_count", 1);
        setting.lifeTimeMin = datas_->Load<float>(groupName + "_lifeTimeMin", 1.0f);
        setting.lifeTimeMax = datas_->Load<float>(groupName + "_lifeTimeMax", 3.0f);
        setting.alphaMin = datas_->Load<float>(groupName + "_alphaMin", 1.0f);
        setting.alphaMax = datas_->Load<float>(groupName + "_alphaMax", 1.0f);
        setting.scaleMin = datas_->Load<float>(groupName + "_scaleMin", 1.0f);
        setting.scaleMax = datas_->Load<float>(groupName + "_scaleMax", 1.0f);
        setting.velocityMin = datas_->Load<Vector3>(groupName + "_velocityMin", {-1, -1, -1});
        setting.velocityMax = datas_->Load<Vector3>(groupName + "_velocityMax", {1, 1, 1});
        setting.particleStartScale = datas_->Load<Vector3>(groupName + "_startScale", {1, 1, 1});
        setting.particleEndScale = datas_->Load<Vector3>(groupName + "_endScale", {0, 0, 0});
        setting.startAcce = datas_->Load<Vector3>(groupName + "_startAcce", {1, 1, 1});
        setting.endAcce = datas_->Load<Vector3>(groupName + "_endAcce", {1, 1, 1});
        setting.startRote = datas_->Load<Vector3>(groupName + "_startRote", {0, 0, 0});
        setting.endRote = datas_->Load<Vector3>(groupName + "_endRote", {0, 0, 0});
        setting.rotateStartMax = datas_->Load<Vector3>(groupName + "_rotateStartMax", {0, 0, 0});
        setting.rotateStartMin = datas_->Load<Vector3>(groupName + "_rotateStartMin", {0, 0, 0});
        setting.rotateVelocityMin = datas_->Load<Vector3>(groupName + "_rotateVelocityMin", {-0.07f, -0.07f, -0.07f});
        setting.rotateVelocityMax = datas_->Load<Vector3>(groupName + "_rotateVelocityMax", {0.07f, 0.07f, 0.07f});
        setting.allScaleMin = datas_->Load<Vector3>(groupName + "_allScaleMin", {0, 0, 0});
        setting.allScaleMax = datas_->Load<Vector3>(groupName + "_allScaleMax", {1, 1, 1});
        setting.isRandomSize = datas_->Load<bool>(groupName + "_isRandomScale", false);
        setting.isRandomAllSize = datas_->Load<bool>(groupName + "_isAllRamdomScale", false);
        setting.isRandomColor = datas_->Load<bool>(groupName + "_isRandomColor", false);
        setting.isRandomRotate = datas_->Load<bool>(groupName + "_isRandomRotate", false);
        setting.isBillboard = datas_->Load<bool>(groupName + "_isBillboard", false);
        setting.isAcceMultiply = datas_->Load<bool>(groupName + "_isAcceMultiply", false);
        setting.isSinMove = datas_->Load<bool>(groupName + "_isSinMove", false);
        setting.isFaceDirection = datas_->Load<bool>(groupName + "_isFaceDirection", false);
        setting.isEndScale = datas_->Load<bool>(groupName + "_isEndScale", false);
        setting.isEmitOnEdge = datas_->Load<bool>(groupName + "_isEmitOnEdge", false);
        setting.isGatherMode = datas_->Load<bool>(groupName + "_isGatherMode", false);
        setting.gatherStartRatio = datas_->Load<float>(groupName + "_gatherStartRatio", 0.0f);
        setting.gatherStrength = datas_->Load<float>(groupName + "_gatherStrength", 0.0f);
        setting.gravity = datas_->Load<float>(groupName + "_gravity", 0.0f);
        setting.isBillboard = datas_->Load<float>(groupName + "_isBillboard", false);
        setting.enableTrail = datas_->Load<bool>(groupName + "_enableTrail", false);
        setting.trailSpawnInterval = datas_->Load<float>(groupName + "_trailSpawnInterval", 0.05f);
        setting.maxTrailParticles = datas_->Load<int>(groupName + "_maxTrailParticles", 20);
        setting.trailLifeScale = datas_->Load<float>(groupName + "_trailLifeScale", 0.5f);
        setting.trailScaleMultiplier = datas_->Load<Vector3>(groupName + "_trailScaleMultiplier", {0.8f, 0.8f, 0.8f});
        setting.trailColorMultiplier = datas_->Load<Vector4>(groupName + "_trailColorMultiplier", {1.0f, 1.0f, 1.0f, 0.7f});
        setting.trailInheritVelocity = datas_->Load<bool>(groupName + "_trailInheritVelocity", true);
        setting.trailVelocityScale = datas_->Load<float>(groupName + "_trailVelocityScale", 0.3f);
        setting.startColor = datas_->Load<Vector4>(groupName + "_startColor", {1.0f, 1.0f, 1.0f, 1.0f});
        setting.endColor = datas_->Load<Vector4>(groupName + "_endColor", {1.0f, 1.0f, 1.0f, 1.0f});

        particleSettings_[groupName] = setting;
    }
}

void ParticleEmitter::LoadParticleGroup() {
    for (auto &particleGroupname : particleGroupNames_) {
        AddParticleGroup(ParticleGroupManager::GetInstance()->GetParticleGroup(particleGroupname));
    }
}

ParticleSetting ParticleEmitter::DefaultSetting() {
    ParticleSetting setting;
    setting.translate = {0, 0, 0};
    setting.rotation = {0, 0, 0};
    setting.scale = {1, 1, 1};
    setting.count = 1;
    setting.lifeTimeMin = 1.0f;
    setting.lifeTimeMax = 3.0f;
    setting.alphaMin = 1.0f;
    setting.alphaMax = 1.0f;
    setting.scaleMin = 1.0f;
    setting.scaleMax = 1.0f;
    setting.velocityMin = {-1, -1, -1};
    setting.velocityMax = {1, 1, 1};
    setting.particleStartScale = {1, 1, 1};
    setting.particleEndScale = {0, 0, 0};
    setting.startAcce = {1, 1, 1};
    setting.endAcce = {1, 1, 1};
    setting.startRote = {0, 0, 0};
    setting.endRote = {0, 0, 0};
    setting.rotateStartMax = {0, 0, 0};
    setting.rotateStartMin = {0, 0, 0};
    setting.rotateVelocityMin = {-0.07f, -0.07f, -0.07f};
    setting.rotateVelocityMax = {0.07f, 0.07f, 0.07f};
    setting.allScaleMin = {0, 0, 0};
    setting.allScaleMax = {1, 1, 1};
    setting.isRandomSize = false;
    setting.isRandomAllSize = false;
    setting.isRandomColor = false;
    setting.isRandomRotate = false;
    setting.isBillboard = false;
    setting.isAcceMultiply = false;
    setting.isSinMove = false;
    setting.isFaceDirection = false;
    setting.isEndScale = false;
    setting.isEmitOnEdge = false;
    setting.isGatherMode = false;
    setting.gatherStartRatio = 0.0f;
    setting.gatherStrength = 0.0f;
    setting.gravity = 0.0f;
    setting.enableTrail = false;
    setting.trailSpawnInterval = 0.05f;
    setting.maxTrailParticles = 20;
    setting.trailLifeScale = 0.5f;
    setting.trailScaleMultiplier = {0.8f, 0.8f, 0.8f};
    setting.trailColorMultiplier = {1.0f, 1.0f, 1.0f, 0.7f};
    setting.trailInheritVelocity = true;
    setting.trailVelocityScale = 0.3f;
    setting.startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    setting.endColor = {1.0f, 1.0f, 1.0f, 1.0f};
    return setting;
}

void ParticleEmitter::DebugParticleData() {

    if (!Manager_)
        return;
    std::vector<std::string> groupNames = Manager_->GetParticleGroupsName();
    if (selectedGroupIndex_ >= groupNames.size()) {
        selectedGroupIndex_ = std::max(0, (int)groupNames.size() - 1);
    }

    if (ImGui::BeginTabBar("パーティクル")) {
        if (ImGui::BeginTabItem(name_.c_str())) {
            if (!groupNames.empty()) {
                std::vector<const char *> groupNameCStrs;
                for (auto &n : groupNames)
                    groupNameCStrs.push_back(n.c_str());
                ImGui::Combo("編集グループ", &selectedGroupIndex_, groupNameCStrs.data(), (int)groupNameCStrs.size());
                std::string selectedGroup = groupNames[selectedGroupIndex_];
                ParticleSetting &setting = particleSettings_[selectedGroup];

                // 基本データセクション
                if (ImGui::CollapsingHeader("エミッターデータ")) {
                    // トランスフォームデータをフレーム内に配置
                    ImGui::Text("Transformデータ:");
                    ImGui::Separator();
                    ImGui::Columns(2, "TransformColumns", false); // 2列レイアウト
                    ImGui::Text("位置");
                    ImGui::NextColumn();
                    ImGui::DragFloat3("##位置", &transform_.translation_.x, 0.1f);
                    ImGui::NextColumn();
                    ImGui::Text("回転");
                    ImGui::NextColumn();
                    float rotationDegrees[3] = {
                        radiansToDegrees(transform_.rotation_.x),
                        radiansToDegrees(transform_.rotation_.y),
                        radiansToDegrees(transform_.rotation_.z)};

                    // ドラッグUIを使用し、度数法で値を操作
                    if (ImGui::DragFloat3("##回転 (度)", rotationDegrees, 0.1f, -360.0f, 360.0f)) {
                        // 操作後、度数法からラジアンに戻して保存
                        transform_.rotation_.x = degreesToRadians(rotationDegrees[0]);
                        transform_.rotation_.y = degreesToRadians(rotationDegrees[1]);
                        transform_.rotation_.z = degreesToRadians(rotationDegrees[2]);
                    }

                    ImGui::NextColumn();
                    ImGui::Text("大きさ");
                    ImGui::NextColumn();
                    ImGui::DragFloat3("##大きさ", &transform_.scale_.x, 0.1f, 0.0f);
                    ImGui::Columns(1); // 列終了
                    ImGui::Separator();

                    // 可視性フラグ
                    ImGui::Checkbox("表示", &isVisible_);
                }

                // パーティクルデータセクション
                if (ImGui::CollapsingHeader("パーティクルデータ")) {
                    // LifeTimeを折りたたみ可能にする
                    if (ImGui::TreeNode("寿命")) {
                        ImGui::Text("寿命設定:");
                        ImGui::Separator();
                        ImGui::DragFloat("最大値", &setting.lifeTimeMax, 0.1f, 0.0f);
                        ImGui::DragFloat("最小値", &setting.lifeTimeMin, 0.1f, 0.0f);
                        setting.lifeTimeMin = std::clamp(setting.lifeTimeMin, 0.0f, setting.lifeTimeMax);
                        setting.lifeTimeMax = std::clamp(setting.lifeTimeMax, setting.lifeTimeMin, 10.0f);
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    if (ImGui::TreeNode("位置")) {
                        ImGui::Checkbox("中心に集める", &setting.isGatherMode);
                        if (setting.isGatherMode) {
                            ImGui::DragFloat("強さ", &setting.gatherStrength, 0.1f);
                            ImGui::DragFloat("始まるタイミング", &setting.gatherStartRatio, 0.1f);
                        }
                        ImGui::Checkbox("外周", &setting.isEmitOnEdge);
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    // 速度と加速度
                    if (ImGui::TreeNode("速度、加速度")) {
                        ImGui::Text("速度:");
                        ImGui::DragFloat3("最大値", &setting.velocityMax.x, 0.1f);
                        ImGui::DragFloat3("最小値", &setting.velocityMin.x, 0.1f);
                        setting.velocityMin.x = std::clamp(setting.velocityMin.x, -FLT_MAX, setting.velocityMax.x);
                        setting.velocityMax.x = std::clamp(setting.velocityMax.x, setting.velocityMin.x, FLT_MAX);
                        setting.velocityMin.y = std::clamp(setting.velocityMin.y, -FLT_MAX, setting.velocityMax.y);
                        setting.velocityMax.y = std::clamp(setting.velocityMax.y, setting.velocityMin.y, FLT_MAX);
                        setting.velocityMin.z = std::clamp(setting.velocityMin.z, -FLT_MAX, setting.velocityMax.z);
                        setting.velocityMax.z = std::clamp(setting.velocityMax.z, setting.velocityMin.z, FLT_MAX);
                        ImGui::Text("加速度:");
                        ImGui::DragFloat3("最初", &setting.startAcce.x, 0.001f);
                        ImGui::DragFloat3("最後", &setting.endAcce.x, 0.001f);
                        ImGui::Checkbox("乗算", &setting.isAcceMultiply);
                        ImGui::DragFloat("重力", &setting.gravity, 0.01f, -FLT_MAX, FLT_MAX);
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    // サイズ
                    if (ImGui::TreeNode("大きさ")) {
                        ImGui::Text("大きさ:");
                        if (setting.isRandomAllSize) {
                            ImGui::Checkbox("最初と最後同じ大きさ", &setting.isEndScale);
                            ImGui::DragFloat3("最大値", &setting.allScaleMax.x, 0.1f, 0.0f);
                            ImGui::DragFloat3("最小値", &setting.allScaleMin.x, 0.1f, 0.0f);
                            setting.allScaleMin.x = std::clamp(setting.allScaleMin.x, -FLT_MAX, setting.allScaleMax.x);
                            setting.allScaleMax.x = std::clamp(setting.allScaleMax.x, setting.allScaleMin.x, FLT_MAX);
                            setting.allScaleMin.y = std::clamp(setting.allScaleMin.y, -FLT_MAX, setting.allScaleMax.y);
                            setting.allScaleMax.y = std::clamp(setting.allScaleMax.y, setting.allScaleMin.y, FLT_MAX);
                            setting.allScaleMin.z = std::clamp(setting.allScaleMin.z, -FLT_MAX, setting.allScaleMax.z);
                            setting.allScaleMax.z = std::clamp(setting.allScaleMax.z, setting.allScaleMin.z, FLT_MAX);
                            if (!setting.isEndScale) {
                                ImGui::DragFloat3("最後", &setting.particleEndScale.x, 0.1f);
                            }
                        } else if (setting.isRandomSize) {
                            ImGui::DragFloat("最大値", &setting.scaleMax, 0.1f, 0.0f);
                            ImGui::DragFloat("最小値", &setting.scaleMin, 0.1f, 0.0f);
                            setting.scaleMax = std::clamp(setting.scaleMax, setting.scaleMin, FLT_MAX);
                            setting.scaleMin = std::clamp(setting.scaleMin, 0.0f, setting.scaleMax);
                        } else if (setting.isSinMove) {
                            ImGui::DragFloat3("最初", &setting.particleStartScale.x, 0.1f, 0.0f);
                        } else {
                            ImGui::DragFloat3("最初", &setting.particleStartScale.x, 0.1f, 0.0f);
                            ImGui::DragFloat3("最後", &setting.particleEndScale.x, 0.1f);
                        }
                        ImGui::Checkbox("均等にランダムな大きさ", &setting.isRandomSize);
                        ImGui::Checkbox("ばらばらにランダムな大きさ", &setting.isRandomAllSize);
                        ImGui::Checkbox("sin波の動き", &setting.isSinMove);
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    // 回転
                    if (ImGui::TreeNode("回転")) {
                        if (!setting.isRandomRotate) {
                            float startRotationDegrees[3] = {
                                radiansToDegrees(setting.startRote.x),
                                radiansToDegrees(setting.startRote.y),
                                radiansToDegrees(setting.startRote.z)};
                            float endRotationDegrees[3] = {
                                radiansToDegrees(setting.endRote.x),
                                radiansToDegrees(setting.endRote.y),
                                radiansToDegrees(setting.endRote.z)};
                            if (ImGui::DragFloat3("最初", startRotationDegrees, 0.1f)) {
                                setting.startRote.x = degreesToRadians(startRotationDegrees[0]);
                                setting.startRote.y = degreesToRadians(startRotationDegrees[1]);
                                setting.startRote.z = degreesToRadians(startRotationDegrees[2]);
                            }
                            if (ImGui::DragFloat3("最後", endRotationDegrees, 0.1f)) {
                                setting.endRote.x = degreesToRadians(endRotationDegrees[0]);
                                setting.endRote.y = degreesToRadians(endRotationDegrees[1]);
                                setting.endRote.z = degreesToRadians(endRotationDegrees[2]);
                            }
                        }
                        if (setting.isRandomRotate) {
                            float startRotationDegrees[3] = {
                                radiansToDegrees(setting.rotateStartMax.x),
                                radiansToDegrees(setting.rotateStartMax.y),
                                radiansToDegrees(setting.rotateStartMax.z)};
                            float endRotationDegrees[3] = {
                                radiansToDegrees(setting.rotateStartMin.x),
                                radiansToDegrees(setting.rotateStartMin.y),
                                radiansToDegrees(setting.rotateStartMin.z)};

                            if (ImGui::DragFloat3("回転 最大値", startRotationDegrees, 0.1f)) {
                                setting.rotateStartMax.x = degreesToRadians(std::clamp(startRotationDegrees[0], radiansToDegrees(setting.rotateStartMin.x), 180.0f));
                                setting.rotateStartMax.y = degreesToRadians(std::clamp(startRotationDegrees[1], radiansToDegrees(setting.rotateStartMin.y), 180.0f));
                                setting.rotateStartMax.z = degreesToRadians(std::clamp(startRotationDegrees[2], radiansToDegrees(setting.rotateStartMin.z), 180.0f));
                            }

                            if (ImGui::DragFloat3("回転 最小値", endRotationDegrees, 0.1f)) {
                                setting.rotateStartMin.x = degreesToRadians(std::clamp(endRotationDegrees[0], -180.0f, radiansToDegrees(setting.rotateStartMax.x)));
                                setting.rotateStartMin.y = degreesToRadians(std::clamp(endRotationDegrees[1], -180.0f, radiansToDegrees(setting.rotateStartMax.y)));
                                setting.rotateStartMin.z = degreesToRadians(std::clamp(endRotationDegrees[2], -180.0f, radiansToDegrees(setting.rotateStartMax.z)));
                            }

                            ImGui::Checkbox("ランダムな回転速度", &setting.isRotateVelocity);

                            if (setting.isRotateVelocity) {
                                ImGui::DragFloat3("最大値", &setting.rotateVelocityMax.x, 0.01f);
                                ImGui::DragFloat3("最小値", &setting.rotateVelocityMin.x, 0.01f);
                                setting.rotateVelocityMin.x = std::clamp(setting.rotateVelocityMin.x, -FLT_MAX, setting.rotateVelocityMax.x);
                                setting.rotateVelocityMax.x = std::clamp(setting.rotateVelocityMax.x, setting.rotateVelocityMin.x, FLT_MAX);
                                setting.rotateVelocityMin.y = std::clamp(setting.rotateVelocityMin.y, -FLT_MAX, setting.rotateVelocityMax.y);
                                setting.rotateVelocityMax.y = std::clamp(setting.rotateVelocityMax.y, setting.rotateVelocityMin.y, FLT_MAX);
                                setting.rotateVelocityMin.z = std::clamp(setting.rotateVelocityMin.z, -FLT_MAX, setting.rotateVelocityMax.z);
                                setting.rotateVelocityMax.z = std::clamp(setting.rotateVelocityMax.z, setting.rotateVelocityMin.z, FLT_MAX);
                            }
                        }
                        ImGui::Checkbox("ランダムな回転", &setting.isRandomRotate);
                        ImGui::Checkbox("進行方向に向ける", &setting.isFaceDirection);
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    if (ImGui::TreeNode("トレイル設定")) {
                        if (ImGui::Checkbox("トレイルを有効にする", &setting.enableTrail)) {
                            SetTrailEnabled(selectedGroup, setting.enableTrail);
                        }
                        if (setting.enableTrail) {
                            if (ImGui::DragFloat("トレイル生成間隔", &setting.trailSpawnInterval, 0.001f, 0.001f, 1.0f)) {
                                SetTrailInterval(selectedGroup, setting.trailSpawnInterval);
                            }
                            if (ImGui::DragInt("最大トレイルパーティクル数", &setting.maxTrailParticles, 1, 1, 100)) {
                                SetMaxTrailParticles(selectedGroup, setting.maxTrailParticles);
                            }
                            if (ImGui::DragFloat("トレイル生存時間スケール", &setting.trailLifeScale, 0.01f, 0.1f, 2.0f)) {
                                SetTrailLifeScale(selectedGroup, setting.trailLifeScale);
                            }
                            if (ImGui::DragFloat3("トレイルスケール倍率", &setting.trailScaleMultiplier.x, 0.01f, 0.1f, 2.0f)) {
                                SetTrailScaleMultiplier(selectedGroup, setting.trailScaleMultiplier);
                            }
                            if (ImGui::DragFloat4("トレイル色彩倍率", &setting.trailColorMultiplier.x, 0.01f, 0.0f, 2.0f)) {
                                SetTrailColorMultiplier(selectedGroup, setting.trailColorMultiplier);
                            }
                            if (ImGui::Checkbox("トレイル速度継承", &setting.trailInheritVelocity)) {
                                SetTrailVelocityInheritance(selectedGroup, setting.trailInheritVelocity, setting.trailVelocityScale);
                            }
                            if (setting.trailInheritVelocity) {
                                if (ImGui::DragFloat("トレイル速度スケール", &setting.trailVelocityScale, 0.01f, 0.0f, 2.0f)) {
                                    SetTrailVelocityInheritance(selectedGroup, setting.trailInheritVelocity, setting.trailVelocityScale);
                                }
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    if (ImGui::TreeNode("色彩")) {

                         if (ImGui::TreeNode("色")) {
                            ImGui::ColorEdit4("開始色", &setting.startColor.x);
                            ImGui::ColorEdit4("終了色", &setting.endColor.x);
                            ImGui::TreePop();
                        }

                        // Alphaを折りたたみ可能にする
                        if (ImGui::TreeNode("透明度")) {
                            ImGui::Text("透明度の設定:");
                            ImGui::DragFloat("最大値", &setting.alphaMax, 0.01f, 0.0f, 1.0f);
                            ImGui::DragFloat("最小値", &setting.alphaMin, 0.01f, 0.0f, 1.0f);
                            setting.alphaMin = std::clamp(setting.alphaMin, 0.0f, setting.alphaMax);
                            setting.alphaMax = std::clamp(setting.alphaMax, setting.alphaMin, 1.0f);
                            ImGui::TreePop();
                        }
                        ImGui::TreePop();
                    }
                }

                // エミット設定セクション
                if (ImGui::CollapsingHeader("パーティクルの数、間隔")) {
                    ImGui::DragFloat("間隔", &emitFrequency_, 0.001f, 0.001f, 100.0f);
                    ImGui::InputInt("数", reinterpret_cast<int *>(&setting.count), 1, 100);
                    setting.count = std::clamp(static_cast<int>(setting.count), 0, 10000);
                }

                // その他の設定セクション
                if (ImGui::CollapsingHeader("各状態の設定")) {
                    ImGui::Checkbox("ビルボード", &setting.isBillboard);
                    ImGui::Checkbox("ランダムカラー", &setting.isRandomColor);
                }
            } else {
                ImGui::Text("グループがありません。");
            }
            if (ImGui::CollapsingHeader("グループ")) {
                // エミッターにアタッチされているグループ名をセットとして扱う（高速検索のため）
                std::set<std::string> emitterGroupNames(
                    particleGroupNames_.begin(),
                    particleGroupNames_.end());

                // 全パーティクルグループを取得
                std::vector<ParticleGroup *> allGroups = ParticleGroupManager::GetInstance()->GetParticleGroups();

                // 選択状態を保持するインデックス
                static std::vector<int> leftSelected;
                static std::vector<int> rightSelected;

                // グループ分類用のリスト
                std::vector<std::string> availableNames;
                std::vector<const char *> availableItems;
                std::vector<std::string> attachedNames;
                std::vector<const char *> attachedItems;

                // グループを分類する処理
                for (const auto &group : allGroups) {
                    const std::string &name = group->GetGroupName();
                    if (emitterGroupNames.contains(name)) {
                        attachedNames.push_back(name);
                    } else {
                        availableNames.push_back(name);
                    }
                }

                // c_str() 変換は最後に行う
                availableItems.clear();
                attachedItems.clear();
                for (auto &name : availableNames) {
                    availableItems.push_back(name.c_str());
                }
                for (auto &name : attachedNames) {
                    attachedItems.push_back(name.c_str());
                }

                // 範囲外インデックスを除外
                while (!leftSelected.empty() && leftSelected.back() >= availableNames.size())
                    leftSelected.pop_back();
                while (!rightSelected.empty() && rightSelected.back() >= attachedNames.size())
                    rightSelected.pop_back();

                // UI横幅の取得
                float width = ImGui::GetContentRegionAvail().x;
                float halfWidth = width * 0.45f;

                ImGui::Text("利用可能");
                ImGui::SameLine(width - halfWidth - 50);
                ImGui::Text("アタッチ済み");

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));

                // 左リスト（未アタッチグループ）
                ImGui::BeginChild("available_groups", ImVec2(halfWidth, 200), true);
                for (int i = 0; i < availableItems.size(); i++) {
                    bool isSelected = std::find(leftSelected.begin(), leftSelected.end(), i) != leftSelected.end();
                    if (ImGui::Selectable(availableItems[i], isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (!ImGui::GetIO().KeyCtrl)
                            leftSelected.clear();

                        auto it = std::find(leftSelected.begin(), leftSelected.end(), i);
                        if (it != leftSelected.end())
                            leftSelected.erase(it);
                        else
                            leftSelected.push_back(i);

                        // ダブルクリックで追加
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            ParticleGroup *group = ParticleGroupManager::GetInstance()->GetParticleGroup(availableNames[i]);
                            if (group) {
                                AddParticleGroup(group);
                            }
                            leftSelected.clear();
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                // 中央の追加・削除ボタン
                ImGui::BeginGroup();
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));

                ImGui::PushID("move_right");
                if (ImGui::Button(">>", ImVec2(40, 30)) && !leftSelected.empty()) {
                    for (auto it = leftSelected.rbegin(); it != leftSelected.rend(); ++it) {
                        int idx = *it;
                        ParticleGroup *group = ParticleGroupManager::GetInstance()->GetParticleGroup(availableNames[idx]);
                        if (group) {
                            AddParticleGroup(group);
                            particleGroupNames_ = Manager_->GetParticleGroupsName();
                        }
                    }
                    leftSelected.clear();
                }
                ImGui::PopID();

                ImGui::PushID("move_left");
                if (ImGui::Button("<<", ImVec2(40, 30)) && !rightSelected.empty()) {
                    for (auto it = rightSelected.rbegin(); it != rightSelected.rend(); ++it) {
                        int idx = *it;
                        RemoveParticleGroup(attachedNames[idx]);
                        particleGroupNames_ = Manager_->GetParticleGroupsName();
                    }
                    rightSelected.clear();
                }
                ImGui::PopID();

                ImGui::PopStyleVar(); // ボタン間の余白復元
                ImGui::EndGroup();

                ImGui::SameLine();

                // 右リスト（アタッチ済みグループ）
                ImGui::BeginChild("attached_groups", ImVec2(halfWidth, 200), true);
                for (int i = 0; i < attachedItems.size(); i++) {
                    bool isSelected = std::find(rightSelected.begin(), rightSelected.end(), i) != rightSelected.end();
                    if (ImGui::Selectable(attachedItems[i], isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (!ImGui::GetIO().KeyCtrl)
                            rightSelected.clear();

                        auto it = std::find(rightSelected.begin(), rightSelected.end(), i);
                        if (it != rightSelected.end())
                            rightSelected.erase(it);
                        else
                            rightSelected.push_back(i);

                        // ダブルクリックで削除
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            RemoveParticleGroup(attachedNames[i]);
                            rightSelected.clear();
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::PopStyleVar(); // ItemSpacing 戻す
            }

            if (ImGui::Button("セーブ")) {
                SaveToJson();
                std::string message = std::format("ParticleData saved.");
                MessageBoxA(nullptr, message.c_str(), "Particle", 0);
            }

            ImGui::Checkbox("自動生成", &isAuto_);

            if (ImGui::Button("生成")) {
                UpdateOnce();
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    };
}

// ImGuiで値を動かす関数
void ParticleEmitter::Debug() {
#ifdef _DEBUG
    if (!name_.empty() && Manager_) {
        DebugParticleData();
    }
#endif
}
void ParticleEmitter::AddParticleGroup(ParticleGroup *particleGroup) {
    if (!particleGroup)
        return;

    // パーティクルグループ名を取得
    const std::string &groupName = particleGroup->GetGroupName();

    // 設定が存在しない場合はデフォルト値で初期化
    auto it = particleSettings_.find(groupName);
    if (it == particleSettings_.end()) {
        particleSettings_[groupName] = DefaultSetting();
    }

    Manager_->AddParticleGroup(particleGroup);
}
#pragma endregion

void ParticleEmitter::SetTrailEnabled(const std::string &groupName, bool enabled) {
    if (particleSettings_.find(groupName) != particleSettings_.end()) {
        particleSettings_[groupName].enableTrail = enabled;
        if (Manager_) {
            Manager_->SetTrailEnabled(groupName, enabled);
        }
    }
}

void ParticleEmitter::SetTrailInterval(const std::string &groupName, float interval) {
    if (particleSettings_.find(groupName) != particleSettings_.end()) {
        particleSettings_[groupName].trailSpawnInterval = interval;
    }
}

void ParticleEmitter::SetMaxTrailParticles(const std::string &groupName, int maxTrails) {
    if (particleSettings_.find(groupName) != particleSettings_.end()) {
        particleSettings_[groupName].maxTrailParticles = maxTrails;
        if (Manager_) {
            Manager_->SetTrailSettings(groupName,
                                       particleSettings_[groupName].trailSpawnInterval, maxTrails);
        }
    }
}

void ParticleEmitter::SetTrailLifeScale(const std::string &groupName, float scale) {
    if (particleSettings_.find(groupName) != particleSettings_.end()) {
        particleSettings_[groupName].trailLifeScale = scale;
    }
}

void ParticleEmitter::SetTrailScaleMultiplier(const std::string &groupName, const Vector3 &multiplier) {
    if (particleSettings_.find(groupName) != particleSettings_.end()) {
        particleSettings_[groupName].trailScaleMultiplier = multiplier;
    }
}

void ParticleEmitter::SetTrailColorMultiplier(const std::string &groupName, const Vector4 &multiplier) {
    if (particleSettings_.find(groupName) != particleSettings_.end()) {
        particleSettings_[groupName].trailColorMultiplier = multiplier;
    }
}

void ParticleEmitter::SetTrailVelocityInheritance(const std::string &groupName, bool inherit, float scale) {
    if (particleSettings_.find(groupName) != particleSettings_.end()) {
        particleSettings_[groupName].trailInheritVelocity = inherit;
        particleSettings_[groupName].trailVelocityScale = scale;
    }
}

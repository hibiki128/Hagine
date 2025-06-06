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
        datas_ = std::make_unique<DataHandler>("Particle", name);
        LoadFromJson();
        Manager_ = std::make_unique<ParticleManager>();
        Manager_->Initialize(SrvManager::GetInstance());
        LoadParticleGroup();
        datas_ = std::make_unique<DataHandler>("Particle", name);
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
    Manager_->SetEmitterCenter(transform_.translation_);

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
        datas_->Save(groupName + "_isBillboardX", setting.isBillboardX);
        datas_->Save(groupName + "_isBillboardY", setting.isBillboardY);
        datas_->Save(groupName + "_isBillboardZ", setting.isBillboardZ);
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
        setting.isBillboardX = datas_->Load<bool>(groupName + "_isBillboardX", false);
        setting.isBillboardY = datas_->Load<bool>(groupName + "_isBillboardY", false);
        setting.isBillboardZ = datas_->Load<bool>(groupName + "_isBillboardZ", false);
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

#pragma region ImGui関連

void ParticleEmitter::DebugParticleData() {
    if (!Manager_)
        return;

    // カスタムスタイル設定
    ImGuiStyle &style = ImGui::GetStyle();

    // メインウィンドウの背景色
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.15f, 1.00f));

    std::vector<std::string> groupNames = Manager_->GetParticleGroupsName();
    if (selectedGroupIndex_ >= groupNames.size()) {
        selectedGroupIndex_ = std::max(0, (int)groupNames.size() - 1);
    }

    if (!groupNames.empty()) {
        // グループ選択部分
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.3f, 0.4f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.7f, 0.9f));

        std::vector<const char *> groupNameCStrs;
        for (auto &n : groupNames)
            groupNameCStrs.push_back(n.c_str());

        ImGui::SetNextItemWidth(200.0f);
        ImGui::Combo("編集グループ", &selectedGroupIndex_, groupNameCStrs.data(), (int)groupNameCStrs.size());

        ImGui::PopStyleColor(3);

        std::string selectedGroup = groupNames[selectedGroupIndex_];
        ParticleSetting &setting = particleSettings_[selectedGroup];

        ImGui::Separator();
        ImGui::Spacing();

        // エミッターデータセクション
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.2f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.3f, 0.3f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.6f, 0.4f, 0.4f, 1.0f));

        if (ImGui::CollapsingHeader("エミッターデータ")) {
            ImGui::PopStyleColor(3);

            // トランスフォームデータ
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.6f, 1.0f));
            ImGui::Text("Transformデータ:");
            ImGui::PopStyleColor();

            ImGui::Separator();
            ImGui::Columns(2, "TransformColumns", false);

            // 位置
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
            ImGui::Text("位置");
            ImGui::PopStyleColor();
            ImGui::NextColumn();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.2f, 0.5f));
            ImGui::DragFloat3("##位置", &transform_.translation_.x, 0.1f);
            ImGui::PopStyleColor();
            ImGui::NextColumn();

            // 回転
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
            ImGui::Text("回転");
            ImGui::PopStyleColor();
            ImGui::NextColumn();
            float rotationDegrees[3] = {
                radiansToDegrees(transform_.rotation_.x),
                radiansToDegrees(transform_.rotation_.y),
                radiansToDegrees(transform_.rotation_.z)};

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.3f, 0.4f, 0.5f));
            if (ImGui::DragFloat3("##回転 (度)", rotationDegrees, 0.1f, -360.0f, 360.0f)) {
                transform_.rotation_.x = degreesToRadians(rotationDegrees[0]);
                transform_.rotation_.y = degreesToRadians(rotationDegrees[1]);
                transform_.rotation_.z = degreesToRadians(rotationDegrees[2]);
            }
            ImGui::PopStyleColor();
            ImGui::NextColumn();

            // 大きさ
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
            ImGui::Text("大きさ");
            ImGui::PopStyleColor();
            ImGui::NextColumn();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.3f, 0.2f, 0.5f));
            ImGui::DragFloat3("##大きさ", &transform_.scale_.x, 0.1f, 0.0f);
            ImGui::PopStyleColor();

            ImGui::Columns(1);
            ImGui::Separator();

            // 可視性フラグ
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            ImGui::Checkbox("表示", &isVisible_);
            ImGui::PopStyleColor();
        } else {
            ImGui::PopStyleColor(3);
        }

        ImGui::Spacing();

        // パーティクルデータセクション
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.3f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.6f, 0.4f, 1.0f));

        if (ImGui::CollapsingHeader("パーティクルデータ")) {
            ImGui::PopStyleColor(3);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.8f, 1.0f));
            if (ImGui::TreeNode("寿命")) {
                ImGui::PopStyleColor();

                ImGui::Text("寿命設定:");
                ImGui::Separator();
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.2f, 0.4f));
                ImGui::DragFloat("最大値", &setting.lifeTimeMax, 0.1f, 0.0f);
                ImGui::DragFloat("最小値", &setting.lifeTimeMin, 0.1f, 0.0f);
                ImGui::PopStyleColor();

                // 正しい順序でクランプする
                setting.lifeTimeMax = std::clamp(setting.lifeTimeMax, setting.lifeTimeMin, 10.0f);
                setting.lifeTimeMin = std::clamp(setting.lifeTimeMin, 0.0f, setting.lifeTimeMax);

                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            // 位置設定
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 1.0f, 0.8f, 1.0f));
            if (ImGui::TreeNode("位置")) {
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                ImGui::Checkbox("中心に集める", &setting.isGatherMode);
                ImGui::PopStyleColor();

                if (setting.isGatherMode) {
                    ImGui::Indent();
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.2f, 0.4f));
                    ImGui::DragFloat("強さ", &setting.gatherStrength, 0.1f);
                    ImGui::DragFloat("始まるタイミング", &setting.gatherStartRatio, 0.1f);
                    ImGui::PopStyleColor();
                    ImGui::Unindent();
                }

                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                ImGui::Checkbox("外周", &setting.isEmitOnEdge);
                ImGui::PopStyleColor();
                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            // 速度と加速度
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.0f, 1.0f));
            if (ImGui::TreeNode("速度、加速度")) {
                ImGui::PopStyleColor();

                ImGui::Text("速度:");
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.4f, 0.4f));
                ImGui::DragFloat3("最大値", &setting.velocityMax.x, 0.1f);
                ImGui::DragFloat3("最小値", &setting.velocityMin.x, 0.1f);
                ImGui::PopStyleColor();

                setting.velocityMin.x = std::clamp(setting.velocityMin.x, -FLT_MAX, setting.velocityMax.x);
                setting.velocityMax.x = std::clamp(setting.velocityMax.x, setting.velocityMin.x, FLT_MAX);
                setting.velocityMin.y = std::clamp(setting.velocityMin.y, -FLT_MAX, setting.velocityMax.y);
                setting.velocityMax.y = std::clamp(setting.velocityMax.y, setting.velocityMin.y, FLT_MAX);
                setting.velocityMin.z = std::clamp(setting.velocityMin.z, -FLT_MAX, setting.velocityMax.z);
                setting.velocityMax.z = std::clamp(setting.velocityMax.z, setting.velocityMin.z, FLT_MAX);

                ImGui::Text("加速度:");
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.2f, 0.4f, 0.4f));
                ImGui::DragFloat3("最初", &setting.startAcce.x, 0.001f);
                ImGui::DragFloat3("最後", &setting.endAcce.x, 0.001f);
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.4f, 0.8f, 1.0f));
                ImGui::Checkbox("乗算", &setting.isAcceMultiply);
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.2f, 0.4f));
                ImGui::DragFloat("重力", &setting.gravity, 0.01f, -FLT_MAX, FLT_MAX);
                ImGui::PopStyleColor();
                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            // サイズ
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
            if (ImGui::TreeNode("大きさ")) {
                ImGui::PopStyleColor();

                ImGui::Text("大きさ:");

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.4f, 0.2f, 0.4f));
                if (setting.isRandomAllSize) {
                    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
                    ImGui::Checkbox("最初と最後同じ大きさ", &setting.isEndScale);
                    ImGui::PopStyleColor();

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
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
                ImGui::Checkbox("均等にランダムな大きさ", &setting.isRandomSize);
                ImGui::Checkbox("ばらばらにランダムな大きさ", &setting.isRandomAllSize);
                ImGui::Checkbox("sin波の動き", &setting.isSinMove);
                ImGui::PopStyleColor();
                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            // 回転
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 1.0f, 1.0f, 1.0f));
            if (ImGui::TreeNode("回転")) {
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.4f, 0.4f));
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

                    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.8f, 1.0f));
                    ImGui::Checkbox("ランダムな回転速度", &setting.isRotateVelocity);
                    ImGui::PopStyleColor();

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
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.8f, 1.0f));
                ImGui::Checkbox("ランダムな回転", &setting.isRandomRotate);
                ImGui::Checkbox("進行方向に向ける", &setting.isFaceDirection);
                ImGui::PopStyleColor();
                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            // トレイル設定
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 1.0f, 1.0f));
            if (ImGui::TreeNode("トレイル設定")) {
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.2f, 0.8f, 1.0f));
                if (ImGui::Checkbox("トレイルを有効にする", &setting.enableTrail)) {
                    SetTrailEnabled(selectedGroup, setting.enableTrail);
                }
                ImGui::PopStyleColor();

                if (setting.enableTrail) {
                    ImGui::Indent();
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.4f, 0.4f));

                    if (ImGui::DragFloat("トレイル生成間隔", &setting.trailSpawnInterval, 0.001f, 0.001f, 10.0f)) {
                        SetTrailInterval(selectedGroup, setting.trailSpawnInterval);
                    }
                    if (ImGui::DragFloat("トレイル生存時間スケール", &setting.trailLifeScale, 0.01f)) {
                        SetTrailLifeScale(selectedGroup, setting.trailLifeScale);
                    }
                    if (ImGui::DragFloat3("トレイルスケール倍率", &setting.trailScaleMultiplier.x, 0.01f)) {
                        SetTrailScaleMultiplier(selectedGroup, setting.trailScaleMultiplier);
                    }
                    if (ImGui::DragFloat4("トレイル色彩倍率", &setting.trailColorMultiplier.x, 0.01f)) {
                        SetTrailColorMultiplier(selectedGroup, setting.trailColorMultiplier);
                    }

                    ImGui::PopStyleColor();

                    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.2f, 0.8f, 1.0f));
                    if (ImGui::Checkbox("トレイル速度継承", &setting.trailInheritVelocity)) {
                        SetTrailVelocityInheritance(selectedGroup, setting.trailInheritVelocity, setting.trailVelocityScale);
                    }
                    ImGui::PopStyleColor();

                    if (setting.trailInheritVelocity) {
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.4f, 0.4f));
                        if (ImGui::DragFloat("トレイル速度スケール", &setting.trailVelocityScale, 0.01f)) {
                            SetTrailVelocityInheritance(selectedGroup, setting.trailInheritVelocity, setting.trailVelocityScale);
                        }
                        ImGui::PopStyleColor();
                    }
                    ImGui::Unindent();
                }
                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            // 色彩
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.8f, 1.0f));
            if (ImGui::TreeNode("色彩")) {
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.8f, 1.0f));
                if (ImGui::TreeNode("色")) {
                    ImGui::PopStyleColor();
                    ImGui::ColorEdit4("開始色", &setting.startColor.x);
                    ImGui::ColorEdit4("終了色", &setting.endColor.x);
                    ImGui::TreePop();
                } else {
                    ImGui::PopStyleColor();
                }

                // 透明度設定
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.0f, 1.0f));
                if (ImGui::TreeNode("透明度")) {
                    ImGui::PopStyleColor();
                    ImGui::Text("透明度の設定:");
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.4f, 0.4f));
                    ImGui::DragFloat("最大値", &setting.alphaMax, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("最小値", &setting.alphaMin, 0.01f, 0.0f, 1.0f);
                    ImGui::PopStyleColor();
                    setting.alphaMin = std::clamp(setting.alphaMin, 0.0f, setting.alphaMax);
                    setting.alphaMax = std::clamp(setting.alphaMax, setting.alphaMin, 1.0f);
                    ImGui::TreePop();
                } else {
                    ImGui::PopStyleColor();
                }
                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }
        } else {
            ImGui::PopStyleColor(3);
        }

        ImGui::Spacing();

        // エミット設定セクション
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.4f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.5f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.4f, 0.6f, 1.0f));

        if (ImGui::CollapsingHeader("パーティクルの数、間隔")) {
            ImGui::PopStyleColor(3);

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.4f, 0.5f));
            ImGui::DragFloat("間隔", &emitFrequency_, 0.001f, 0.001f, 100.0f);
            ImGui::InputInt("数", reinterpret_cast<int *>(&setting.count), 1, 100);
            ImGui::PopStyleColor();
            setting.count = std::clamp(static_cast<int>(setting.count), 0, 10000);
        } else {
            ImGui::PopStyleColor(3);
        }

        ImGui::Spacing();

        // その他の設定セクション
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        if (ImGui::CollapsingHeader("各状態の設定")) {
            ImGui::PopStyleColor(3);

            ImGui::Spacing();

            // チェックボックス用のスタイル設定
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.6f, 0.8f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.7f));

            // レンダリング設定
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.7f, 1.0f));
            ImGui::Text("レンダリング設定:");
            ImGui::PopStyleColor();

            ImGui::Separator();
            ImGui::Indent();

            ImGui::Checkbox("ビルボード", &setting.isBillboard);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("パーティクルを常にカメラに向ける");
            }

            ImGui::Checkbox("Xビルボード", &setting.isBillboardX);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("パーティクルのX軸を常にカメラに向ける");
            }

            ImGui::Checkbox("Yビルボード", &setting.isBillboardY);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("パーティクルのY軸を常にカメラに向ける");
            }

            ImGui::Checkbox("Zビルボード", &setting.isBillboardZ);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("パーティクルのZ軸を常にカメラに向ける");
            }

            ImGui::Checkbox("ランダムカラー", &setting.isRandomColor);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("パーティクルごとに異なる色を適用");
            }

            ImGui::Unindent();
            ImGui::PopStyleColor(3); // CheckMark, FrameBg, FrameBgHovered
            ImGui::Spacing();

        } else {
            ImGui::PopStyleColor(3);
        }

        ImGui::Spacing();
    } else {
        // グループがない場合の表示
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("グループがありません。");
        ImGui::PopStyleColor();
    }
    // グループ管理セクション
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.3f, 0.5f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.4f, 0.6f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.5f, 0.7f, 1.0f));

    if (ImGui::CollapsingHeader("グループ管理")) {
        ImGui::PopStyleColor(3);

        ImGui::Spacing();

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

        // ヘッダーテキストのスタイル
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
        ImGui::Text("利用可能なグループ");
        ImGui::SameLine(width - halfWidth - 50);
        ImGui::Text("アタッチ済みグループ");
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));

        // 左リスト用のスタイル設定
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.15f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.4f, 0.5f, 0.8f));

        // 左リスト（未アタッチグループ）
        ImGui::BeginChild("available_groups", ImVec2(halfWidth, 200), true, ImGuiWindowFlags_AlwaysUseWindowPadding);

        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.7f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));

        if (availableItems.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("利用可能なグループがありません");
            ImGui::PopStyleColor();
        } else {
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
                            particleGroupNames_ = Manager_->GetParticleGroupsName();
                        }
                        leftSelected.clear();
                    }
                }
            }
        }

        ImGui::PopStyleColor(3); // Header colors
        ImGui::EndChild();

        ImGui::SameLine();

        // 中央のボタン群
        ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 12));

        // ボタンのスタイル設定
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));

        // 追加ボタン
        ImGui::PushID("move_right");
        bool canMoveRight = !leftSelected.empty();
        if (!canMoveRight) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        }

        if (ImGui::Button("追加 >>", ImVec2(80, 35)) && canMoveRight) {
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

        if (!canMoveRight) {
            ImGui::PopStyleColor(3);
        }
        ImGui::PopID();

        // 削除ボタン
        ImGui::PushID("move_left");
        bool canMoveLeft = !rightSelected.empty();
        if (!canMoveLeft) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        }

        if (ImGui::Button("<< 削除", ImVec2(80, 35)) && canMoveLeft) {
            for (auto it = rightSelected.rbegin(); it != rightSelected.rend(); ++it) {
                int idx = *it;
                RemoveParticleGroup(attachedNames[idx]);
                particleGroupNames_ = Manager_->GetParticleGroupsName();
            }
            rightSelected.clear();
        }

        if (!canMoveLeft) {
            ImGui::PopStyleColor(3);
        }
        ImGui::PopID();

        ImGui::PopStyleColor(3); // Button colors
        ImGui::PopStyleVar();    // ボタン間の余白復元
        ImGui::EndGroup();

        ImGui::SameLine();

        // 右リスト（アタッチ済みグループ）
        ImGui::BeginChild("attached_groups", ImVec2(halfWidth, 200), true, ImGuiWindowFlags_AlwaysUseWindowPadding);

        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.4f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.7f, 0.5f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.8f, 0.6f, 0.4f, 1.0f));

        if (attachedItems.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("アタッチされたグループがありません");
            ImGui::PopStyleColor();
        } else {
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
                        particleGroupNames_ = Manager_->GetParticleGroupsName();
                        rightSelected.clear();
                    }
                }
            }
        }

        ImGui::PopStyleColor(3); // Header colors
        ImGui::EndChild();

        ImGui::PopStyleColor(2); // ChildBg, Border
        ImGui::PopStyleVar();    // ItemSpacing 戻す

        ImGui::Spacing();
        ImGui::Separator();

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

    if (ImGui::CollapsingHeader("ファイル操作")) {
        ImGui::PopStyleColor(3);

        ImGui::Spacing();

        // セーブボタン
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

        if (ImGui::Button("設定を保存", ImVec2(120, 35))) {
            SaveToJson();
            std::string message = std::format("ParticleData saved.");
            MessageBoxA(nullptr, message.c_str(), "Particle", 0);
        }
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("現在のパーティクル設定をJSONファイルに保存します");
        }

        ImGui::Spacing();

    } else {
        ImGui::PopStyleColor(3);
    }

    ImGui::Spacing();

    // 制御セクション
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.2f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.3f, 0.4f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.7f, 0.4f, 0.5f, 1.0f));

    if (ImGui::CollapsingHeader("エミッター制御")) {
        ImGui::PopStyleColor(3);

        ImGui::Spacing();

        // 自動生成チェックボックス
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.2f, 0.1f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.3f, 0.2f, 0.7f));

        ImGui::Checkbox("自動生成", &isAuto_);
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("パーティクルを自動的に継続生成します");
        }

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // 手動生成ボタン
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.1f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.5f, 0.3f, 1.0f));

        if (ImGui::Button("手動生成", ImVec2(100, 30))) {
            UpdateOnce();
        }
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("パーティクルを1回だけ生成します");
        }

        ImGui::Spacing();

    } else {
        ImGui::PopStyleColor(3);
    }

    // メインウィンドウの背景色をポップ
    ImGui::PopStyleColor();
}

// ImGuiで値を動かす関数
void ParticleEmitter::Debug() {
#ifdef _DEBUG
    if (!name_.empty() && Manager_) {
        DebugParticleData();
    }
#endif
}
#pragma endregion

bool ParticleEmitter::IsAllParticlesComplete() {
    return Manager_->IsAllParticlesComplete();
}
void ParticleEmitter::AddParticleGroup(ParticleGroup *particleGroup) {
    if (!particleGroup)
        return;

    // パーティクルグループ名を取得
    const std::string &groupName = particleGroup->GetGroupName();

    // 独立したコピーを作成
    ParticleGroup *independentGroup = ParticleGroupManager::GetInstance()->GetIndependentParticleGroup(groupName);
    if (!independentGroup) {
        return;
    }

    // 設定が存在しない場合はデフォルト値で初期化
    auto it = particleSettings_.find(groupName);
    if (it == particleSettings_.end()) {
        particleSettings_[groupName] = DefaultSetting();
    }

    Manager_->AddParticleGroup(independentGroup);
}

std::unique_ptr<ParticleEmitter> ParticleEmitter::Clone() const {
    auto newEmitter = std::make_unique<ParticleEmitter>();

    // コピー元の情報を複製
    newEmitter->SetName(this->name_ + "_clone");
    newEmitter->SetFrequency(this->emitFrequency_);
    newEmitter->SetActive(this->isActive_);
    newEmitter->isAuto_ = this->isAuto_;
    newEmitter->isVisible_ = this->isVisible_;
    newEmitter->transform_ = this->transform_;
    newEmitter->particleSettings_ = this->particleSettings_;
    newEmitter->particleGroupNames_ = this->particleGroupNames_;

    // ParticleManager は個別に生成して初期化
    newEmitter->Manager_ = std::make_unique<ParticleManager>();
    newEmitter->Manager_->Initialize(SrvManager::GetInstance());

    // 同じグループを再アタッチ（共有参照でOK）
    for (const auto &groupName : particleGroupNames_) {
        ParticleGroup *group = ParticleGroupManager::GetInstance()->GetParticleGroup(groupName);
        if (group) {
            newEmitter->AddParticleGroup(group);
        }
    }

    return newEmitter;
}

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

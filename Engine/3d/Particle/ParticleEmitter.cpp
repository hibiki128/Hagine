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
    transform_.translation_ = particleSetting_.translate;
    transform_.rotation_ = particleSetting_.rotation;
    transform_.scale_ = particleSetting_.scale;
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
    transform_.translation_ = particleSetting_.translate;
    transform_.rotation_ = particleSetting_.rotation;
    transform_.scale_ = particleSetting_.scale;
    transform_.UpdateMatrix();
    if (Manager_) {
        Manager_->Update(vp_);
        Manager_->Draw();
    }
    DrawEmitter();
}

void ParticleEmitter::DrawEmitter() {
    // isVisibleがtrueのときだけ描画
    if (!isVisible_)
        return;

    // 立方体のローカル座標での基本頂点（スケーリング済み）
    std::array<Vector3, 8> localVertices = {
        Vector3{-1.0f, -1.0f, -1.0f}, // 左下手前
        Vector3{1.0f, -1.0f, -1.0f},  // 右下手前
        Vector3{-1.0f, 1.0f, -1.0f},  // 左上手前
        Vector3{1.0f, 1.0f, -1.0f},   // 右上手前
        Vector3{-1.0f, -1.0f, 1.0f},  // 左下奥
        Vector3{1.0f, -1.0f, 1.0f},   // 右下奥
        Vector3{-1.0f, 1.0f, 1.0f},   // 左上奥
        Vector3{1.0f, 1.0f, 1.0f}     // 右上奥
    };

    // ワールド変換結果を格納する配列
    std::array<Vector3, 8> worldVertices;

    // ワールド行列の計算
    Matrix4x4 worldMatrix = MakeAffineMatrix(particleSetting_.scale, particleSetting_.rotation, particleSetting_.translate);

    // 頂点のワールド変換
    for (size_t i = 0; i < localVertices.size(); i++) {
        worldVertices[i] = Transformation(localVertices[i], worldMatrix);
    }

    // エッジリスト（線の接続順）
    constexpr std::array<std::pair<int, int>, 12> edges = {
        std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0), // 前面
        std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4), // 背面
        std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)  // 側面
    };

    // エッジ描画
    for (const auto &edge : edges) {
        DrawLine3D::GetInstance()->SetPoints(worldVertices[edge.first], worldVertices[edge.second]);
    }
}

// Emit関数
void ParticleEmitter::Emit() {
    // ParticleManagerのEmit関数を呼び出す
    Manager_->SetParticleSetting(particleSetting_);
    Manager_->Emit();
}

#pragma region ImGui関連

void ParticleEmitter::SaveToJson() {

    datas_->Save("emitterTranslation", particleSetting_.translate);
    datas_->Save("emitterRotation", particleSetting_.rotation);
    datas_->Save("emitterScale", particleSetting_.scale);
    datas_->Save("count", particleSetting_.count);
    datas_->Save("emitFrequency", emitFrequency_);
    datas_->Save("lifeTimeMin", particleSetting_.lifeTimeMin);
    datas_->Save("lifeTimeMax", particleSetting_.lifeTimeMax);
    datas_->Save("alphaMin", particleSetting_.alphaMin);
    datas_->Save("alphaMax", particleSetting_.alphaMax);
    datas_->Save("scaleMin", particleSetting_.scaleMin);
    datas_->Save("scaleMax", particleSetting_.scaleMax);
    datas_->Save("velocityMin", particleSetting_.velocityMin);
    datas_->Save("velocityMax", particleSetting_.velocityMax);
    datas_->Save("startScale", particleSetting_.particleStartScale);
    datas_->Save("endScale", particleSetting_.particleEndScale);
    datas_->Save("startAcce", particleSetting_.startAcce);
    datas_->Save("endAcce", particleSetting_.endAcce);
    datas_->Save("startRote", particleSetting_.startRote);
    datas_->Save("endRote", particleSetting_.endRote);
    datas_->Save("rotateStartMax", particleSetting_.rotateStartMax);
    datas_->Save("rotateStartMin", particleSetting_.rotateStartMin);
    datas_->Save("rotateVelocityMin", particleSetting_.rotateVelocityMin);
    datas_->Save("rotateVelocityMax", particleSetting_.rotateVelocityMax);
    datas_->Save("allScaleMin", particleSetting_.allScaleMin);
    datas_->Save("allScaleMax", particleSetting_.allScaleMax);
    datas_->Save("isRandomScale", particleSetting_.isRandomSize);
    datas_->Save("isAllRamdomScale", particleSetting_.isRandomAllSize);
    datas_->Save("isRandomColor", particleSetting_.isRandomColor);
    datas_->Save("isRandomRotate", particleSetting_.isRandomRotate);
    datas_->Save("isVisible",isVisible_);
    datas_->Save("isBillBoard", isBillBoard_);
    datas_->Save("isActive", isActive_);
    datas_->Save("isAcceMultiply", particleSetting_.isAcceMultiply);
    datas_->Save("isSinMove", particleSetting_.isSinMove);
    datas_->Save("isFaceDirection", particleSetting_.isFaceDirection);
    datas_->Save("isEndScale", particleSetting_.isEndScale);
    datas_->Save("isEmitOnEdge", particleSetting_.isEmitOnEdge);
    datas_->Save("isGatherMode", particleSetting_.isGatherMode);
    datas_->Save("gatherStartRatio", particleSetting_.gatherStartRatio);
    datas_->Save("gatherStrength", particleSetting_.gatherStrength);
    particleGroupNames_ = Manager_->GetParticleGroupsName();
    int count = 0;
    for (auto &particleGroupName : particleGroupNames_) {
        datas_->Save("GroupName" + std::to_string(count), particleGroupName);
        count++;
    }
}

void ParticleEmitter::LoadFromJson() {
    if (!name_.empty()) {
        particleSetting_.translate = datas_->Load<Vector3>("emitterTranslation", {0.0f, 0.0f, 0.0f});
        particleSetting_.rotation = datas_->Load<Vector3>("emitterRotation", {0.0f, 0.0f, 0.0f});
        particleSetting_.scale = datas_->Load<Vector3>("emitterScale", {1.0f, 1.0f, 1.0f});
        particleSetting_.count = datas_->Load<int>("count", 1);
        emitFrequency_ = datas_->Load<float>("emitFrequency", 0.1f);
        particleSetting_.lifeTimeMin = datas_->Load<float>("lifeTimeMin", 1.0f);
        particleSetting_.lifeTimeMax = datas_->Load<float>("lifeTimeMax", 3.0f);
        particleSetting_.alphaMin = datas_->Load<float>("alphaMin", 1.0f);
        particleSetting_.alphaMax = datas_->Load<float>("alphaMax", 1.0f);
        particleSetting_.scaleMin = datas_->Load<float>("scaleMin", 1.0f);
        particleSetting_.scaleMax = datas_->Load<float>("scaleMax", 1.0f);
        particleSetting_.velocityMin = datas_->Load<Vector3>("velocityMin", {-1.0f, -1.0f, -1.0f});
        particleSetting_.velocityMax = datas_->Load<Vector3>("velocityMax", {1.0f, 1.0f, 1.0f});
        particleSetting_.particleStartScale = datas_->Load<Vector3>("startScale", {1.0f, 1.0f, 1.0f});
        particleSetting_.particleEndScale = datas_->Load<Vector3>("endScale", {0.0f, 0.0f, 0.0f});
        particleSetting_.startAcce = datas_->Load<Vector3>("startAcce", {1.0f, 1.0f, 1.0f});
        particleSetting_.endAcce = datas_->Load<Vector3>("endAcce", {1.0f, 1.0f, 1.0f});
        particleSetting_.startRote = datas_->Load<Vector3>("startRote", {0.0f, 0.0f, 0.0f});
        particleSetting_.endRote = datas_->Load<Vector3>("endRote", {0.0f, 0.0f, 0.0f});
        particleSetting_.rotateStartMax = datas_->Load<Vector3>("rotateStartMax", {0.0f, 0.0f, 0.0f});
        particleSetting_.rotateStartMin = datas_->Load<Vector3>("rotateStartMin", {0.0f, 0.0f, 0.0f});
        particleSetting_.rotateVelocityMin = datas_->Load<Vector3>("rotateVelocityMin", {-0.07f, -0.07f, -0.07f});
        particleSetting_.rotateVelocityMax = datas_->Load<Vector3>("rotateVelocityMax", {0.07f, 0.07f, 0.07f});
        particleSetting_.allScaleMin = datas_->Load<Vector3>("allScaleMin", {0.0f, 0.0f, 0.0f});
        particleSetting_.allScaleMax = datas_->Load<Vector3>("allScaleMax", {1.0f, 1.0f, 1.0f});
        particleSetting_.isRandomSize = datas_->Load<bool>("isRandomScale", false);
        particleSetting_.isRandomAllSize = datas_->Load<bool>("isAllRamdomScale", false);
        particleSetting_.isRandomColor = datas_->Load<bool>("isRandomColor", false);
        particleSetting_.isRandomRotate = datas_->Load<bool>("isRandomRotate", false);
        isVisible_ = datas_->Load<bool>("isVisible", true);
        isBillBoard_ = datas_->Load<bool>("isBillBoard", false);
        isActive_ = datas_->Load<bool>("isActive", false);
        particleSetting_.isAcceMultiply = datas_->Load<bool>("isAcceMultiply", false);
        particleSetting_.isSinMove = datas_->Load<bool>("isSinMove", false);
        particleSetting_.isFaceDirection = datas_->Load<bool>("isFaceDirection", false);
        particleSetting_.isEndScale = datas_->Load<bool>("isEndScale", false);
        particleSetting_.isEmitOnEdge = datas_->Load<bool>("isEmitOnEdge", false);
        particleSetting_.isGatherMode = datas_->Load<bool>("isGatherMode", false);
        particleSetting_.gatherStartRatio = datas_->Load<float>("gatherStartRatio", 0.0f);
        particleSetting_.gatherStrength = datas_->Load<float>("gatherStrength", 0.0f);
        for (size_t i = 0; i < ParticleGroupManager::GetInstance()->GetParticleGroups().size(); i++) {
            std::string groupName = datas_->Load<std::string>("GroupName_" + std::to_string(i), "");
            if (groupName == "") {
                break;
            }
            particleGroupNames_.push_back(groupName);
        }
    }
}

void ParticleEmitter::LoadParticleGroup() {
    for (auto &particleGroupname : particleGroupNames_) {
        AddParticleGroup(ParticleGroupManager::GetInstance()->GetParticleGroup(particleGroupname));
    }
}

void ParticleEmitter::DebugParticleData() {

    if (ImGui::BeginTabBar("パーティクル")) {
        if (ImGui::BeginTabItem(name_.c_str())) {

            // 基本データセクション
            if (ImGui::CollapsingHeader("エミッターデータ")) {
                // トランスフォームデータをフレーム内に配置
                ImGui::Text("Transformデータ:");
                ImGui::Separator();
                ImGui::Columns(2, "TransformColumns", false); // 2列レイアウト
                ImGui::Text("位置");
                ImGui::NextColumn();
                ImGui::DragFloat3("##位置", &particleSetting_.translate.x, 0.1f);
                ImGui::NextColumn();
                ImGui::Text("回転");
                ImGui::NextColumn();
                float rotationDegrees[3] = {
                    radiansToDegrees(particleSetting_.rotation.x),
                    radiansToDegrees(particleSetting_.rotation.y),
                    radiansToDegrees(particleSetting_.rotation.z)};

                // ドラッグUIを使用し、度数法で値を操作
                if (ImGui::DragFloat3("##回転 (度)", rotationDegrees, 0.1f, -360.0f, 360.0f)) {
                    // 操作後、度数法からラジアンに戻して保存
                    particleSetting_.rotation.x = degreesToRadians(rotationDegrees[0]);
                    particleSetting_.rotation.y = degreesToRadians(rotationDegrees[1]);
                    particleSetting_.rotation.z = degreesToRadians(rotationDegrees[2]);
                }

                ImGui::NextColumn();
                ImGui::Text("大きさ");
                ImGui::NextColumn();
                ImGui::DragFloat3("##大きさ", &particleSetting_.scale.x, 0.1f, 0.0f);
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
                    ImGui::DragFloat("最大値", &particleSetting_.lifeTimeMax, 0.1f, 0.0f);
                    ImGui::DragFloat("最小値", &particleSetting_.lifeTimeMin, 0.1f, 0.0f);
                    particleSetting_.lifeTimeMin = std::clamp(particleSetting_.lifeTimeMin, 0.0f, particleSetting_.lifeTimeMax);
                    particleSetting_.lifeTimeMax = std::clamp(particleSetting_.lifeTimeMax, particleSetting_.lifeTimeMin, 10.0f);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                if (ImGui::TreeNode("位置")) {
                    ImGui::Checkbox("中心に集める", &particleSetting_.isGatherMode);
                    if (particleSetting_.isGatherMode) {
                        ImGui::DragFloat("強さ", &particleSetting_.gatherStrength, 0.1f);
                        ImGui::DragFloat("始まるタイミング", &particleSetting_.gatherStartRatio, 0.1f);
                    }
                    ImGui::Checkbox("外周", &particleSetting_.isEmitOnEdge);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 速度と加速度
                if (ImGui::TreeNode("速度、加速度")) {
                    ImGui::Text("速度:");
                    ImGui::DragFloat3("最大値", &particleSetting_.velocityMax.x, 0.1f);
                    ImGui::DragFloat3("最小値", &particleSetting_.velocityMin.x, 0.1f);
                    particleSetting_.velocityMin.x = std::clamp(particleSetting_.velocityMin.x, -FLT_MAX, particleSetting_.velocityMax.x);
                    particleSetting_.velocityMax.x = std::clamp(particleSetting_.velocityMax.x, particleSetting_.velocityMin.x, FLT_MAX);
                    particleSetting_.velocityMin.y = std::clamp(particleSetting_.velocityMin.y, -FLT_MAX, particleSetting_.velocityMax.y);
                    particleSetting_.velocityMax.y = std::clamp(particleSetting_.velocityMax.y, particleSetting_.velocityMin.y, FLT_MAX);
                    particleSetting_.velocityMin.z = std::clamp(particleSetting_.velocityMin.z, -FLT_MAX, particleSetting_.velocityMax.z);
                    particleSetting_.velocityMax.z = std::clamp(particleSetting_.velocityMax.z, particleSetting_.velocityMin.z, FLT_MAX);
                    ImGui::Text("加速度:");
                    ImGui::DragFloat3("最初", &particleSetting_.startAcce.x, 0.001f);
                    ImGui::DragFloat3("最後", &particleSetting_.endAcce.x, 0.001f);
                    ImGui::Checkbox("乗算", &particleSetting_.isAcceMultiply);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // サイズ
                if (ImGui::TreeNode("大きさ")) {
                    ImGui::Text("大きさ:");
                    if (particleSetting_.isRandomAllSize) {
                        ImGui::Checkbox("最初と最後同じ大きさ", &particleSetting_.isEndScale);
                        ImGui::DragFloat3("最大値", &particleSetting_.allScaleMax.x, 0.1f, 0.0f);
                        ImGui::DragFloat3("最小値", &particleSetting_.allScaleMin.x, 0.1f, 0.0f);
                        particleSetting_.allScaleMin.x = std::clamp(particleSetting_.allScaleMin.x, -FLT_MAX, particleSetting_.allScaleMax.x);
                        particleSetting_.allScaleMax.x = std::clamp(particleSetting_.allScaleMax.x, particleSetting_.allScaleMin.x, FLT_MAX);
                        particleSetting_.allScaleMin.y = std::clamp(particleSetting_.allScaleMin.y, -FLT_MAX, particleSetting_.allScaleMax.y);
                        particleSetting_.allScaleMax.y = std::clamp(particleSetting_.allScaleMax.y, particleSetting_.allScaleMin.y, FLT_MAX);
                        particleSetting_.allScaleMin.z = std::clamp(particleSetting_.allScaleMin.z, -FLT_MAX, particleSetting_.allScaleMax.z);
                        particleSetting_.allScaleMax.z = std::clamp(particleSetting_.allScaleMax.z, particleSetting_.allScaleMin.z, FLT_MAX);
                        if (!particleSetting_.isEndScale) {
                            ImGui::DragFloat3("最後", &particleSetting_.particleEndScale.x, 0.1f);
                        }
                    } else if (particleSetting_.isRandomSize) {
                        ImGui::DragFloat("最大値", &particleSetting_.scaleMax, 0.1f, 0.0f);
                        ImGui::DragFloat("最小値", &particleSetting_.scaleMin, 0.1f, 0.0f);
                        particleSetting_.scaleMax = std::clamp(particleSetting_.scaleMax, particleSetting_.scaleMin, FLT_MAX);
                        particleSetting_.scaleMin = std::clamp(particleSetting_.scaleMin, 0.0f, particleSetting_.scaleMax);
                    } else if (particleSetting_.isSinMove) {
                        ImGui::DragFloat3("最初", &particleSetting_.particleStartScale.x, 0.1f, 0.0f);
                    } else {
                        ImGui::DragFloat3("最初", &particleSetting_.particleStartScale.x, 0.1f, 0.0f);
                        ImGui::DragFloat3("最後", &particleSetting_.particleEndScale.x, 0.1f);
                    }
                    ImGui::Checkbox("均等にランダムな大きさ", &particleSetting_.isRandomSize);
                    ImGui::Checkbox("ばらばらにランダムな大きさ", &particleSetting_.isRandomAllSize);
                    ImGui::Checkbox("sin波の動き", &particleSetting_.isSinMove);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 回転
                if (ImGui::TreeNode("回転")) {
                    if (!particleSetting_.isRandomRotate) {
                        float startRotationDegrees[3] = {
                            radiansToDegrees(particleSetting_.startRote.x),
                            radiansToDegrees(particleSetting_.startRote.y),
                            radiansToDegrees(particleSetting_.startRote.z)};
                        float endRotationDegrees[3] = {
                            radiansToDegrees(particleSetting_.endRote.x),
                            radiansToDegrees(particleSetting_.endRote.y),
                            radiansToDegrees(particleSetting_.endRote.z)};
                        if (ImGui::DragFloat3("最初", startRotationDegrees, 0.1f)) {
                            particleSetting_.startRote.x = degreesToRadians(startRotationDegrees[0]);
                            particleSetting_.startRote.y = degreesToRadians(startRotationDegrees[1]);
                            particleSetting_.startRote.z = degreesToRadians(startRotationDegrees[2]);
                        }
                        if (ImGui::DragFloat3("最後", endRotationDegrees, 0.1f)) {
                            particleSetting_.endRote.x = degreesToRadians(endRotationDegrees[0]);
                            particleSetting_.endRote.y = degreesToRadians(endRotationDegrees[1]);
                            particleSetting_.endRote.z = degreesToRadians(endRotationDegrees[2]);
                        }
                    }
                    if (particleSetting_.isRandomRotate) {
                        float startRotationDegrees[3] = {
                            radiansToDegrees(particleSetting_.rotateStartMax.x),
                            radiansToDegrees(particleSetting_.rotateStartMax.y),
                            radiansToDegrees(particleSetting_.rotateStartMax.z)};
                        float endRotationDegrees[3] = {
                            radiansToDegrees(particleSetting_.rotateStartMin.x),
                            radiansToDegrees(particleSetting_.rotateStartMin.y),
                            radiansToDegrees(particleSetting_.rotateStartMin.z)};

                        if (ImGui::DragFloat3("回転 最大値", startRotationDegrees, 0.1f)) {
                            particleSetting_.rotateStartMax.x = degreesToRadians(std::clamp(startRotationDegrees[0], radiansToDegrees(particleSetting_.rotateStartMin.x), 180.0f));
                            particleSetting_.rotateStartMax.y = degreesToRadians(std::clamp(startRotationDegrees[1], radiansToDegrees(particleSetting_.rotateStartMin.y), 180.0f));
                            particleSetting_.rotateStartMax.z = degreesToRadians(std::clamp(startRotationDegrees[2], radiansToDegrees(particleSetting_.rotateStartMin.z), 180.0f));
                        }

                        if (ImGui::DragFloat3("回転 最小値", endRotationDegrees, 0.1f)) {
                            particleSetting_.rotateStartMin.x = degreesToRadians(std::clamp(endRotationDegrees[0], -180.0f, radiansToDegrees(particleSetting_.rotateStartMax.x)));
                            particleSetting_.rotateStartMin.y = degreesToRadians(std::clamp(endRotationDegrees[1], -180.0f, radiansToDegrees(particleSetting_.rotateStartMax.y)));
                            particleSetting_.rotateStartMin.z = degreesToRadians(std::clamp(endRotationDegrees[2], -180.0f, radiansToDegrees(particleSetting_.rotateStartMax.z)));
                        }

                        ImGui::Checkbox("ランダムな回転速度", &particleSetting_.isRotateVelocity);

                        if (particleSetting_.isRotateVelocity) {
                            ImGui::DragFloat3("最大値", &particleSetting_.rotateVelocityMax.x, 0.01f);
                            ImGui::DragFloat3("最小値", &particleSetting_.rotateVelocityMin.x, 0.01f);
                            particleSetting_.rotateVelocityMin.x = std::clamp(particleSetting_.rotateVelocityMin.x, -FLT_MAX, particleSetting_.rotateVelocityMax.x);
                            particleSetting_.rotateVelocityMax.x = std::clamp(particleSetting_.rotateVelocityMax.x, particleSetting_.rotateVelocityMin.x, FLT_MAX);
                            particleSetting_.rotateVelocityMin.y = std::clamp(particleSetting_.rotateVelocityMin.y, -FLT_MAX, particleSetting_.rotateVelocityMax.y);
                            particleSetting_.rotateVelocityMax.y = std::clamp(particleSetting_.rotateVelocityMax.y, particleSetting_.rotateVelocityMin.y, FLT_MAX);
                            particleSetting_.rotateVelocityMin.z = std::clamp(particleSetting_.rotateVelocityMin.z, -FLT_MAX, particleSetting_.rotateVelocityMax.z);
                            particleSetting_.rotateVelocityMax.z = std::clamp(particleSetting_.rotateVelocityMax.z, particleSetting_.rotateVelocityMin.z, FLT_MAX);
                        }
                    }
                    ImGui::Checkbox("ランダムな回転", &particleSetting_.isRandomRotate);
                    ImGui::Checkbox("進行方向に向ける", &particleSetting_.isFaceDirection);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // Alphaを折りたたみ可能にする
                if (ImGui::TreeNode("透明度")) {
                    ImGui::Text("透明度の設定:");
                    ImGui::DragFloat("最大値", &particleSetting_.alphaMax, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("最小値", &particleSetting_.alphaMin, 0.01f, 0.0f, 1.0f);
                    particleSetting_.alphaMin = std::clamp(particleSetting_.alphaMin, 0.0f, particleSetting_.alphaMax);
                    particleSetting_.alphaMax = std::clamp(particleSetting_.alphaMax, particleSetting_.alphaMin, 1.0f);
                    ImGui::TreePop();
                }
            }

            // エミット設定セクション
            if (ImGui::CollapsingHeader("パーティクルの数、間隔")) {
                ImGui::DragFloat("間隔", &emitFrequency_, 0.001f, 0.001f, 100.0f);
                ImGui::InputInt("数", reinterpret_cast<int*>(&particleSetting_.count), 1, 100);
                particleSetting_.count = std::clamp(static_cast<int>(particleSetting_.count), 0, 10000);
            }

            // その他の設定セクション
            if (ImGui::CollapsingHeader("各状態の設定")) {
                ImGui::Checkbox("ビルボード", &isBillBoard_);
                ImGui::Checkbox("ランダムカラー", &particleSetting_.isRandomColor);
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
    Manager_->AddParticleGroup(particleGroup);
}
#pragma endregion
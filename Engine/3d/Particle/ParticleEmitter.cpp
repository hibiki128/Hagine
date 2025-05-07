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
        Manager_->SetRandomRotate(isRandomRotate_);
        Manager_->SetRotateVelocity(isRotateVelocity_);
        Manager_->SetAcceMultipy(isAcceMultiply_);
        Manager_->SetBillBorad(isBillBoard_);
        Manager_->SetRandomSize(isRandomScale_);
        Manager_->SetAllRandomSize(isAllRamdomScale_);
        Manager_->SetSinMove(isSinMove_);
        Manager_->SetFaceDirection(isFaceDirection_);
        Manager_->SetEndScale(isEndScale_);
        Manager_->SetOnEdge(isEmitOnEdge_);
        Manager_->SetGatherMode(isGatherMode_);
        Manager_->SetGatherStartRatio(gatherStartRatio_);
        Manager_->SetGatherStrength(gatherStrength_);
        Emit();                         // パーティクルを発生させる
        elapsedTime_ -= emitFrequency_; // 過剰に進んだ時間を考慮
    }
}

void ParticleEmitter::UpdateOnce() {
    isActive_ = false;
    if (!isActive_) {
        Manager_->SetRandomRotate(isRandomRotate_);
        Manager_->SetRotateVelocity(isRotateVelocity_);
        Manager_->SetAcceMultipy(isAcceMultiply_);
        Manager_->SetBillBorad(isBillBoard_);
        Manager_->SetRandomSize(isRandomScale_);
        Manager_->SetAllRandomSize(isAllRamdomScale_);
        Manager_->SetSinMove(isSinMove_);
        Manager_->SetFaceDirection(isFaceDirection_);
        Manager_->SetEndScale(isEndScale_);
        Manager_->SetOnEdge(isEmitOnEdge_);
        Manager_->SetGatherMode(isGatherMode_);
        Manager_->SetGatherStartRatio(gatherStartRatio_);
        Manager_->SetGatherStrength(gatherStrength_);
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
    Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale_, transform_.rotation_, transform_.translation_);

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
    Manager_->Emit(
        transform_.translation_,
        count_,
        transform_.scale_, // スケールを引数として渡す
        velocityMin_,      // 最小速度を引数として渡す
        velocityMax_,      // 最大速度を引数として渡す
        lifeTimeMin_,      // 最小ライフタイムを引数として渡す
        lifeTimeMax_,      // 最大ライフタイムを引数として渡す
        startScale_,
        endScale_,
        startAcce_,
        endAcce_,
        startRote_,
        endRote_,
        isRandomColor_,
        alphaMin_,
        alphaMax_,
        rotateVelocityMin_,
        rotateVelocityMax_,
        allScaleMax_,
        allScaleMin_,
        scaleMin_,
        scaleMax_,
        transform_.rotation_,
        rotateStartMax_,
        rotateStartMin_);
}

#pragma region ImGui関連

void ParticleEmitter::SaveToJson() {

    datas_->Save("emitterTranslation", transform_.translation_);
    datas_->Save("emitterRotation", transform_.rotation_);
    datas_->Save("emitterScale", transform_.scale_);
    datas_->Save("count", count_);
    datas_->Save("emitFrequency", emitFrequency_);
    datas_->Save("lifeTimeMin", lifeTimeMin_);
    datas_->Save("lifeTimeMax", lifeTimeMax_);
    datas_->Save("alphaMin", alphaMin_);
    datas_->Save("alphaMax", alphaMax_);
    datas_->Save("scaleMin", scaleMin_);
    datas_->Save("scaleMax", scaleMax_);
    datas_->Save("velocityMin", velocityMin_);
    datas_->Save("velocityMax", velocityMax_);
    datas_->Save("startScale", startScale_);
    datas_->Save("endScale", endScale_);
    datas_->Save("startAcce", startAcce_);
    datas_->Save("endAcce", endAcce_);
    datas_->Save("startRote", startRote_);
    datas_->Save("endRote", endRote_);
    datas_->Save("rotateStartMax", rotateStartMax_);
    datas_->Save("rotateStartMin", rotateStartMin_);
    datas_->Save("rotateVelocityMin", rotateVelocityMin_);
    datas_->Save("rotateVelocityMax", rotateVelocityMax_);
    datas_->Save("allScaleMin", allScaleMin_);
    datas_->Save("allScaleMax", allScaleMax_);
    datas_->Save("isRandomScale", isRandomScale_);
    datas_->Save("isAllRamdomScale", isAllRamdomScale_);
    datas_->Save("isRandomColor", isRandomColor_);
    datas_->Save("isRandomRotate", isRandomRotate_);
    datas_->Save("isVisible", isVisible_);
    datas_->Save("isBillBoard", isBillBoard_);
    datas_->Save("isActive", isActive_);
    datas_->Save("isAcceMultiply", isAcceMultiply_);
    datas_->Save("isSinMove", isSinMove_);
    datas_->Save("isFaceDirection", isFaceDirection_);
    datas_->Save("isEndScale", isEndScale_);
    datas_->Save("isEmitOnEdge", isEmitOnEdge_);
    datas_->Save("isGatherMode", isGatherMode_);
    datas_->Save("gatherStartRatio", gatherStartRatio_);
    datas_->Save("gatherStrength", gatherStrength_);
    particleGroupNames_ = Manager_->GetParticleGroupsName();
    int count = 0;
    for (auto &particleGroupName : particleGroupNames_) {
        datas_->Save("GroupName_" + std::to_string(count), particleGroupName);
        count++;
    }
}

void ParticleEmitter::LoadFromJson() {
    if (!name_.empty()) {
        transform_.translation_ = datas_->Load<Vector3>("emitterTranslation", {0.0f, 0.0f, 0.0f});
        transform_.rotation_ = datas_->Load<Vector3>("emitterRotation", {0.0f, 0.0f, 0.0f});
        transform_.scale_ = datas_->Load<Vector3>("emitterScale", {1.0f, 1.0f, 1.0f});
        count_ = datas_->Load<int>("count", 1);
        emitFrequency_ = datas_->Load<float>("emitFrequency", 0.1f);
        lifeTimeMin_ = datas_->Load<float>("lifeTimeMin", 1.0f);
        lifeTimeMax_ = datas_->Load<float>("lifeTimeMax", 3.0f);
        alphaMin_ = datas_->Load<float>("alphaMin", 1.0f);
        alphaMax_ = datas_->Load<float>("alphaMax", 1.0f);
        scaleMin_ = datas_->Load<float>("scaleMin", 1.0f);
        scaleMax_ = datas_->Load<float>("scaleMax", 1.0f);
        velocityMin_ = datas_->Load<Vector3>("velocityMin", {-1.0f, -1.0f, -1.0f});
        velocityMax_ = datas_->Load<Vector3>("velocityMax", {1.0f, 1.0f, 1.0f});
        startScale_ = datas_->Load<Vector3>("startScale", {1.0f, 1.0f, 1.0f});
        endScale_ = datas_->Load<Vector3>("endScale", {0.0f, 0.0f, 0.0f});
        startAcce_ = datas_->Load<Vector3>("startAcce", {1.0f, 1.0f, 1.0f});
        endAcce_ = datas_->Load<Vector3>("endAcce", {1.0f, 1.0f, 1.0f});
        startRote_ = datas_->Load<Vector3>("startRote", {0.0f, 0.0f, 0.0f});
        endRote_ = datas_->Load<Vector3>("endRote", {0.0f, 0.0f, 0.0f});
        rotateStartMax_ = datas_->Load<Vector3>("rotateStartMax", {0.0f, 0.0f, 0.0f});
        rotateStartMin_ = datas_->Load<Vector3>("rotateStartMin", {0.0f, 0.0f, 0.0f});
        rotateVelocityMin_ = datas_->Load<Vector3>("rotateVelocityMin", {-0.07f, -0.07f, -0.07f});
        rotateVelocityMax_ = datas_->Load<Vector3>("rotateVelocityMax", {0.07f, 0.07f, 0.07f});
        allScaleMin_ = datas_->Load<Vector3>("allScaleMin", {0.0f, 0.0f, 0.0f});
        allScaleMax_ = datas_->Load<Vector3>("allScaleMax", {1.0f, 1.0f, 1.0f});
        isRandomScale_ = datas_->Load<bool>("isRandomScale", false);
        isAllRamdomScale_ = datas_->Load<bool>("isAllRamdomScale", false);
        isRandomColor_ = datas_->Load<bool>("isRandomColor", false);
        isRandomRotate_ = datas_->Load<bool>("isRandomRotate", false);
        isVisible_ = datas_->Load<bool>("isVisible", true);
        isBillBoard_ = datas_->Load<bool>("isBillBoard", false);
        isActive_ = datas_->Load<bool>("isActive", false);
        isAcceMultiply_ = datas_->Load<bool>("isAcceMultiply", false);
        isSinMove_ = datas_->Load<bool>("isSinMove", false);
        isFaceDirection_ = datas_->Load<bool>("isFaceDirection", false);
        isEndScale_ = datas_->Load<bool>("isEndScale", false);
        isEmitOnEdge_ = datas_->Load<bool>("isEmitOnEdge", false);
        isGatherMode_ = datas_->Load<bool>("isGatherMode", false);
        gatherStartRatio_ = datas_->Load<float>("gatherStartRatio", 0.0f);
        gatherStrength_ = datas_->Load<float>("gatherStrength", 0.0f);
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
                    ImGui::DragFloat("最大値", &lifeTimeMax_, 0.1f, 0.0f);
                    ImGui::DragFloat("最小値", &lifeTimeMin_, 0.1f, 0.0f);
                    lifeTimeMin_ = std::clamp(lifeTimeMin_, 0.0f, lifeTimeMax_);
                    lifeTimeMax_ = std::clamp(lifeTimeMax_, lifeTimeMin_, 10.0f);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                if (ImGui::TreeNode("位置")) {
                    ImGui::Checkbox("中心に集める", &isGatherMode_);
                    if (isGatherMode_) {
                        ImGui::DragFloat("強さ", &gatherStrength_,0.1f);
                        ImGui::DragFloat("始まるタイミング", &gatherStartRatio_,0.1f);
                    }
                    ImGui::Checkbox("外周", &isEmitOnEdge_);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 速度と加速度
                if (ImGui::TreeNode("速度、加速度")) {
                    ImGui::Text("速度:");
                    ImGui::DragFloat3("最大値", &velocityMax_.x, 0.1f);
                    ImGui::DragFloat3("最小値", &velocityMin_.x, 0.1f);
                    velocityMin_.x = std::clamp(velocityMin_.x, -FLT_MAX, velocityMax_.x);
                    velocityMax_.x = std::clamp(velocityMax_.x, velocityMin_.x, FLT_MAX);
                    velocityMin_.y = std::clamp(velocityMin_.y, -FLT_MAX, velocityMax_.y);
                    velocityMax_.y = std::clamp(velocityMax_.y, velocityMin_.y, FLT_MAX);
                    velocityMin_.z = std::clamp(velocityMin_.z, -FLT_MAX, velocityMax_.z);
                    velocityMax_.z = std::clamp(velocityMax_.z, velocityMin_.z, FLT_MAX);
                    ImGui::Text("加速度:");
                    ImGui::DragFloat3("最初", &startAcce_.x, 0.001f);
                    ImGui::DragFloat3("最後", &endAcce_.x, 0.001f);
                    ImGui::Checkbox("乗算", &isAcceMultiply_);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // サイズ
                if (ImGui::TreeNode("大きさ")) {
                    ImGui::Text("大きさ:");
                    if (isAllRamdomScale_) {
                        ImGui::Checkbox("最初と最後同じ大きさ", &isEndScale_);
                        ImGui::DragFloat3("最大値", &allScaleMax_.x, 0.1f, 0.0f);
                        ImGui::DragFloat3("最小値", &allScaleMin_.x, 0.1f, 0.0f);
                        allScaleMin_.x = std::clamp(allScaleMin_.x, -FLT_MAX, allScaleMax_.x);
                        allScaleMax_.x = std::clamp(allScaleMax_.x, allScaleMin_.x, FLT_MAX);
                        allScaleMin_.y = std::clamp(allScaleMin_.y, -FLT_MAX, allScaleMax_.y);
                        allScaleMax_.y = std::clamp(allScaleMax_.y, allScaleMin_.y, FLT_MAX);
                        allScaleMin_.z = std::clamp(allScaleMin_.z, -FLT_MAX, allScaleMax_.z);
                        allScaleMax_.z = std::clamp(allScaleMax_.z, allScaleMin_.z, FLT_MAX);
                        if (!isEndScale_) {
                            ImGui::DragFloat3("最後", &endScale_.x, 0.1f);
                        }
                    } else if (isRandomScale_) {
                        ImGui::DragFloat("最大値", &scaleMax_, 0.1f, 0.0f);
                        ImGui::DragFloat("最小値", &scaleMin_, 0.1f, 0.0f);
                        scaleMax_ = std::clamp(scaleMax_, scaleMin_, FLT_MAX);
                        scaleMin_ = std::clamp(scaleMin_, 0.0f, scaleMax_);
                    } else if (isSinMove_) {
                        ImGui::DragFloat3("最初", &startScale_.x, 0.1f, 0.0f);
                    } else {
                        ImGui::DragFloat3("最初", &startScale_.x, 0.1f, 0.0f);
                        ImGui::DragFloat3("最後", &endScale_.x, 0.1f);
                    }
                    ImGui::Checkbox("均等にランダムな大きさ", &isRandomScale_);
                    ImGui::Checkbox("ばらばらにランダムな大きさ", &isAllRamdomScale_);
                    ImGui::Checkbox("sin波の動き", &isSinMove_);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 回転
                if (ImGui::TreeNode("回転")) {
                    if (!isRandomRotate_) {
                        float startRotationDegrees[3] = {
                            radiansToDegrees(startRote_.x),
                            radiansToDegrees(startRote_.y),
                            radiansToDegrees(startRote_.z)};
                        float endRotationDegrees[3] = {
                            radiansToDegrees(endRote_.x),
                            radiansToDegrees(endRote_.y),
                            radiansToDegrees(endRote_.z)};
                        if (ImGui::DragFloat3("最初", startRotationDegrees, 0.1f)) {
                            startRote_.x = degreesToRadians(startRotationDegrees[0]);
                            startRote_.y = degreesToRadians(startRotationDegrees[1]);
                            startRote_.z = degreesToRadians(startRotationDegrees[2]);
                        }
                        if (ImGui::DragFloat3("最後", endRotationDegrees, 0.1f)) {
                            endRote_.x = degreesToRadians(endRotationDegrees[0]);
                            endRote_.y = degreesToRadians(endRotationDegrees[1]);
                            endRote_.z = degreesToRadians(endRotationDegrees[2]);
                        }
                    }
                    if (isRandomRotate_) {
                        float startRotationDegrees[3] = {
                            radiansToDegrees(rotateStartMax_.x),
                            radiansToDegrees(rotateStartMax_.y),
                            radiansToDegrees(rotateStartMax_.z)};
                        float endRotationDegrees[3] = {
                            radiansToDegrees(rotateStartMin_.x),
                            radiansToDegrees(rotateStartMin_.y),
                            radiansToDegrees(rotateStartMin_.z)};

                        if (ImGui::DragFloat3("回転 最大値", startRotationDegrees, 0.1f)) {
                            rotateStartMax_.x = degreesToRadians(std::clamp(startRotationDegrees[0], radiansToDegrees(rotateStartMin_.x), 180.0f));
                            rotateStartMax_.y = degreesToRadians(std::clamp(startRotationDegrees[1], radiansToDegrees(rotateStartMin_.y), 180.0f));
                            rotateStartMax_.z = degreesToRadians(std::clamp(startRotationDegrees[2], radiansToDegrees(rotateStartMin_.z), 180.0f));
                        }

                        if (ImGui::DragFloat3("回転 最小値", endRotationDegrees, 0.1f)) {
                            rotateStartMin_.x = degreesToRadians(std::clamp(endRotationDegrees[0], -180.0f, radiansToDegrees(rotateStartMax_.x)));
                            rotateStartMin_.y = degreesToRadians(std::clamp(endRotationDegrees[1], -180.0f, radiansToDegrees(rotateStartMax_.y)));
                            rotateStartMin_.z = degreesToRadians(std::clamp(endRotationDegrees[2], -180.0f, radiansToDegrees(rotateStartMax_.z)));
                        }

                        ImGui::Checkbox("ランダムな回転速度", &isRotateVelocity_);

                        if (isRotateVelocity_) {
                            ImGui::DragFloat3("最大値", &rotateVelocityMax_.x, 0.01f);
                            ImGui::DragFloat3("最小値", &rotateVelocityMin_.x, 0.01f);
                            rotateVelocityMin_.x = std::clamp(rotateVelocityMin_.x, -FLT_MAX, rotateVelocityMax_.x);
                            rotateVelocityMax_.x = std::clamp(rotateVelocityMax_.x, rotateVelocityMin_.x, FLT_MAX);
                            rotateVelocityMin_.y = std::clamp(rotateVelocityMin_.y, -FLT_MAX, rotateVelocityMax_.y);
                            rotateVelocityMax_.y = std::clamp(rotateVelocityMax_.y, rotateVelocityMin_.y, FLT_MAX);
                            rotateVelocityMin_.z = std::clamp(rotateVelocityMin_.z, -FLT_MAX, rotateVelocityMax_.z);
                            rotateVelocityMax_.z = std::clamp(rotateVelocityMax_.z, rotateVelocityMin_.z, FLT_MAX);
                        }
                    }
                    ImGui::Checkbox("ランダムな回転", &isRandomRotate_);
                    ImGui::Checkbox("進行方向に向ける", &isFaceDirection_);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // Alphaを折りたたみ可能にする
                if (ImGui::TreeNode("透明度")) {
                    ImGui::Text("透明度の設定:");
                    ImGui::DragFloat("最大値", &alphaMax_, 0.01f, 0.0f, 1.0f);
                    ImGui::DragFloat("最小値", &alphaMin_, 0.01f, 0.0f, 1.0f);
                    alphaMin_ = std::clamp(alphaMin_, 0.0f, alphaMax_);
                    alphaMax_ = std::clamp(alphaMax_, alphaMin_, 1.0f);
                    ImGui::TreePop();
                }
            }

            // エミット設定セクション
            if (ImGui::CollapsingHeader("パーティクルの数、間隔")) {
                ImGui::DragFloat("間隔", &emitFrequency_, 0.001f, 0.001f, 100.0f);
                ImGui::InputInt("数", &count_, 1, 100);
                count_ = std::clamp(count_, 0, 10000);
            }

            // その他の設定セクション
            if (ImGui::CollapsingHeader("各状態の設定")) {
                ImGui::Checkbox("ビルボード", &isBillBoard_);
                ImGui::Checkbox("ランダムカラー", &isRandomColor_);
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
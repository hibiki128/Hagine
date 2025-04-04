#include "ParticleEmitter.h"
#include "line/DrawLine3D.h"
#include "myEngine/Frame/Frame.h"
// コンストラクタ
ParticleEmitter::ParticleEmitter() {}

void ParticleEmitter::Initialize(std::string name, std::string filePath) {
    transform_.Initialize();
    if (!name.empty()) {
        name_ = name;
        datas_ = std::make_unique<DataHandler>("Particle", name_);
        LoadFromJson();
        if (!filePath.empty()) {
            fileName_ = filePath;
        }
        Manager_ = std::make_unique<ParticleManager>();
        Manager_->Initialize(SrvManager::GetInstance());
        Manager_->CreateParticleGroup(name_, fileName_);
        if (texturePath_.empty()) {
            texturePath_ = Manager_->GetTexturePath();
        }
        Manager_->SetTexture(texturePath_);
    }
}

// Update関数
void ParticleEmitter::Update() {
    // 経過時間を進める
    elapsedTime_ += Frame::DeltaTime();

    // 発生頻度に基づいてパーティクルを発生させる
    while (elapsedTime_ >= emitFrequency_) {
        Manager_->SetRandomRotate(isRandomRotate_);
        Manager_->SetAcceMultipy(isAcceMultiply_);
        Manager_->SetBillBorad(isBillBoard_);
        Manager_->SetRandomSize(isRandomScale_);
        Manager_->SetAllRandomSize(isAllRamdomScale_);
        Manager_->SetSinMove(isSinMove_);
        Manager_->SetFaceDirection(isFaceDirection_);
        Emit();                         // パーティクルを発生させる
        elapsedTime_ -= emitFrequency_; // 過剰に進んだ時間を考慮
    }
}

void ParticleEmitter::UpdateOnce() {
    isActive_ = false;
    if (!isActive_) {
        Manager_->SetRandomRotate(isRandomRotate_);
        Manager_->SetAcceMultipy(isAcceMultiply_);
        Manager_->SetBillBorad(isBillBoard_);
        Manager_->SetRandomSize(isRandomScale_);
        Manager_->SetAllRandomSize(isAllRamdomScale_);
        Manager_->SetSinMove(isSinMove_);
        Manager_->SetFaceDirection(isFaceDirection_);
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

void ParticleEmitter::SetTexture(const std::string &filePath) {
    texturePath_ = filePath;
    Manager_->SetTexture(filePath);
}

// Emit関数
void ParticleEmitter::Emit() {
    // ParticleManagerのEmit関数を呼び出す
    Manager_->Emit(
        name_,
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
        transform_.rotation_);
}

void ParticleEmitter::CreateParticle(const std::string &name, const std::string &fileName, const std::string &texturePath) {

    name_ = name;
    fileName_ = fileName;
    Manager_ = std::make_unique<ParticleManager>();
    Manager_->Initialize(SrvManager::GetInstance());
    Manager_->CreateParticleGroup(name_, fileName_);
    if (texturePath_.empty()) {
        texturePath_ = Manager_->GetTexturePath();
    }
    texturePath_ = texturePath;
    Manager_->SetTexture(texturePath_);
    datas_ = std::make_unique<DataHandler>("Particle", name_);
    LoadFromJson();
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
    datas_->Save("fileName", fileName_);
    datas_->Save("texturePath", texturePath_);
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
        fileName_ = datas_->Load<std::string>("fileName", fileName_);
        texturePath_ = datas_->Load<std::string>("texturePath", texturePath_);
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
                        ImGui::DragFloat3("最大値", &allScaleMax_.x, 0.1f, 0.0f);
                        ImGui::DragFloat3("最小値", &allScaleMin_.x, 0.1f, 0.0f);
                        allScaleMin_.x = std::clamp(allScaleMin_.x, -FLT_MAX, allScaleMax_.x);
                        allScaleMax_.x = std::clamp(allScaleMax_.x, allScaleMin_.x, FLT_MAX);
                        allScaleMin_.y = std::clamp(allScaleMin_.y, -FLT_MAX, allScaleMax_.y);
                        allScaleMax_.y = std::clamp(allScaleMax_.y, allScaleMin_.y, FLT_MAX);
                        allScaleMin_.z = std::clamp(allScaleMin_.z, -FLT_MAX, allScaleMax_.z);
                        allScaleMax_.z = std::clamp(allScaleMax_.z, allScaleMin_.z, FLT_MAX);
                    } else if (isRandomScale_) {
                        ImGui::DragFloat("最大値", &scaleMax_, 0.1f, 0.0f);
                        ImGui::DragFloat("最小値", &scaleMin_, 0.1f, 0.0f);
                        scaleMax_ = std::clamp(scaleMax_, scaleMin_, FLT_MAX);
                        scaleMin_ = std::clamp(scaleMin_, 0.0f, scaleMax_);
                    } else if (isSinMove_) {
                        ImGui::DragFloat3("最初", &startScale_.x, 0.1f, 0.0f);
                    } else {
                        ImGui::DragFloat3("最初", &startScale_.x, 0.1f, 0.0f);
                    }
                    if (!isSinMove_) {
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
                        ImGui::DragFloat3("最大値", &rotateVelocityMax_.x, 0.01f);
                        ImGui::DragFloat3("最小値", &rotateVelocityMin_.x, 0.01f);
                        rotateVelocityMin_.x = std::clamp(rotateVelocityMin_.x, -FLT_MAX, rotateVelocityMax_.x);
                        rotateVelocityMax_.x = std::clamp(rotateVelocityMax_.x, rotateVelocityMin_.x, FLT_MAX);
                        rotateVelocityMin_.y = std::clamp(rotateVelocityMin_.y, -FLT_MAX, rotateVelocityMax_.y);
                        rotateVelocityMax_.y = std::clamp(rotateVelocityMax_.y, rotateVelocityMin_.y, FLT_MAX);
                        rotateVelocityMin_.z = std::clamp(rotateVelocityMin_.z, -FLT_MAX, rotateVelocityMax_.z);
                        rotateVelocityMax_.z = std::clamp(rotateVelocityMax_.z, rotateVelocityMin_.z, FLT_MAX);
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
    if (ImGui::Begin("パーティクルデータ")) {
        if (!name_.empty() && Manager_) {
            DebugParticleData();
        }
        ImGui::End();
    }
#endif
}
#pragma endregion
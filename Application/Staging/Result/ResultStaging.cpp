#include "ResultStaging.h"
#include "Particle/CSParticle/ParticleCSEditor.h"
#include "Edit/MotionEditor/MotionEditor.h"
#include <Frame.h>
#include <Line/DrawLine3D.h>
#include <Object/Base/BaseObjectManager.h>
#include <random>

using namespace Hagine;
void ResultStaging::Initialize()
{

    /// ===================================================
    /// ポインタ共有
    /// ===================================================

    RightHand_ = BaseObjectManager::GetInstance()->GetObjectByName("sphere_1");
    LeftHand_ = BaseObjectManager::GetInstance()->GetObjectByName("sphere_2");

    fireWorkStates_.resize(fireWorks_count_);

    for (int i = 0; i < fireWorks_count_; ++i)
    {
        fireWorks_explosions_.push_back(ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("fireWork_explosion"));
        fireWorks_trails_.push_back(ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("fireWork_Trail"));

        fireWorks_explosions_[i]->SetAuto(false);
        fireWorks_trails_[i]->SetAuto(false);

        fireWorkStates_[i].phase = FireWorkState::Phase::Ready;
        fireWorkStates_[i].timer = 0.0f;
    }

    /// ===================================================
    /// 登録
    /// ===================================================
    MotionEditor::GetInstance()->Register(RightHand_);
    MotionEditor::GetInstance()->Register(LeftHand_);
}

void ResultStaging::Update()
{

    // イージング開始時のモーション制御
    if (startEasing_)
    {
        if (!secondMove_ && !motionStarted_)
        {
            // 最初のパンチモーション
            MotionEditor::GetInstance()->PlayFromFile(LeftHand_, "LeftPunch");
            MotionEditor::GetInstance()->PlayFromFile(RightHand_, "RightBack");
            motionStarted_ = true;
        }
        if (!secondMove_ &&
            MotionEditor::GetInstance()->IsAttackFinished(LeftHand_) &&
            MotionEditor::GetInstance()->IsAttackFinished(RightHand_))
        {
            // 2つ目のパンチモーション
            secondMove_ = true;
            MotionEditor::GetInstance()->PlayFromFile(LeftHand_, "LeftBack");
            MotionEditor::GetInstance()->PlayFromFile(RightHand_, "RightPunch");
        }
    }

    // デバッグ用エリア描画
    DrawFireWorkArea();

    // 全パーティクルの更新
    for (int i = 0; i < fireWorks_count_; i++)
    {
        fireWorks_explosions_[i]->Update();
        fireWorks_trails_[i]->Update();
    }

    // 花火の打ち上げ制御
    if (fireWorkStarted_)
    {
        float deltaTime = Frame::DeltaTime();

        // 新しい花火の発射タイマー
        nextFireWorkTimer_ -= deltaTime;
        if (nextFireWorkTimer_ <= 0.0f)
        {
            int availableIndex = FindAvailableFireWork();
            if (availableIndex != -1)
            {
                // 新しい花火を発射
                FireWorkState &state = fireWorkStates_[availableIndex];
                state.phase = FireWorkState::Phase::Rising;
                state.timer = 0.0f;
                state.startPosition = GetRandomPositionInArea();
                state.explodePosition = state.startPosition + Vector3(0.0f, kFireWorkHeightOffset, 0.0f);

                // 花火軌跡を発生
                fireWorks_trails_[availableIndex]->SetTranslate(state.startPosition);
                fireWorks_trails_[availableIndex]->EmitOnce();

                // 次の発射間隔をランダムに設定
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> dist(minFireWorkInterval_, maxFireWorkInterval_);
                nextFireWorkTimer_ = dist(gen);
            }
        }

        // 各花火の状態更新
        for (int i = 0; i < (int)fireWorkStates_.size(); ++i)
        {
            FireWorkState &state = fireWorkStates_[i];

            switch (state.phase)
            {
            case FireWorkState::Phase::Rising:
                // 上昇中
                state.timer += deltaTime;

                if (state.timer >= kFireWorkRisingTime)
                {
                    // 爆発フェーズへ
                    state.phase = FireWorkState::Phase::Exploding;
                    state.timer = 0.0f;

                    // 花火爆発を発生
                    fireWorks_explosions_[i]->SetTranslate(state.explodePosition);
                    fireWorks_explosions_[i]->EmitOnce();
                }
                break;

            case FireWorkState::Phase::Exploding:
                // 爆発中
                state.timer += deltaTime;

                if (state.timer >= kFireWorkExplodingTime)
                {
                    // 待機状態に戻す
                    state.phase = FireWorkState::Phase::Ready;
                    state.timer = 0.0f;
                }
                break;

            case FireWorkState::Phase::Ready:
                // 何もしない
                break;
            }
        }
    }
}

void ResultStaging::Draw(const ViewProjection &viewProjection)
{

    for (int i = 0; i < fireWorks_count_; i++)
    {
        fireWorks_explosions_[i]->Draw(viewProjection);
        fireWorks_trails_[i]->Draw(viewProjection);
    }
}

Vector3 ResultStaging::GetRandomPositionInArea()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(-kRandomPositionRange, kRandomPositionRange);
    std::uniform_real_distribution<float> distY(-kRandomPositionRange, kRandomPositionRange);
    std::uniform_real_distribution<float> distZ(-kRandomPositionRange, kRandomPositionRange);

    // ローカル空間でランダムな位置を生成
    Vector3 localPos(
        distX(gen) * fireWorkAreaSize_.x,
        distY(gen) * fireWorkAreaSize_.y,
        distZ(gen) * fireWorkAreaSize_.z);

    // 回転を適用
    Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(fireWorkAreaRotation_);
    Vector3 rotatedPos = Transformation(localPos, rotationMatrix);

    // 中心座標を加算してワールド空間に変換
    return fireWorkAreaCenter_ + rotatedPos;
}

int ResultStaging::FindAvailableFireWork()
{
    for (int i = 0; i < fireWorkStates_.size(); ++i)
    {
        if (fireWorkStates_[i].phase == FireWorkState::Phase::Ready)
        {
            return i;
        }
    }
    return -1; // 利用可能な花火がない
}

void ResultStaging::DrawFireWorkArea()
{
    // 立方体の8つの頂点をローカル座標で定義
    Vector3 halfSize = fireWorkAreaSize_ * kAreaHalfSizeMultiplier;
    Vector3 vertices[8] = {
        {-halfSize.x, -halfSize.y, -halfSize.z}, // 0: 左下奥
        {halfSize.x, -halfSize.y, -halfSize.z},  // 1: 右下奥
        {halfSize.x, halfSize.y, -halfSize.z},   // 2: 右上奥
        {-halfSize.x, halfSize.y, -halfSize.z},  // 3: 左上奥
        {-halfSize.x, -halfSize.y, halfSize.z},  // 4: 左下手前
        {halfSize.x, -halfSize.y, halfSize.z},   // 5: 右下手前
        {halfSize.x, halfSize.y, halfSize.z},    // 6: 右上手前
        {-halfSize.x, halfSize.y, halfSize.z}    // 7: 左上手前
    };

    // 回転と平行移動を適用
    Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(fireWorkAreaRotation_);
    Vector3 worldVertices[8];
    for (int i = 0; i < 8; ++i)
    {
        worldVertices[i] = Transformation(vertices[i], rotationMatrix) + fireWorkAreaCenter_;
    }

    // 線の色
    Vector4 lineColor = {kLineColorR, kLineColorG, kLineColorB, kLineColorA}; // オレンジ色

    // 底面の4本
    DrawLine3D::GetInstance()->SetPoints(worldVertices[0], worldVertices[1], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[1], worldVertices[5], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[5], worldVertices[4], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[4], worldVertices[0], lineColor);

    // 上面の4本
    DrawLine3D::GetInstance()->SetPoints(worldVertices[3], worldVertices[2], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[2], worldVertices[6], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[6], worldVertices[7], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[7], worldVertices[3], lineColor);

    // 縦の4本
    DrawLine3D::GetInstance()->SetPoints(worldVertices[0], worldVertices[3], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[1], worldVertices[2], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[5], worldVertices[6], lineColor);
    DrawLine3D::GetInstance()->SetPoints(worldVertices[4], worldVertices[7], lineColor);
}

void ResultStaging::DrawImGui()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Result Staging Settings"))
    {

        ImGui::SeparatorText("花火システム");

        ImGui::Checkbox("花火開始##FireWorkStart", &fireWorkStarted_);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("花火発生エリア設定##FireWorkArea"))
        {
            ImGui::DragFloat3("エリア中心座標##AreaCenter", &fireWorkAreaCenter_.x, 0.5f);
            ImGui::DragFloat3("エリアサイズ##AreaSize", &fireWorkAreaSize_.x, 0.5f, 1.0f, 200.0f);

            Vector3 currentEuler = fireWorkAreaRotation_.ToEulerDegrees();
            ImGui::Text("現在の回転: %.1f° %.1f° %.1f°", currentEuler.x, currentEuler.y, currentEuler.z);

            static Vector3 deltaRotation = {0.0f, 0.0f, 0.0f};
            if (ImGui::DragFloat3("エリア回転##AreaRotation", &deltaRotation.x, 0.1f, -10.0f, 10.0f, "%.1f°"))
            {
                Quaternion currentRotation = fireWorkAreaRotation_;
                Quaternion deltaQuatX = Quaternion::FromAxisAngle(Vector3(1, 0, 0), deltaRotation.x * std::numbers::pi_v<float> / 180.0f);
                Quaternion deltaQuatY = Quaternion::FromAxisAngle(Vector3(0, 1, 0), deltaRotation.y * std::numbers::pi_v<float> / 180.0f);
                Quaternion deltaQuatZ = Quaternion::FromAxisAngle(Vector3(0, 0, 1), deltaRotation.z * std::numbers::pi_v<float> / 180.0f);
                Quaternion deltaQuat = deltaQuatY * deltaQuatX * deltaQuatZ;
                Quaternion newRotation = currentRotation * deltaQuat;
                fireWorkAreaRotation_ = newRotation.Normalize();
                deltaRotation = {0.0f, 0.0f, 0.0f};
            }

            ImGui::SameLine();
            if (ImGui::Button("リセット##AreaRotation"))
            {
                fireWorkAreaRotation_ = Quaternion::IdentityQuaternion();
                deltaRotation = {0.0f, 0.0f, 0.0f};
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("花火発射間隔設定##FireWorkInterval"))
        {
            ImGui::DragFloat("最小間隔 (秒)##MinInterval", &minFireWorkInterval_, 0.01f, 0.1f, 5.0f, "%.2f");
            ImGui::DragFloat("最大間隔 (秒)##MaxInterval", &maxFireWorkInterval_, 0.01f, 0.1f, 10.0f, "%.2f");

            if (minFireWorkInterval_ > maxFireWorkInterval_)
            {
                maxFireWorkInterval_ = minFireWorkInterval_;
            }

            ImGui::Text("次の花火まで: %.2f秒", nextFireWorkTimer_);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("花火ステータス##FireWorkStatus"))
        {
            int readyCount = 0;
            int risingCount = 0;
            int explodingCount = 0;

            for (const auto &state : fireWorkStates_)
            {
                switch (state.phase)
                {
                case FireWorkState::Phase::Ready:
                    readyCount++;
                    break;
                case FireWorkState::Phase::Rising:
                    risingCount++;
                    break;
                case FireWorkState::Phase::Exploding:
                    explodingCount++;
                    break;
                }
            }

            ImGui::Text("待機中: %d", readyCount);
            ImGui::Text("上昇中: %d", risingCount);
            ImGui::Text("爆発中: %d", explodingCount);
            ImGui::Text("総数: %d", (int)fireWorkStates_.size());
        }
    }
    ImGui::End();
#endif
}
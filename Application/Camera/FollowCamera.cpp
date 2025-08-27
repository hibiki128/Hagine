#include "FollowCamera.h"
#include "Easing.h"
#include "Input.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <Engine/Frame/Frame.h>
#include <cmath>

void FollowCamera::Init() {
    viewProjection_.farZ = 1100;
    viewProjection_.Initialize();
    worldTransform_.Initialize();
    yaw_ = 0.0f;                  // 水平回転角度を初期化
    distanceFromTarget_ = -25.0f; // ターゲットからの距離を初期化
    heightOffset_ = 3.0f;         // ターゲットの上方オフセット
}

void FollowCamera::Update() {
    if (!target_)
        return;

    // 基本のカメラ移動処理
    Move();

    Vector3 targetPos = target_->GetLocalPosition();
    Vector3 velocity = target_->GetVelocity();
    Vector3 cameraPos;

    Player *player = dynamic_cast<Player *>(target_);

    // Rush状態専用処理
    if (player) {
        std::string currentStateName = player->GetCurrentStateName();
        if (currentStateName == "Rush") {
            Vector3 currentPos = player->GetLocalPosition();

            if (player->GetIsLockOn() && player->GetEnemy()) {
                Vector3 enemyTargetPos = player->GetEnemy()->GetPositionBehind(3.0f);
                float distanceToTarget = (enemyTargetPos - currentPos).Length();

                if (distanceToTarget > rushCameraResumeDistance_) {
                    // Rush中の部分追従処理
                    if (!isRushCameraActive_) {
                        isRushCameraActive_ = true;
                        rushCameraPosition_ = worldTransform_.translation_;
                        rushCameraRotation_ = worldTransform_.quateRotation_;
                    }

                    // 目標カメラ位置
                    Vector3 targetCameraPos = currentPos + rushCameraOffset_;

                    // プレイヤーとの距離によって追従率を調整
                    Vector3 playerToCameraDir = worldTransform_.translation_ - currentPos;
                    float playerCameraDistance = playerToCameraDir.Length();
                    float dynamicFollowRate = rushCameraFollowRate_;
                    if (playerCameraDistance > 35.0f) {
                        dynamicFollowRate = std::min(1.0f, rushCameraFollowRate_ * 3.0f);
                    } else if (playerCameraDistance > 25.0f) {
                        dynamicFollowRate = rushCameraFollowRate_ * 2.0f;
                    }

                    // カメラ位置補間
                    Vector3 blendedPos = Lerp(rushCameraPosition_, targetCameraPos,
                                              dynamicFollowRate * Frame::DeltaTime());
                    worldTransform_.translation_ = blendedPos;
                    rushCameraPosition_ = blendedPos;

                    // カメラ回転：敵＋プレイヤー中間方向
                    Vector3 toEnemy = (player->GetEnemy()->GetLocalPosition() - worldTransform_.translation_).Normalize();
                    Vector3 toPlayer = (currentPos - worldTransform_.translation_).Normalize();
                    Vector3 blendedDir = Lerp(toEnemy, toPlayer, 0.3f).Normalize(); // 0.3はプレイヤー寄り

                    Vector3 forward = blendedDir;
                    Vector3 worldUp = {0.0f, 1.0f, 0.0f};

                    Vector3 right;
                    if (std::abs(forward.Dot(worldUp)) > 0.999f) {
                        right = {1.0f, 0.0f, 0.0f};
                    } else {
                        right = (worldUp.Cross(forward)).Normalize();
                    }
                    Vector3 up = (forward.Cross(right)).Normalize();
                    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
                    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

                    worldTransform_.quateRotation_ = Quaternion::Slerp(
                        rushCameraRotation_, targetRot,
                        rushCameraFollowRate_ * Frame::DeltaTime() * 0.5f);
                    rushCameraRotation_ = worldTransform_.quateRotation_;

                    worldTransform_.UpdateMatrix();
                    viewProjection_.translation_ = worldTransform_.translation_;
                    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
                    viewProjection_.matWorld_ = worldTransform_.matWorld_;
                    viewProjection_.UpdateMatrix();
                    return;
                } else {
                    // 追従再開準備
                    if (!isResumeFromRush_) {
                        isResumeFromRush_ = true;
                        rushCameraPosition_ = worldTransform_.translation_;
                        rushCameraRotation_ = worldTransform_.quateRotation_;
                    }
                    isRushCameraActive_ = false;
                }
            }
        } else {
            // Rush終了時のフラグリセット
            if (isResumeFromRush_)
                isResumeFromRush_ = false;
            isRushCameraActive_ = false;
        }
    }

    // 肩オフセット計算
    if (player && player->GetIsLockOn() && player->GetEnemy()) {
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;

        Vector3 toEnemyDirXZ = {toEnemyDir.x, 0.0f, toEnemyDir.z};
        float lengthXZ = toEnemyDirXZ.Length();
        if (lengthXZ > 0.001f)
            toEnemyDirXZ = toEnemyDirXZ.Normalize();
        yaw_ = std::atan2(toEnemyDirXZ.x, toEnemyDirXZ.z);

        Vector3 cameraRightDir = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};
        float lateralVelocity = velocity.x * cameraRightDir.x + velocity.z * cameraRightDir.z;

        if (std::abs(lateralVelocity) > 0.1f) {
            float dirSign = std::clamp(lateralVelocity / target_->GetMaxSpeed(), -1.0f, 1.0f);
            shoulderOffsetTarget_.x = -dirSign * shoulderMaxOffset_;
        } else {
            shoulderOffsetTarget_.x = shoulderMaxOffset_;
        }
    } else {
        shoulderOffsetTarget_.x = 0.0f;
    }

    Vector3 cameraRightDir = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};
    shoulderOffsetCurrent_.x = Lerp(shoulderOffsetCurrent_.x, shoulderOffsetTarget_.x, shoulderLerpSpeed_ * Frame::DeltaTime());

    // カメラ位置計算
    if (player && player->GetIsLockOn() && player->GetEnemy()) {
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;
        float length = toEnemyDir.Length();
        if (length > 0.001f)
            toEnemyDir = toEnemyDir.Normalize();

        cameraPos = targetPos - toEnemyDir * std::abs(cameraOffset_.z);

        Vector3 forward = toEnemyDir;
        Vector3 worldUp = {0.0f, 1.0f, 0.0f};
        Vector3 right;
        if (std::abs(forward.Dot(worldUp)) > 0.999f) {
            right = {1.0f, 0.0f, 0.0f};
        } else {
            right = (worldUp.Cross(forward)).Normalize();
        }
        Vector3 up = (forward.Cross(right)).Normalize();
        Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
        worldTransform_.quateRotation_ = Quaternion::FromMatrix(rotMatrix);
    } else {
        cameraPos.x = targetPos.x + std::sin(yaw_) * cameraOffset_.z;
        cameraPos.z = targetPos.z + std::cos(yaw_) * cameraOffset_.z;
        cameraPos.y = targetPos.y + cameraOffset_.y;
        worldTransform_.quateRotation_ = Quaternion::FromEulerAngles({0.0f, -yaw_, 0.0f});
    }

    cameraPos += cameraRightDir * shoulderOffsetCurrent_.x;

    // Rush復帰補間
    if (isResumeFromRush_) {
        Vector3 targetCameraPos = cameraPos;
        Quaternion targetCameraRot = worldTransform_.quateRotation_;
        worldTransform_.translation_ = Lerp(rushCameraPosition_, targetCameraPos, rushResumeBlendSpeed_ * Frame::DeltaTime());
        worldTransform_.quateRotation_ = Quaternion::Slerp(rushCameraRotation_, targetCameraRot, rushResumeBlendSpeed_ * Frame::DeltaTime());

        float positionDiff = (worldTransform_.translation_ - targetCameraPos).Length();
        float rotationDiff = std::abs(1.0f - std::abs(worldTransform_.quateRotation_.Dot(targetCameraRot)));
        if (positionDiff < 0.5f && rotationDiff < 0.01f)
            isResumeFromRush_ = false;

        rushCameraPosition_ = worldTransform_.translation_;
        rushCameraRotation_ = worldTransform_.quateRotation_;
    } else {
        worldTransform_.translation_ = cameraPos;
    }

    worldTransform_.UpdateMatrix();

    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
    viewProjection_.matWorld_ = worldTransform_.matWorld_;
    viewProjection_.UpdateMatrix();
}

void FollowCamera::imgui() {
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat3("wt position", &worldTransform_.translation_.x, 0.1f);
    ImGui::DragFloat3("vp position", &viewProjection_.translation_.x, 0.1f);
    ImGui::DragFloat3("offsetCurrent", &shoulderOffsetCurrent_.x, 0.1f);
    ImGui::DragFloat3("offsetTarget", &shoulderOffsetTarget_.x, 0.1f);
    ImGui::End();
}

void FollowCamera::Move() {
    Player *player = dynamic_cast<Player *>(target_);
    if (!player || !player->GetIsLockOn()) {
        if (Input::GetInstance()->PushKey(DIK_LEFT)) {
            yaw_ -= 0.04f;
        }
        if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
            yaw_ += 0.04f;
        }
    }
}
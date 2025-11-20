#include "FollowCamera.h"
#include "Easing.h"
#include "Input.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <Engine/Frame/Frame.h>
#include <cmath>

void FollowCamera::Init() {
    viewProjection_.farZ = 1100;
    viewProjection_.Initialize("");
    worldTransform_.Initialize();
    yaw_ = 0.0f;
    shoulderOffsetTarget_ = {0.0f, 0.0f, 0.0f};
    shoulderOffsetCurrent_ = {0.0f, 0.0f, 0.0f};
    wasLockedOn_ = false;
    isResettingShoulderOffset_ = false;
    shoulderResetTimer_ = 0.0f;
    shoulderLerpTimer_ = 0.0f;
    shoulderLerpStartValue_ = 0.0f;

    // 高さオフセットの初期化
    lockOnHeightOffsetCurrent_ = lockOnGroundedHeight_;
    lockOnHeightOffsetTarget_ = lockOnGroundedHeight_;
}

void FollowCamera::Update() {
    if (!target_)
        return;

    Move();

    Vector3 targetPos = target_->GetLocalPosition();
    Vector3 velocity = target_->GetVelocity();
    Vector3 cameraPos;

    Player *player = dynamic_cast<Player *>(target_);

    // ロックオン状態の変化を検出
    bool isCurrentlyLockedOn = player && player->GetIsLockOn() && player->GetEnemy();

    // ロックオンが外れた瞬間を検出
    if (wasLockedOn_ && !isCurrentlyLockedOn) {
        // 肩オフセットのリセットを開始
        isResettingShoulderOffset_ = true;
        shoulderResetTimer_ = 0.0f;
        shoulderOffsetStart_ = shoulderOffsetCurrent_;
        shoulderOffsetTarget_ = {0.0f, 0.0f, 0.0f};
    }

    // 現在のロックオン状態を保存
    wasLockedOn_ = isCurrentlyLockedOn;

    // Rush状態専用処理
    if (player) {
        std::string currentStateName = player->GetCurrentStateName();
        if (currentStateName == "Rush") {
            Vector3 currentPos = player->GetLocalPosition();

            if (player->GetIsLockOn() && player->GetEnemy()) {
                Vector3 enemyTargetPos = player->GetEnemy()->GetPositionBehind(3.0f);
                float distanceToTarget = (enemyTargetPos - currentPos).Length();

                if (distanceToTarget > rushCameraResumeDistance_) {
                    if (!isRushCameraActive_) {
                        isRushCameraActive_ = true;
                        rushCameraPosition_ = worldTransform_.translation_;
                        rushCameraRotation_ = worldTransform_.quateRotation_;
                    }

                    Vector3 targetCameraPos = currentPos + rushCameraOffset_;

                    Vector3 playerToCameraDir = worldTransform_.translation_ - currentPos;
                    float playerCameraDistance = playerToCameraDir.Length();
                    float dynamicFollowRate = rushCameraFollowRate_;
                    if (playerCameraDistance > 35.0f) {
                        dynamicFollowRate = std::min(1.0f, rushCameraFollowRate_ * 3.0f);
                    } else if (playerCameraDistance > 25.0f) {
                        dynamicFollowRate = rushCameraFollowRate_ * 2.0f;
                    }

                    float deltaTime = Frame::DeltaTime();
                    rushBlendTimer_ += deltaTime;
                    float t = std::min(rushBlendTimer_ / (1.0f / dynamicFollowRate), 1.0f);
                    Vector3 blendedPos = ApplyEasing(rushCameraEasingType_, rushCameraPosition_, targetCameraPos, t, 1.0f);
                    worldTransform_.translation_ = blendedPos;
                    rushCameraPosition_ = blendedPos;

                    Vector3 toEnemy = (player->GetEnemy()->GetLocalPosition() - worldTransform_.translation_).Normalize();
                    Vector3 toPlayer = (currentPos - worldTransform_.translation_).Normalize();
                    Vector3 blendedDir = ApplyEasing(rushCameraEasingType_, toEnemy, toPlayer, 0.3f, 1.0f).Normalize();

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

                    rushRotationTimer_ += deltaTime;
                    float rotT = std::min(rushRotationTimer_ / (1.0f / (dynamicFollowRate * 0.5f)), 1.0f);
                    worldTransform_.quateRotation_ = Quaternion::Slerp(
                        rushCameraRotation_, targetRot,
                        ApplyEasing(rushCameraEasingType_, 0.0f, 1.0f, rotT, 1.0f));
                    rushCameraRotation_ = worldTransform_.quateRotation_;

                    worldTransform_.UpdateMatrix();

                    viewProjection_.translation_ = worldTransform_.translation_;
                    viewProjection_.isUseQuaternion_ = true;
                    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
                    viewProjection_.UpdateMatrix();
                    return;
                } else {
                    if (!isResumeFromRush_) {
                        isResumeFromRush_ = true;
                        rushResumeTimer_ = 0.0f;
                        rushCameraPosition_ = worldTransform_.translation_;
                        rushCameraRotation_ = worldTransform_.quateRotation_;
                    }
                    isRushCameraActive_ = false;
                }
            }
        } else {
            if (isResumeFromRush_)
                isResumeFromRush_ = false;
            isRushCameraActive_ = false;
            rushBlendTimer_ = 0.0f;
            rushRotationTimer_ = 0.0f;
        }
    }

    // 肩オフセット計算
    bool hasInput = false;
    if (isCurrentlyLockedOn) {
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;

        Vector3 toEnemyDirXZ = {toEnemyDir.x, 0.0f, toEnemyDir.z};
        float lengthXZ = toEnemyDirXZ.Length();
        if (lengthXZ > 0.001f)
            toEnemyDirXZ = toEnemyDirXZ.Normalize();
        yaw_ = std::atan2(toEnemyDirXZ.x, toEnemyDirXZ.z);

        Vector3 cameraRightDir = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};
        float lateralVelocity = velocity.x * cameraRightDir.x + velocity.z * cameraRightDir.z;

        hasInput = Input::GetInstance()->PushKey(DIK_W) ||
                   Input::GetInstance()->PushKey(DIK_A) ||
                   Input::GetInstance()->PushKey(DIK_S) ||
                   Input::GetInstance()->PushKey(DIK_D);

        // 入力がある場合のみターゲットを更新
        if (hasInput && std::abs(lateralVelocity) > 0.1f) {
            float dirSign = std::clamp(lateralVelocity / target_->GetMaxSpeed(), -1.0f, 1.0f);
            float newTarget = -dirSign * shoulderMaxOffset_;

            // ターゲットが大きく変わった場合のみリセット
            if (std::abs(newTarget - shoulderOffsetTarget_.x) > 0.1f) {
                shoulderLerpTimer_ = 0.0f;
                shoulderLerpStartValue_ = shoulderOffsetCurrent_.x;
            }

            shoulderOffsetTarget_.x = newTarget;
            isResettingShoulderOffset_ = false;
        }

        // ロックオン時の高さオフセット更新
        if (player->GetIsGrounded()) {
            lockOnHeightOffsetTarget_ = lockOnGroundedHeight_;
        } else {
            lockOnHeightOffsetTarget_ = lockOnAirborneHeight_;
        }

        // 高さオフセットの補間
        float deltaTime = Frame::DeltaTime();
        float heightDiff = lockOnHeightOffsetTarget_ - lockOnHeightOffsetCurrent_;
        lockOnHeightOffsetCurrent_ += heightDiff * lockOnHeightLerpSpeed_ * deltaTime;

        // 微小な差は切り捨て
        if (std::abs(heightDiff) < 0.01f) {
            lockOnHeightOffsetCurrent_ = lockOnHeightOffsetTarget_;
        }
    }

    // 肩オフセットの補間処理
    Vector3 cameraRightDir = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};

    if (isResettingShoulderOffset_) {
        // リセット中はイージングを使用
        shoulderResetTimer_ += Frame::DeltaTime();
        float t = std::min(shoulderResetTimer_ / shoulderResetDuration_, 1.0f);
        shoulderOffsetCurrent_ = ApplyEasing(shoulderResetEasingType_, shoulderOffsetStart_, shoulderOffsetTarget_, t, 1.0f);

        if (t >= 1.0f) {
            isResettingShoulderOffset_ = false;
        }
    } else {
        // 常にスムーズに追従(イージングなしの単純な補間)
        float deltaTime = Frame::DeltaTime();
        float lerpSpeed = shoulderLerpSpeed_ * deltaTime;

        float diff = shoulderOffsetTarget_.x - shoulderOffsetCurrent_.x;
        if (std::abs(diff) < 0.01f) {
            shoulderOffsetCurrent_.x = shoulderOffsetTarget_.x;
        } else {
            // OutQuadイージングを使用した追従
            float t = std::min(lerpSpeed, 1.0f);
            shoulderOffsetCurrent_.x += diff * ApplyEasing(shoulderEasingType_, 0.0f, 1.0f, t, 1.0f);
        }
    }

    // カメラ位置計算
    if (isCurrentlyLockedOn) {
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;
        float length = toEnemyDir.Length();
        if (length > 0.001f)
            toEnemyDir = toEnemyDir.Normalize();

        cameraPos = targetPos - toEnemyDir * std::abs(cameraOffset_.z);

        // ロックオン時の高さオフセットを適用
        cameraPos.y += lockOnHeightOffsetCurrent_;

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

        rushResumeTimer_ += Frame::DeltaTime();
        float t = std::min(rushResumeTimer_ / (1.0f / rushResumeBlendSpeed_), 1.0f);

        worldTransform_.translation_ = ApplyEasing(rushResumeEasingType_, rushCameraPosition_, targetCameraPos, t, 1.0f);
        worldTransform_.quateRotation_ = Quaternion::Slerp(
            rushCameraRotation_, targetCameraRot,
            ApplyEasing(rushResumeEasingType_, 0.0f, 1.0f, t, 1.0f));

        float positionDiff = (worldTransform_.translation_ - targetCameraPos).Length();
        float rotationDiff = std::abs(1.0f - std::abs(worldTransform_.quateRotation_.Dot(targetCameraRot)));
        if (positionDiff < 0.5f && rotationDiff < 0.01f) {
            isResumeFromRush_ = false;
            rushResumeTimer_ = 0.0f;
        }

        rushCameraPosition_ = worldTransform_.translation_;
        rushCameraRotation_ = worldTransform_.quateRotation_;
    } else {
        worldTransform_.translation_ = cameraPos;
    }

    worldTransform_.UpdateMatrix();

    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.isUseQuaternion_ = true;
    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
    viewProjection_.UpdateMatrix();
}

void FollowCamera::imgui() {
#ifdef USE_IMGUI
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat3("wt position", &worldTransform_.translation_.x, 0.1f);
    ImGui::DragFloat3("vp position", &viewProjection_.translation_.x, 0.1f);
    ImGui::DragFloat3("offsetCurrent", &shoulderOffsetCurrent_.x, 0.1f);
    ImGui::DragFloat3("offsetTarget", &shoulderOffsetTarget_.x, 0.1f);

    // 高さオフセット設定UI
    ImGui::Separator();
    ImGui::Text("Lock-On Height Offset");
    ImGui::DragFloat("Current Height Offset", &lockOnHeightOffsetCurrent_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Target Height Offset", &lockOnHeightOffsetTarget_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Grounded Height", &lockOnGroundedHeight_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Airborne Height", &lockOnAirborneHeight_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Height Lerp Speed", &lockOnHeightLerpSpeed_, 0.1f, 0.1f, 20.0f);

    // イージングタイプの選択UI
    ImGui::Separator();
    ImGui::Text("Easing Types");

    const char *easingTypes[] = {
        "Linear", "InSine", "OutSine", "InOutSine",
        "InQuad", "OutQuad", "InOutQuad",
        "InCubic", "OutCubic", "InOutCubic",
        "InQuart", "OutQuart", "InOutQuart",
        "InQuint", "OutQuint", "InOutQuint",
        "InCirc", "OutCirc", "InOutCirc",
        "InExpo", "OutExpo", "InOutExpo",
        "InBack", "OutBack", "InOutBack",
        "InElastic", "OutElastic", "InOutElastic",
        "InBounce", "OutBounce", "InOutBounce"};

    int shoulderEasing = static_cast<int>(shoulderEasingType_);
    int shoulderResetEasing = static_cast<int>(shoulderResetEasingType_);
    int rushCameraEasing = static_cast<int>(rushCameraEasingType_);
    int rushResumeEasing = static_cast<int>(rushResumeEasingType_);

    if (ImGui::Combo("Shoulder Easing", &shoulderEasing, easingTypes, IM_ARRAYSIZE(easingTypes))) {
        shoulderEasingType_ = static_cast<EasingType>(shoulderEasing);
    }
    if (ImGui::Combo("Shoulder Reset Easing", &shoulderResetEasing, easingTypes, IM_ARRAYSIZE(easingTypes))) {
        shoulderResetEasingType_ = static_cast<EasingType>(shoulderResetEasing);
    }
    if (ImGui::Combo("Rush Camera Easing", &rushCameraEasing, easingTypes, IM_ARRAYSIZE(easingTypes))) {
        rushCameraEasingType_ = static_cast<EasingType>(rushCameraEasing);
    }
    if (ImGui::Combo("Rush Resume Easing", &rushResumeEasing, easingTypes, IM_ARRAYSIZE(easingTypes))) {
        rushResumeEasingType_ = static_cast<EasingType>(rushResumeEasing);
    }

    ImGui::DragFloat("Shoulder Reset Duration", &shoulderResetDuration_, 0.01f, 0.1f, 2.0f);

    ImGui::End();
#endif
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
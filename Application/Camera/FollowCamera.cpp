#include "FollowCamera.h"
#include "Easing.h"
#include "Input.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <Engine/Frame/Frame.h>
#include <cmath>

void FollowCamera::Init() {
    viewProjection_.farZ = kFarZ;
    viewProjection_.Initialize("");
    worldTransform_.Initialize();
    yaw_ = kInitialYaw;
    shoulderOffsetTarget_ = {kVectorZero, kVectorZero, kVectorZero};
    shoulderOffsetCurrent_ = {kVectorZero, kVectorZero, kVectorZero};
    wasLockedOn_ = false;
    isResettingShoulderOffset_ = false;
    shoulderResetTimer_ = kTimerReset;
    shoulderLerpTimer_ = kTimerReset;
    shoulderLerpStartValue_ = kTimerReset;

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
        shoulderResetTimer_ = kTimerReset;
        shoulderOffsetStart_ = shoulderOffsetCurrent_;
        shoulderOffsetTarget_ = {kVectorZero, kVectorZero, kVectorZero};
    }

    // 現在のロックオン状態を保存
    wasLockedOn_ = isCurrentlyLockedOn;

    // Rush状態専用処理
    if (player) {
        std::string currentStateName = player->GetCurrentStateName();
        if (currentStateName == "Rush") {
            Vector3 currentPos = player->GetLocalPosition();

            if (player->GetIsLockOn() && player->GetEnemy()) {
                Vector3 enemyTargetPos = player->GetEnemy()->GetPositionBehind(rushEnemyBehindOffset_);
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
                    if (playerCameraDistance > rushHighDistThreshold_) {
                        dynamicFollowRate = std::min(kMaxFollowRate, rushCameraFollowRate_ * kHighDistSpeedMultiplier);
                    } else if (playerCameraDistance > rushMidDistThreshold_) {
                        dynamicFollowRate = rushCameraFollowRate_ * kMidDistSpeedMultiplier;
                    }

                    float deltaTime = Frame::DeltaTime();
                    rushBlendTimer_ += deltaTime;
                    float t = std::min(rushBlendTimer_ / (kNormalizedValue / dynamicFollowRate), kMaxBlendValue);
                    Vector3 blendedPos = ApplyEasing(rushCameraEasingType_, rushCameraPosition_, targetCameraPos, t, kEasingMaxValue);
                    worldTransform_.translation_ = blendedPos;
                    rushCameraPosition_ = blendedPos;

                    Vector3 toEnemy = (player->GetEnemy()->GetLocalPosition() - worldTransform_.translation_).Normalize();
                    Vector3 toPlayer = (currentPos - worldTransform_.translation_).Normalize();
                    Vector3 blendedDir = ApplyEasing(rushCameraEasingType_, toEnemy, toPlayer, kRushDirectionBlendRatio, kEasingMaxValue).Normalize();

                    Vector3 forward = blendedDir;
                    Vector3 worldUp = {kVectorZero, kUpVectorY, kVectorZero};

                    Vector3 right;
                    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
                        right = {kRightVectorX, kVectorZero, kVectorZero};
                    } else {
                        right = (worldUp.Cross(forward)).Normalize();
                    }
                    Vector3 up = (forward.Cross(right)).Normalize();
                    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
                    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

                    rushRotationTimer_ += deltaTime;
                    float rotT = std::min(rushRotationTimer_ / (kNormalizedValue / (dynamicFollowRate * kRotationSpeedMultiplier)), kMaxBlendValue);
                    worldTransform_.quateRotation_ = Quaternion::Slerp(
                        rushCameraRotation_, targetRot,
                        ApplyEasing(rushCameraEasingType_, kVectorZero, kEasingMaxValue, rotT, kEasingMaxValue));
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
                        rushResumeTimer_ = kTimerReset;
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
            rushBlendTimer_ = kTimerReset;
            rushRotationTimer_ = kTimerReset;
        }
    }

    // 肩オフセット計算
    bool hasInput = false;
    if (isCurrentlyLockedOn) {
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;

        Vector3 toEnemyDirXZ = {toEnemyDir.x, kVectorZero, toEnemyDir.z};
        float lengthXZ = toEnemyDirXZ.Length();
        if (lengthXZ > kEpsilon)
            toEnemyDirXZ = toEnemyDirXZ.Normalize();
        yaw_ = std::atan2(toEnemyDirXZ.x, toEnemyDirXZ.z);

        Vector3 cameraRightDir = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};
        float lateralVelocity = velocity.x * cameraRightDir.x + velocity.z * cameraRightDir.z;

        if (!player->GetGamePad()->IsConnected()) {
            // キーボード入力
            hasInput = Input::GetInstance()->PushKey(DIK_W) ||
                       Input::GetInstance()->PushKey(DIK_A) ||
                       Input::GetInstance()->PushKey(DIK_S) ||
                       Input::GetInstance()->PushKey(DIK_D);
        } else {
            // ゲームパッド入力 - 左スティック
            float leftStickX = player->GetGamePad()->GetLeftStickX();
            float leftStickY = player->GetGamePad()->GetLeftStickY();

            // スティックの入力があるかチェック
            hasInput = (leftStickX != 0.0f || leftStickY != 0.0f);
        }

        // 入力がある場合のみターゲットを更新
        if (hasInput && std::abs(lateralVelocity) > kVelocityThreshold) {
            float dirSign = std::clamp(lateralVelocity / target_->GetMaxSpeed(), kMinClamp, kMaxClamp);
            float newTarget = -dirSign * shoulderMaxOffset_;

            // ターゲットが大きく変わった場合のみリセット
            if (std::abs(newTarget - shoulderOffsetTarget_.x) > kShoulderTargetThreshold) {
                shoulderLerpTimer_ = kTimerReset;
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
        if (std::abs(heightDiff) < kHeightDiffThreshold) {
            lockOnHeightOffsetCurrent_ = lockOnHeightOffsetTarget_;
        }
    }

    // 肩オフセットの補間処理
    Vector3 cameraRightDir = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};

    if (isResettingShoulderOffset_) {
        // リセット中はイージングを使用
        shoulderResetTimer_ += Frame::DeltaTime();
        float t = std::min(shoulderResetTimer_ / shoulderResetDuration_, kMaxBlendValue);
        shoulderOffsetCurrent_ = ApplyEasing(shoulderResetEasingType_, shoulderOffsetStart_, shoulderOffsetTarget_, t, kEasingMaxValue);

        if (t >= kMaxBlendValue) {
            isResettingShoulderOffset_ = false;
        }
    } else {
        // 常にスムーズに追従(イージングなしの単純な補間)
        float deltaTime = Frame::DeltaTime();
        float lerpSpeed = shoulderLerpSpeed_ * deltaTime;

        float diff = shoulderOffsetTarget_.x - shoulderOffsetCurrent_.x;
        if (std::abs(diff) < kShoulderDiffThreshold) {
            shoulderOffsetCurrent_.x = shoulderOffsetTarget_.x;
        } else {
            // OutQuadイージングを使用した追従
            float t = std::min(lerpSpeed, kMaxBlendValue);
            shoulderOffsetCurrent_.x += diff * ApplyEasing(shoulderEasingType_, kVectorZero, kEasingMaxValue, t, kEasingMaxValue);
        }
    }

    // カメラ位置計算
    if (isCurrentlyLockedOn) {
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;
        float length = toEnemyDir.Length();
        if (length > kEpsilon)
            toEnemyDir = toEnemyDir.Normalize();

        cameraPos = targetPos - toEnemyDir * std::abs(cameraOffset_.z);

        // ロックオン時の高さオフセットを適用
        cameraPos.y += lockOnHeightOffsetCurrent_;

        Vector3 forward = toEnemyDir;
        Vector3 worldUp = {kVectorZero, kUpVectorY, kVectorZero};
        Vector3 right;
        if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
            right = {kRightVectorX, kVectorZero, kVectorZero};
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
        worldTransform_.quateRotation_ = Quaternion::FromEulerAngles({kVectorZero, -yaw_, kVectorZero});
    }

    cameraPos += cameraRightDir * shoulderOffsetCurrent_.x;

    // Rush復帰補間
    if (isResumeFromRush_) {
        Vector3 targetCameraPos = cameraPos;
        Quaternion targetCameraRot = worldTransform_.quateRotation_;

        rushResumeTimer_ += Frame::DeltaTime();
        float t = std::min(rushResumeTimer_ / (kNormalizedValue / rushResumeBlendSpeed_), kMaxBlendValue);

        worldTransform_.translation_ = ApplyEasing(rushResumeEasingType_, rushCameraPosition_, targetCameraPos, t, kEasingMaxValue);
        worldTransform_.quateRotation_ = Quaternion::Slerp(
            rushCameraRotation_, targetCameraRot,
            ApplyEasing(rushResumeEasingType_, kVectorZero, kEasingMaxValue, t, kEasingMaxValue));

        float positionDiff = (worldTransform_.translation_ - targetCameraPos).Length();
        float rotationDiff = std::abs(kNormalizedValue - std::abs(worldTransform_.quateRotation_.Dot(targetCameraRot)));
        if (positionDiff < rushPosArrivalThreshold_ && rotationDiff < rushRotationArrivalThreshold_) {
            isResumeFromRush_ = false;
        }

        rushCameraPosition_ = worldTransform_.translation_;
        rushCameraRotation_ = worldTransform_.quateRotation_;
    } else {
        worldTransform_.translation_ = cameraPos;
    }

    // 地面の高さ
    static constexpr float kGroundY = 0.0f;
    // カメラを地面から浮かせる最小高さ
    static constexpr float kMinCameraHeight = 0.5f;

    // カメラが地面より下に行かないように制限
    if (worldTransform_.translation_.y < kGroundY + kMinCameraHeight) {
        worldTransform_.translation_.y = kGroundY + kMinCameraHeight;
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
    GamePad *gamePad = player->GetGamePad();
    if (player && player->GetIsLockOn()) {
        return;
    }
    if (!gamePad->IsConnected()) {
        Input *input = Input::GetInstance();
        // キーボード
        if (input->PushKey(DIK_LEFT)) {
            yaw_ -= manualYawSpeed_;
        }
        if (input->PushKey(DIK_RIGHT)) {
            yaw_ += manualYawSpeed_;
        }
    } else {
        // ゲームパッド - 右スティック
        if (player) {
            const float stickSensitivity = 0.05f;
            yaw_ += gamePad->GetRightStickX() * stickSensitivity;
        }
    }
}
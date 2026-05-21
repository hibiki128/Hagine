#include "FollowCamera.h"
#include "Easing.h"
#include "Input.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <Engine/3d/Line/DrawLine3D.h>
#include <Engine/Frame/Frame.h>
#include <cmath>

void FollowCamera::Init() {
    // ViewProjectionの初期設定
    viewProjection_.farZ = kFarZ;
    viewProjection_.Initialize("");
    worldTransform_.Initialize();
    yaw_ = kInitialYaw;

    // 肩オフセットの初期化
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
    // ターゲットがいなければ更新しない
    if (!target_)
        return;

    // カメラの移動処理
    Move();

    Vector3 targetPos = target_->GetLocalPosition();
    Vector3 velocity = target_->GetVelocity();
    Vector3 cameraPos;

    Player *player = dynamic_cast<Player *>(target_);

    // ロックオン状態の取得
    bool isCurrentlyLockedOn = player && player->GetIsLockOn() && player->GetEnemy();

    // ロックオンが外れた瞬間に肩オフセットのリセットを開始
    if (wasLockedOn_ && !isCurrentlyLockedOn) {
        isResettingShoulderOffset_ = true;
        shoulderResetTimer_ = kTimerReset;
        shoulderOffsetStart_ = shoulderOffsetCurrent_;
        shoulderOffsetTarget_ = {kVectorZero, kVectorZero, kVectorZero};
    } else if (!wasLockedOn_ && isCurrentlyLockedOn) {
        // ロックオン開始時は肩オフセットを設定
        shoulderOffsetTarget_.x = shoulderMaxOffset_;
    }

    // ロックオン状態を保存
    wasLockedOn_ = isCurrentlyLockedOn;

    // Rush状態専用のカメラ制御
    if (player) {
        std::string currentStateName = player->GetCurrentStateName();
        if (currentStateName == "Rush") {
            Vector3 currentPos = player->GetLocalPosition();

            if (player->GetIsLockOn() && player->GetEnemy()) {
                Vector3 enemyTargetPos = player->GetEnemy()->GetPositionBehind(rushEnemyBehindOffset_);
                float distanceToTarget = (enemyTargetPos - currentPos).Length();

                // 一定距離以上離れている場合は特殊な追従を行う
                if (distanceToTarget > rushCameraResumeDistance_) {
                    if (!isRushCameraActive_) {
                        isRushCameraActive_ = true;
                        rushCameraPosition_ = worldTransform_.translation_;
                        rushCameraRotation_ = worldTransform_.quateRotation_;
                    }

                    Vector3 targetCameraPos = currentPos + rushCameraOffset_;

                    // 距離に応じて追従速度を調整
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
                    
                    // 位置の補間
                    Vector3 blendedPos = ApplyEasing(rushCameraEasingType_, rushCameraPosition_, targetCameraPos, t, kEasingMaxValue);
                    worldTransform_.translation_ = blendedPos;
                    rushCameraPosition_ = blendedPos;

                    // 敵とプレイヤーの間を向くように回転を計算
                    Vector3 toEnemy = (player->GetEnemy()->GetLocalPosition() - worldTransform_.translation_).Normalize();
                    Vector3 toPlayer = (currentPos - worldTransform_.translation_).Normalize();
                    Vector3 blendedDir = ApplyEasing(rushCameraEasingType_, toEnemy, toPlayer, kRushDirectionBlendRatio, kEasingMaxValue).Normalize();

                    Vector3 forward = blendedDir;
                    Vector3 worldUp = {kVectorZero, kUpVectorY, kVectorZero};

                    Vector3 right;
                    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
                        right = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};
                    } else {
                        right = (worldUp.Cross(forward)).Normalize();
                    }
                    Vector3 up = (forward.Cross(right)).Normalize();
                    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
                    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

                    // 回転の補間
                    rushRotationTimer_ += deltaTime;
                    float rotT = std::min(rushRotationTimer_ / (kNormalizedValue / (dynamicFollowRate * kRotationSpeedMultiplier)), kMaxBlendValue);
                    worldTransform_.quateRotation_ = Quaternion::Slerp(
                        rushCameraRotation_, targetRot,
                        ApplyEasing(rushCameraEasingType_, kVectorZero, kEasingMaxValue, rotT, kEasingMaxValue));
                    rushCameraRotation_ = worldTransform_.quateRotation_;

                    worldTransform_.UpdateMatrix();

                    // ViewProjectionの更新
                    viewProjection_.translation_ = worldTransform_.translation_;
                    viewProjection_.isUseQuaternion_ = true;
                    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
                    viewProjection_.UpdateMatrix();
                    return;
                } else {
                    // 通常追従への復帰準備
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
            // Rush状態でなければフラグリセット
            if (isResumeFromRush_)
                isResumeFromRush_ = false;
            isRushCameraActive_ = false;
            rushBlendTimer_ = kTimerReset;
            rushRotationTimer_ = kTimerReset;
        }
    }

    // ロックオン時の肩オフセット計算
    bool hasInput = false;
    if (isCurrentlyLockedOn) {
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;

        Vector3 toEnemyDirXZ = {toEnemyDir.x, kVectorZero, toEnemyDir.z};
        float lengthXZ = toEnemyDirXZ.Length();

        // 敵の方向からyaw角を更新
        if (lengthXZ > kEpsilon) {
            toEnemyDirXZ = toEnemyDirXZ.Normalize();
            yaw_ = std::atan2(toEnemyDirXZ.x, toEnemyDirXZ.z);
        }

        Vector3 cameraRightDir = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};
        float lateralVelocity = velocity.x * cameraRightDir.x + velocity.z * cameraRightDir.z;

        // 入力チェック
        if (!player->GetGamePad()->IsConnected()) {
            hasInput = Input::GetInstance()->PushKey(DIK_W) ||
                       Input::GetInstance()->PushKey(DIK_A) ||
                       Input::GetInstance()->PushKey(DIK_S) ||
                       Input::GetInstance()->PushKey(DIK_D);
        } else {
            float leftStickX = player->GetGamePad()->GetLeftStickX();
            float leftStickY = player->GetGamePad()->GetLeftStickY();
            hasInput = (leftStickX != 0.0f || leftStickY != 0.0f);
        }

        // 横移動入力がある場合、カメラを左右にずらす
        if (hasInput && std::abs(lateralVelocity) > kVelocityThreshold) {
            float sign = lateralVelocity > 0.0f ? 1.0f : -1.0f;
            float newTarget = -sign * shoulderMaxOffset_;

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

        if (std::abs(heightDiff) < kHeightDiffThreshold) {
            lockOnHeightOffsetCurrent_ = lockOnHeightOffsetTarget_;
        }
    }

    // 肩オフセットの補間実行
    if (isResettingShoulderOffset_) {
        // リセットイージング
        shoulderResetTimer_ += Frame::DeltaTime();
        float t = std::min(shoulderResetTimer_ / shoulderResetDuration_, kMaxBlendValue);
        shoulderOffsetCurrent_ = ApplyEasing(shoulderResetEasingType_, shoulderOffsetStart_, shoulderOffsetTarget_, t, kEasingMaxValue);

        if (t >= kMaxBlendValue) {
            isResettingShoulderOffset_ = false;
        }
    } else {
        // 通常の追従補間
        float deltaTime = Frame::DeltaTime();
        float lerpSpeed = shoulderLerpSpeed_ * deltaTime;

        float diff = shoulderOffsetTarget_.x - shoulderOffsetCurrent_.x;
        if (std::abs(diff) < kShoulderDiffThreshold) {
            shoulderOffsetCurrent_.x = shoulderOffsetTarget_.x;
        } else {
            float t = std::min(lerpSpeed, kMaxBlendValue);
            shoulderOffsetCurrent_.x += diff * ApplyEasing(shoulderEasingType_, kVectorZero, kEasingMaxValue, t, kEasingMaxValue);
        }
    }

    // カメラの最終的な位置と回転を計算
    if (isCurrentlyLockedOn) {
        // ロックオン時：敵の方向を向く
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;
        float length = toEnemyDir.Length();
        if (length > kEpsilon)
            toEnemyDir = toEnemyDir.Normalize();

        Vector3 forward = toEnemyDir;
        Vector3 right = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};
        Vector3 up = (forward.Cross(right)).Normalize();

        Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
        worldTransform_.quateRotation_ = Quaternion::FromMatrix(rotMatrix);

        // オフセットを適用
        cameraPos = targetPos - forward * std::abs(cameraOffset_.z);
        cameraPos += up * lockOnHeightOffsetCurrent_;
        cameraPos += right * shoulderOffsetCurrent_.x;

    } else {
        // 非ロックオン時：手動回転に基づいた位置
        cameraPos.x = targetPos.x + std::sin(yaw_) * cameraOffset_.z;
        cameraPos.z = targetPos.z + std::cos(yaw_) * cameraOffset_.z;
        cameraPos.y = targetPos.y + cameraOffset_.y;
        worldTransform_.quateRotation_ = Quaternion::FromEulerAngles({kVectorZero, -yaw_, kVectorZero});

        Vector3 right = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};
        cameraPos += right * shoulderOffsetCurrent_.x;
    }

    // Rushからの復帰補間
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

    worldTransform_.UpdateMatrix();

    // 最終的な行列とViewProjectionの更新
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.isUseQuaternion_ = true;
    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
    viewProjection_.UpdateMatrix();

    // 視錐台ロックオン判定
    UpdateFrustumLockOn();
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

    // ===================================================
    // 視錐台ロックオン
    // ===================================================
    ImGui::Separator();
    ImGui::Text("【視錐台ロックオン】");

    // ロックオン状態表示
    Player *player = dynamic_cast<Player *>(target_);
    if (player) {
        if (player->GetIsLockOn()) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "ロックオン中: ON");
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ロックオン中: OFF");
        }
    }

    ImGui::Checkbox("視錐台デバッグ描画", &drawLockOnFrustumDebug_);
    if (ImGui::Button("ロックオン解除")) {
        target_->ReleaseLockOn();
    }

    // パラメータ調整（度数表示）
    const float kToDeg = 180.0f / 3.14159265f;
    const float kToRad = 3.14159265f / 180.0f;
    ImGui::DragFloat("有効距離", &lockOnRange_, 0.5f, 1.0f, 100.0f, "%.1f");
    float halfFovHDeg = lockOnHalfFovH_ * kToDeg;
    float halfFovVDeg = lockOnHalfFovV_ * kToDeg;
    if (ImGui::DragFloat("水平半角 (度)", &halfFovHDeg, 0.5f, 1.0f, 89.0f, "%.1f")) {
        lockOnHalfFovH_ = halfFovHDeg * kToRad;
    }
    if (ImGui::DragFloat("垂直半角 (度)", &halfFovVDeg, 0.5f, 1.0f, 89.0f, "%.1f")) {
        lockOnHalfFovV_ = halfFovVDeg * kToRad;
    }
    ImGui::Text("  水平全角: %.1f°  垂直全角: %.1f°", halfFovHDeg * 2.0f, halfFovVDeg * 2.0f);

    // 敵との距離確認
    if (player && player->GetEnemy()) {
        ImGui::Separator();
        const float dist = (player->GetEnemy()->GetPosition() - worldTransform_.translation_).Length();
        ImGui::Text("カメラ-敵 距離: %.2f / %.1f", dist, lockOnRange_);
    }

    ImGui::End();
#endif
}

void FollowCamera::DrawFrustum() {
    // 視錐台デバッグ描画
    if (drawLockOnFrustumDebug_) {
        DrawLockOnFrustum(DrawLine3D::GetInstance());
    }
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

// ============================================================
// 視錐台ロックオン（カメラ基準）
// ============================================================

bool FollowCamera::IsPointInLockOnFrustum(const Vector3 &point) const {
    // カメラ位置を原点に
    const Vector3 origin = worldTransform_.translation_;
    Vector3 toTarget = point - origin;

    // 距離チェック
    float distance = toTarget.Length();
    if (distance < kEpsilon || distance > lockOnRange_) {
        return false;
    }

    // カメラのローカル軸をクォータニオンから取得
    // worldTransform_ はワールド行列のため、Z列がそのままカメラの前方向になる
    Matrix4x4 rotMat = QuaternionToMatrix4x4(worldTransform_.quateRotation_);
    const Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
    const Vector3 right = {rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2]};
    const Vector3 up = {rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2]};

    // 前方成分（負 = カメラの背後）
    float dotF = toTarget.Dot(forward);
    if (dotF <= kVectorZero) {
        return false;
    }

    // 水平角判定
    float tanH = toTarget.Dot(right) / dotF;
    if (std::abs(tanH) > std::tan(lockOnHalfFovH_)) {
        return false;
    }

    // 垂直角判定
    float tanV = toTarget.Dot(up) / dotF;
    if (std::abs(tanV) > std::tan(lockOnHalfFovV_)) {
        return false;
    }

    return true;
}

void FollowCamera::UpdateFrustumLockOn() {
    Player *player = dynamic_cast<Player *>(target_);
    if (!player) {
        return;
    }

    Enemy *enemy = player->GetEnemy();
    if (!enemy) {
        return;
    }

    // 既にロックオン済みなら再判定しない
    if (player->GetIsLockOn()) {
        return;
    }

    // 敵の位置が視錐台内に入っているか判定
    if (IsPointInLockOnFrustum(enemy->GetPosition())) {
        player->SetIsLockOn(true);
    }
}

void FollowCamera::DrawLockOnFrustum(DrawLine3D *drawLine3D) const {
    if (!drawLine3D) {
        return;
    }

    Player *player = dynamic_cast<Player *>(target_);
    const bool isLockedOn = player && player->GetIsLockOn();

    const Vector3 origin = worldTransform_.translation_;

    // カメラローカル軸
    // worldTransform_ はワールド行列のため、Z列がそのままカメラの前方向になる
    Matrix4x4 rotMat = QuaternionToMatrix4x4(worldTransform_.quateRotation_);
    const Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
    const Vector3 right = {rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2]};
    const Vector3 up = {rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2]};

    const Vector4 color = isLockedOn ? Vector4{1.0f, 0.0f, 0.0f, 1.0f}
                                     : Vector4{1.0f, 1.0f, 1.0f, 0.8f};
    const Vector4 axisColor = isLockedOn ? Vector4{1.0f, 0.0f, 0.0f, 0.5f}
                                         : Vector4{1.0f, 1.0f, 1.0f, 0.4f};

    // Near / Far 面コーナー計算
    auto CalcCorners = [&](float depth, std::array<Vector3, 4> &corners) {
        float halfH = depth * std::tan(lockOnHalfFovH_);
        float halfV = depth * std::tan(lockOnHalfFovV_);
        Vector3 center = origin + forward * depth;
        corners[0] = center - right * halfH + up * halfV; // 左上
        corners[1] = center + right * halfH + up * halfV; // 右上
        corners[2] = center + right * halfH - up * halfV; // 右下
        corners[3] = center - right * halfH - up * halfV; // 左下
    };

    std::array<Vector3, 4> nearCorners, farCorners;
    CalcCorners(kFrustumDebugNear, nearCorners);
    CalcCorners(lockOnRange_, farCorners);

    for (int i = 0; i < 4; ++i) {
        drawLine3D->SetPoints(nearCorners[i], nearCorners[(i + 1) % 4], color);
    }
    for (int i = 0; i < 4; ++i) {
        drawLine3D->SetPoints(farCorners[i], farCorners[(i + 1) % 4], color);
    }
    for (int i = 0; i < 4; ++i) {
        drawLine3D->SetPoints(nearCorners[i], farCorners[i], color);
    }

    drawLine3D->SetPoints(origin, origin + forward * lockOnRange_, axisColor);
}
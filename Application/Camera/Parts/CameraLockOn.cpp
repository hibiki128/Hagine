#define NOMINMAX
#include "CameraLockOn.h"
#include "Easing.h"
#include "Input.h"
#include "application/Camera/FollowCamera.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <3d/Line/DrawLine3D.h>
#include <Frame/Frame.h>
#include <algorithm>
#include <array>
#include <cmath>

using namespace Hagine;

void CameraLockOn::Init(FollowCamera *owner) {
    owner_ = owner;

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

void CameraLockOn::UpdateLockOnTransition(bool isCurrentlyLockedOn) {
    if (wasLockedOn_ && !isCurrentlyLockedOn) {
        // ロックオン解除：肩オフセットを中央へ戻す演出を開始
        isResettingShoulderOffset_ = true;
        shoulderResetTimer_ = kTimerReset;
        shoulderOffsetStart_ = shoulderOffsetCurrent_;
        shoulderOffsetTarget_ = {kVectorZero, kVectorZero, kVectorZero};
    } else if (!wasLockedOn_ && isCurrentlyLockedOn) {
        // ロックオン開始：肩を片側へ寄せる
        shoulderOffsetTarget_.x = shoulderMaxOffset_;
    }

    // 前フレームの状態を保存
    wasLockedOn_ = isCurrentlyLockedOn;
}

void CameraLockOn::UpdateLockOnShoulderAndHeight(Player *player, const Vector3 &targetPos, const Vector3 &velocity) {
    Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
    Vector3 toEnemyDir = enemyPos - targetPos;

    Vector3 toEnemyDirXZ = {toEnemyDir.x, kVectorZero, toEnemyDir.z};
    float lengthXZ = toEnemyDirXZ.Length();

    // 敵の方向に基づいてヨー角を更新
    float yaw = owner_->GetYaw();
    if (lengthXZ > kEpsilon) {
        toEnemyDirXZ = toEnemyDirXZ.Normalize();
        yaw = std::atan2(toEnemyDirXZ.x, toEnemyDirXZ.z);
        owner_->SetYaw(yaw);
    }

    Vector3 cameraRightDir = {std::cos(yaw), kVectorZero, -std::sin(yaw)};
    float lateralVelocity = velocity.x * cameraRightDir.x + velocity.z * cameraRightDir.z;

    // 入力の確認
    bool hasInput = false;
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

    // 入力方向に応じた肩オフセットの切り替え
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

    // 接地状態に応じた高さオフセットの目標値設定
    if (player->GetIsGrounded()) {
        lockOnHeightOffsetTarget_ = lockOnGroundedHeight_;
    } else {
        lockOnHeightOffsetTarget_ = lockOnAirborneHeight_;
    }

    // 高さの補間実行
    float deltaTime = Frame::DeltaTime();
    float heightDiff = lockOnHeightOffsetTarget_ - lockOnHeightOffsetCurrent_;
    lockOnHeightOffsetCurrent_ += heightDiff * lockOnHeightLerpSpeed_ * deltaTime;

    if (std::abs(heightDiff) < kHeightDiffThreshold) {
        lockOnHeightOffsetCurrent_ = lockOnHeightOffsetTarget_;
    }
}

void CameraLockOn::UpdateShoulderOffset() {
    if (isResettingShoulderOffset_) {
        // 解除時の戻り演出
        shoulderResetTimer_ += Frame::DeltaTime();
        float t = std::min(shoulderResetTimer_ / shoulderResetDuration_, kMaxBlendValue);
        shoulderOffsetCurrent_ = ApplyEasing(shoulderResetEasingType_, shoulderOffsetStart_, shoulderOffsetTarget_, t, kEasingMaxValue);

        if (t >= kMaxBlendValue) {
            isResettingShoulderOffset_ = false;
        }
    } else {
        // 通常の追従演出
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
}

bool CameraLockOn::IsPointInLockOnFrustum(const Vector3 &point) const {
    WorldTransform &worldTransform = owner_->GetCameraWorldTransform();
    const Vector3 origin = worldTransform.translation_;
    Vector3 toTarget = point - origin;

    // 距離による判定
    float distance = toTarget.Length();
    if (distance < kEpsilon || distance > lockOnRange_) {
        return false;
    }

    // カメラのローカル座標系を取得
    Matrix4x4 rotMat = QuaternionToMatrix4x4(worldTransform.quateRotation_);
    const Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
    const Vector3 right = {rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2]};
    const Vector3 up = {rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2]};

    // 前方判定（カメラの背面にあれば除外）
    float dotF = toTarget.Dot(forward);
    if (dotF <= kVectorZero) {
        return false;
    }

    // 水平角による判定
    float tanH = toTarget.Dot(right) / dotF;
    if (std::abs(tanH) > std::tan(lockOnHalfFovH_)) {
        return false;
    }

    // 垂直角による判定
    float tanV = toTarget.Dot(up) / dotF;
    if (std::abs(tanV) > std::tan(lockOnHalfFovV_)) {
        return false;
    }

    return true;
}

void CameraLockOn::UpdateFrustumLockOn() {
    Player *player = owner_->GetTarget();
    if (!player) {
        return;
    }

    Enemy *enemy = player->GetEnemy();
    if (!enemy) {
        return;
    }

    // 既にロックオンしている場合はスキップ
    if (player->GetIsLockOn()) {
        return;
    }

    // 敵が視錐台内に入った瞬間にロックオンを開始
    if (IsPointInLockOnFrustum(enemy->GetPosition())) {
        player->SetIsLockOn(true);
    }
}

void CameraLockOn::DrawFrustum() {
    // デバッグ描画が有効な場合に視錐台を描画
    if (drawLockOnFrustumDebug_) {
        DrawLockOnFrustum(DrawLine3D::GetInstance());
    }
}

void CameraLockOn::DrawLockOnFrustum(DrawLine3D *drawLine3D) const {
    if (!drawLine3D) {
        return;
    }

    Player *player = owner_->GetTarget();
    const bool isLockedOn = player && player->GetIsLockOn();

    WorldTransform &worldTransform = owner_->GetCameraWorldTransform();
    const Vector3 origin = worldTransform.translation_;

    // カメラのローカル軸情報を取得
    Matrix4x4 rotMat = QuaternionToMatrix4x4(worldTransform.quateRotation_);
    const Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
    const Vector3 right = {rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2]};
    const Vector3 up = {rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2]};

    // ロックオン状態に応じた描画色
    const Vector4 color = isLockedOn ? Vector4{1.0f, 0.0f, 0.0f, 1.0f}
                                     : Vector4{1.0f, 1.0f, 1.0f, 0.8f};
    const Vector4 axisColor = isLockedOn ? Vector4{1.0f, 0.0f, 0.0f, 0.5f}
                                         : Vector4{1.0f, 1.0f, 1.0f, 0.4f};

    // 角の座標を計算するラムダ式
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

    // 面の輪郭を描画
    for (int i = 0; i < 4; ++i) {
        drawLine3D->SetPoints(nearCorners[i], nearCorners[(i + 1) % 4], color);
    }
    for (int i = 0; i < 4; ++i) {
        drawLine3D->SetPoints(farCorners[i], farCorners[(i + 1) % 4], color);
    }
    for (int i = 0; i < 4; ++i) {
        drawLine3D->SetPoints(nearCorners[i], farCorners[i], color);
    }

    // 中心軸の描画
    drawLine3D->SetPoints(origin, origin + forward * lockOnRange_, axisColor);
}

void CameraLockOn::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::DragFloat3("offsetCurrent", &shoulderOffsetCurrent_.x, 0.1f);
    ImGui::DragFloat3("offsetTarget", &shoulderOffsetTarget_.x, 0.1f);

    // 高さオフセット関連のUI
    ImGui::Separator();
    ImGui::Text("Lock-On Height Offset");
    ImGui::DragFloat("Current Height Offset", &lockOnHeightOffsetCurrent_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Target Height Offset", &lockOnHeightOffsetTarget_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Grounded Height", &lockOnGroundedHeight_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Airborne Height", &lockOnAirborneHeight_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Height Lerp Speed", &lockOnHeightLerpSpeed_, 0.1f, 0.1f, 20.0f);

    // イージング設定のUI
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

    if (ImGui::Combo("Shoulder Easing", &shoulderEasing, easingTypes, IM_ARRAYSIZE(easingTypes))) {
        shoulderEasingType_ = static_cast<EasingType>(shoulderEasing);
    }
    if (ImGui::Combo("Shoulder Reset Easing", &shoulderResetEasing, easingTypes, IM_ARRAYSIZE(easingTypes))) {
        shoulderResetEasingType_ = static_cast<EasingType>(shoulderResetEasing);
    }

    ImGui::DragFloat("Shoulder Reset Duration", &shoulderResetDuration_, 0.01f, 0.1f, 2.0f);

    // 視錐台ロックオンのUI
    ImGui::Separator();
    ImGui::Text("【視錐台ロックオン】");

    Player *player = owner_->GetTarget();
    if (player) {
        if (player->GetIsLockOn()) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "ロックオン中: ON");
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ロックオン中: OFF");
        }
    }

    ImGui::Checkbox("視錐台デバッグ描画", &drawLockOnFrustumDebug_);
    if (ImGui::Button("ロックオン解除")) {
        if (player) {
            player->ReleaseLockOn();
        }
    }

    // 各種ロックオンパラメータの調整
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

    // 敵との距離情報の表示
    if (player && player->GetEnemy()) {
        ImGui::Separator();
        const float dist = (player->GetEnemy()->GetPosition() - owner_->GetCameraWorldTransform().translation_).Length();
        ImGui::Text("カメラ-敵 距離: %.2f / %.1f", dist, lockOnRange_);
    }
#endif // USE_IMGUI
}

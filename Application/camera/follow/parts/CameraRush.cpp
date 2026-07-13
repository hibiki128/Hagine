#define NOMINMAX
#include "CameraRush.h"
#include <Easing.h>
#include <Application/camera/follow/FollowCamera.h>
#include <Application/entity/enemy/Enemy.h>
#include <Application/entity/player/Player.h>
#include <frame/Frame.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;

bool CameraRush::UpdateRushCamera(Player *pPlayer)
{
    if (!pPlayer)
    {
        return false;
    }

    WorldTransform &worldTransform = pOwner_->GetCameraWorldTransform();

    // Rush状態でなければ各種フラグ・タイマーをリセットして通常処理へ
    if (pPlayer->GetCurrentStateName() != "Rush")
    {
        if (isResumeFromRush_)
            isResumeFromRush_ = false;
        isRushCameraActive_ = false;
        rushBlendTimer_ = kTimerReset;
        rushRotationTimer_ = kTimerReset;
        return false;
    }

    // Rush中でもロックオンしていなければ専用カメラは使わない
    if (!(pPlayer->GetIsLockOn() && pPlayer->GetEnemy()))
    {
        return false;
    }

    const Vector3 currentPos = pPlayer->GetLocalPosition();
    const Vector3 enemyTargetPos = pPlayer->GetEnemy()->GetPositionBehind(rushEnemyBehindOffset_);
    const float distanceToTarget = (enemyTargetPos - currentPos).Length();

    // ターゲットに十分近ければ通常カメラへの復帰を開始（専用追従はしない）
    if (distanceToTarget <= rushCameraResumeDistance_)
    {
        if (!isResumeFromRush_)
        {
            isResumeFromRush_ = true;
            rushResumeTimer_ = kTimerReset;
            rushCameraPosition_ = worldTransform.translation_;
            rushCameraRotation_ = worldTransform.quateRotation_;
        }
        isRushCameraActive_ = false;
        return false;
    }

    // === ターゲットから離れている：Rush専用追従でカメラを確定する ===
    if (!isRushCameraActive_)
    {
        isRushCameraActive_ = true;
        rushCameraPosition_ = worldTransform.translation_;
        rushCameraRotation_ = worldTransform.quateRotation_;
    }

    Vector3 targetCameraPos = currentPos + rushCameraOffset_;

    // 距離に基づいた追従速度の動的変更
    Vector3 playerToCameraDir = worldTransform.translation_ - currentPos;
    float playerCameraDistance = playerToCameraDir.Length();
    float dynamicFollowRate = rushCameraFollowRate_;
    if (playerCameraDistance > rushHighDistThreshold_)
    {
        dynamicFollowRate = std::min(kMaxFollowRate, rushCameraFollowRate_ * kHighDistSpeedMultiplier);
    }
    else if (playerCameraDistance > rushMidDistThreshold_)
    {
        dynamicFollowRate = rushCameraFollowRate_ * kMidDistSpeedMultiplier;
    }

    float deltaTime = Frame::DeltaTime();
    rushBlendTimer_ += deltaTime;
    float t = std::min(rushBlendTimer_ / (kNormalizedValue / dynamicFollowRate), kMaxBlendValue);

    // 位置の補間実行
    Vector3 blendedPos = ApplyEasing(rushCameraEasingType_, rushCameraPosition_, targetCameraPos, t, kEasingMaxValue);
    worldTransform.translation_ = blendedPos;
    rushCameraPosition_ = blendedPos;

    // 敵とプレイヤーの中間を注視する回転の計算
    const float yaw = pOwner_->GetYaw();
    Vector3 toEnemy = (pPlayer->GetEnemy()->GetLocalPosition() - worldTransform.translation_).Normalize();
    Vector3 toPlayer = (currentPos - worldTransform.translation_).Normalize();
    Vector3 blendedDir = ApplyEasing(rushCameraEasingType_, toEnemy, toPlayer, kRushDirectionBlendRatio, kEasingMaxValue).Normalize();

    Vector3 forward = blendedDir;
    Vector3 worldUp = {kVectorZero, kUpVectorY, kVectorZero};

    Vector3 right;
    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold)
    {
        right = {std::cos(yaw), kVectorZero, -std::sin(yaw)};
    }
    else
    {
        right = (worldUp.Cross(forward)).Normalize();
    }
    Vector3 up = (forward.Cross(right)).Normalize();
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    // 回転の補間実行
    rushRotationTimer_ += deltaTime;
    float rotT = std::min(rushRotationTimer_ / (kNormalizedValue / (dynamicFollowRate * kRotationSpeedMultiplier)), kMaxBlendValue);
    worldTransform.quateRotation_ = Quaternion::Slerp(
        rushCameraRotation_, targetRot,
        ApplyEasing(rushCameraEasingType_, kVectorZero, kEasingMaxValue, rotT, kEasingMaxValue));
    rushCameraRotation_ = worldTransform.quateRotation_;

    worldTransform.UpdateMatrix();

    // ビュープロジェクションへ反映してカメラを確定
    pOwner_->ApplyToViewProjection();
    return true;
}

void CameraRush::ApplyCameraPosition(const Vector3 &cameraPos)
{
    WorldTransform &worldTransform = pOwner_->GetCameraWorldTransform();

    if (isResumeFromRush_)
    {
        // Rush演出からの復帰補間実行
        Vector3 targetCameraPos = cameraPos;
        Quaternion targetCameraRot = worldTransform.quateRotation_;

        rushResumeTimer_ += Frame::DeltaTime();
        float t = std::min(rushResumeTimer_ / (kNormalizedValue / rushResumeBlendSpeed_), kMaxBlendValue);

        worldTransform.translation_ = ApplyEasing(rushResumeEasingType_, rushCameraPosition_, targetCameraPos, t, kEasingMaxValue);
        worldTransform.quateRotation_ = Quaternion::Slerp(
            rushCameraRotation_, targetCameraRot,
            ApplyEasing(rushResumeEasingType_, kVectorZero, kEasingMaxValue, t, kEasingMaxValue));

        float positionDiff = (worldTransform.translation_ - targetCameraPos).Length();
        float rotationDiff = std::abs(kNormalizedValue - std::abs(worldTransform.quateRotation_.Dot(targetCameraRot)));
        if (positionDiff < rushPosArrivalThreshold_ && rotationDiff < rushRotationArrivalThreshold_)
        {
            isResumeFromRush_ = false;
        }

        rushCameraPosition_ = worldTransform.translation_;
        rushCameraRotation_ = worldTransform.quateRotation_;
    }
    else
    {
        worldTransform.translation_ = cameraPos;
    }

    worldTransform.UpdateMatrix();
}

void CameraRush::DrawImGui()
{
#ifdef USE_IMGUI
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

    int rushCameraEasing = static_cast<int>(rushCameraEasingType_);
    int rushResumeEasing = static_cast<int>(rushResumeEasingType_);

    if (ImGui::Combo("Rush Camera Easing", &rushCameraEasing, easingTypes, IM_ARRAYSIZE(easingTypes)))
    {
        rushCameraEasingType_ = static_cast<EasingType>(rushCameraEasing);
    }
    if (ImGui::Combo("Rush Resume Easing", &rushResumeEasing, easingTypes, IM_ARRAYSIZE(easingTypes)))
    {
        rushResumeEasingType_ = static_cast<EasingType>(rushResumeEasing);
    }
#endif // USE_IMGUI
}

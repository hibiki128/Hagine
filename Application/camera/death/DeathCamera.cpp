#define NOMINMAX
#include "DeathCamera.h"
#include <Frame.h>
#include <algorithm>

using namespace Hagine;

void DeathCamera::Init()
{
    // ViewProjectionの初期設定
    viewProjection_.farZ_ = kFarZ;
    viewProjection_.Initialize("");

    // WorldTransformの初期化
    worldTransform_.Initialize();
}

void DeathCamera::Update()
{
    // イージング中でなければ終了
    if (!isEasing_)
    {
        return;
    }

    // 進捗の計算 → トランスフォームの補間 → ビューへの反映、の順に処理する
    const float progress = AdvanceEasingProgress();
    ApplyEasedTransform(progress);
    ApplyToViewProjection();

    // イージング完了判定
    if (progress >= kEasingComplete)
    {
        isEasing_ = false;
        isComplete_ = true;
    }
}

float DeathCamera::AdvanceEasingProgress()
{
    easingTimer_ += Frame::DeltaTime();

    // 経過時間を演出時間で割った進捗。1.0を超えないようにする
    const float progress = std::clamp(easingTimer_ / easingDuration_, kEasingStart, kEasingComplete);

    // 中間地点到達判定（一度だけフラグを立てる）
    if (!isHalfway_ && progress >= kHalfwayRatio)
    {
        isHalfway_ = true;
    }

    return progress;
}

void DeathCamera::ApplyEasedTransform(float progress)
{
    // 位置はイージングカーブ、回転はクォータニオンの球面線形補間で繋ぐ
    worldTransform_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos_,
                                              progress, kEasingComplete);
    worldTransform_.quaternionRotation_ = Quaternion::Slerp(easingStartRot_, easingTargetRot_, progress);
    worldTransform_.UpdateMatrix();
}

void DeathCamera::ApplyToViewProjection()
{
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.isUseQuaternion_ = true;
    viewProjection_.quaternionRotation_ = worldTransform_.quaternionRotation_;
    viewProjection_.UpdateMatrix();
}

Quaternion DeathCamera::CalcLookAtRotation(const Vector3 &eyePosition, const Vector3 &lookAtPosition)
{
    const Vector3 forward = (lookAtPosition - eyePosition).Normalize();

    // 前方が上方向とほぼ平行なときは外積が退化するため、代替の右方向を使う
    Vector3 right = kWorldRight;
    if (std::abs(forward.Dot(kWorldUp)) <= kParallelThreshold)
    {
        right = (kWorldUp.Cross(forward)).Normalize();
    }

    // 前方と右方向から直交する上方向を再算出し、回転行列を組む
    const Vector3 up = (forward.Cross(right)).Normalize();
    const Matrix4x4 rotateMatrix = MakeRotateMatrix(right, up, forward);

    return Quaternion::FromMatrix(rotateMatrix);
}

void DeathCamera::StartEasing(const ViewProjection &currentViewProjection, const Vector3 &targetPosition)
{
    // 各種フラグのリセットとタイマーの初期化
    isEasing_ = true;
    isComplete_ = false;
    isHalfway_ = false;
    easingTimer_ = kEasingStart;

    // 現在のカメラ状態を保存（開始地点の設定）
    easingStartPos_ = currentViewProjection.translation_;
    easingStartRot_ = currentViewProjection.isUseQuaternion_
                          ? currentViewProjection.quaternionRotation_
                          : Quaternion::FromEulerAngles(currentViewProjection.eulerRotation_);

    // 目標位置（プレイヤーの正面やや斜め上）と、そこから対象を注視する回転
    easingTargetPos_ = targetPosition + cameraOffset_;
    easingTargetRot_ = CalcLookAtRotation(easingTargetPos_, targetPosition);

    // 初期状態をトランスフォームに適用
    worldTransform_.translation_ = easingStartPos_;
    worldTransform_.quaternionRotation_ = easingStartRot_;
    worldTransform_.UpdateMatrix();
}

void DeathCamera::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Begin("DeathCamera");
    ImGui::DragFloat3("Offset", &cameraOffset_.x, 0.1f, -10.0f, 10.0f);
    ImGui::DragFloat("Duration", &easingDuration_, 0.1f, 0.5f, 5.0f);
    ImGui::Checkbox("Is Easing", &isEasing_);
    ImGui::Checkbox("Is Complete", &isComplete_);
    ImGui::End();
#endif // USE_IMGUI
}

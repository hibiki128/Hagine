#define NOMINMAX
#include "DeathCamera.h"
#include <Frame.h>

void DeathCamera::Init() {
    // ViewProjectionの初期化
    vp_.farZ = kFarZ;
    vp_.Initialize("");
    // WorldTransformの初期化
    wt_.Initialize();
}

void DeathCamera::Update() {
    // イージング中でなければ終了
    if (!isEasing_) {
        return;
    }

    // タイマー更新
    easingTimer_ += Frame::DeltaTime();
    float t = std::min(easingTimer_ / easingDuration_, kEasingEndThreshold);

    // 半分到達チェック（一度だけフラグを立てる）
    if (!isHalfway_ && easingTimer_ >= easingDuration_ * kHalfwayRatio) {
        isHalfway_ = true;
    }

    // 位置のイージング
    wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos_, t, kEasingMaxValue);
    // 回転のイージング（クォータニオン補間）
    wt_.quateRotation_ = Quaternion::Slerp(easingStartRot_, easingTargetRot_, t);
    wt_.UpdateMatrix();

    // ViewProjectionを更新（クォータニオンモード）
    vp_.translation_ = wt_.translation_;
    vp_.isUseQuaternion_ = true;
    vp_.quateRotation_ = wt_.quateRotation_;
    vp_.UpdateMatrix();

    // イージング完了チェック
    if (t >= kEasingMaxValue) {
        isEasing_ = false;
        isComplete_ = true;
    }
}

void DeathCamera::StartEasing(const ViewProjection &currentVp, const Vector3 &targetPosition) {
    // フラグリセットとタイマー初期化
    isEasing_ = true;
    isComplete_ = false;
    isHalfway_ = false;
    easingTimer_ = kTimerReset;

    // 現在のカメラ状態を保存
    easingStartPos_ = currentVp.translation_;

    // クォータニオンが使用されているか確認して開始回転をセット
    if (currentVp.isUseQuaternion_) {
        easingStartRot_ = currentVp.quateRotation_;
    } else {
        easingStartRot_ = Quaternion::FromEulerAngles(currentVp.eulerRotation_);
    }

    // 目標位置を計算（プレイヤーの正面やや斜め上）
    easingTargetPos_ = targetPosition + cameraOffset_;

    // 目標回転を計算（プレイヤーを見る方向）
    Vector3 toTarget = (targetPosition - easingTargetPos_).Normalize();

    Vector3 forward = toTarget;
    Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ};

    // 前方ベクトルと上方向ベクトルから右方向ベクトルを計算
    Vector3 right;
    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
        right = {kRightVectorX, kRightVectorY, kRightVectorZ};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }
    // 正確な上方向ベクトルを再計算
    Vector3 up = (forward.Cross(right)).Normalize();

    // 回転行列から目標クォータニオンを作成
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    easingTargetRot_ = Quaternion::FromMatrix(rotMatrix);

    // WorldTransformの初期状態を設定
    wt_.translation_ = easingStartPos_;
    wt_.quateRotation_ = easingStartRot_;
    wt_.UpdateMatrix();
}

void DeathCamera::imgui() {
#ifdef USE_IMGUI
    ImGui::Begin("DeathCamera");
    ImGui::DragFloat3("Offset", &cameraOffset_.x, 0.1f, -10.0f, 10.0f);
    ImGui::DragFloat("Duration", &easingDuration_, 0.1f, 0.5f, 5.0f);
    ImGui::Checkbox("Is Easing", &isEasing_);
    ImGui::Checkbox("Is Complete", &isComplete_);
    ImGui::End();
#endif // USE_IMGUI
}
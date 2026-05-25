#define NOMINMAX
#include "DeathCamera.h"
#include <Frame.h>

void DeathCamera::Init() {
    // ViewProjectionの初期設定
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

    // 中間地点到達判定（一度だけフラグを立てる）
    if (!isHalfway_ && easingTimer_ >= easingDuration_ * kHalfwayRatio) {
        isHalfway_ = true;
    }

    // トランスフォームの補間
    // 位置のイージング
    wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos_, t, kEasingMaxValue);
    // 回転のイージング（クォータニオンによる補間）
    wt_.quateRotation_ = Quaternion::Slerp(easingStartRot_, easingTargetRot_, t);
    wt_.UpdateMatrix();

    // ビュープロジェクションへの反映
    vp_.translation_ = wt_.translation_;
    vp_.isUseQuaternion_ = true;
    vp_.quateRotation_ = wt_.quateRotation_;
    vp_.UpdateMatrix();

    // イージング完了判定
    if (t >= kEasingMaxValue) {
        isEasing_ = false;
        isComplete_ = true;
    }
}

void DeathCamera::StartEasing(const ViewProjection &currentVp, const Vector3 &targetPosition) {
    // 各種フラグのリセットとタイマーの初期化
    isEasing_ = true;
    isComplete_ = false;
    isHalfway_ = false;
    easingTimer_ = kTimerReset;

    // 現在のカメラ状態を保存（開始地点の設定）
    easingStartPos_ = currentVp.translation_;

    // クォータニオンが使用されているか確認して開始回転をセット
    if (currentVp.isUseQuaternion_) {
        easingStartRot_ = currentVp.quateRotation_;
    } else {
        easingStartRot_ = Quaternion::FromEulerAngles(currentVp.eulerRotation_);
    }

    // 目標位置の計算（プレイヤーの正面やや斜め上を向く位置）
    easingTargetPos_ = targetPosition + cameraOffset_;

    // 目標回転の計算（目標物を注視する回転）
    Vector3 toTarget = (targetPosition - easingTargetPos_).Normalize();

    Vector3 forward = toTarget;
    Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ};

    // 前方ベクトルと上方向ベクトルから右方向ベクトルを算出
    Vector3 right;
    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
        right = {kRightVectorX, kRightVectorY, kRightVectorZ};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }
    
    // 直交する正確な上方向ベクトルを再算出
    Vector3 up = (forward.Cross(right)).Normalize();

    // 回転行列を作成し、目標のクォータニオンを生成
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    easingTargetRot_ = Quaternion::FromMatrix(rotMatrix);

    // 初期状態をトランスフォームに適用
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

#define NOMINMAX
#include "StartCamera.h"
#include <Frame.h>

void StartCamera::Init() {
    vp_.farZ = 1100;
    vp_.Initialize("");
    wt_.Initialize();

    // 初期角度を設定(真東から開始)
    angle_ = degreesToRadians(-90.0f);

    // 初期位置を計算(中心点の周りに配置)
    wt_.translation_.x = centerPos_.x + radius_ * std::cos(angle_);
    wt_.translation_.y = 42.0f;
    wt_.translation_.z = centerPos_.z + radius_ * std::sin(angle_);

    wt_.UpdateMatrix();
}

void StartCamera::Update() {
    if (isEasing_) {
        easingTimer_ += Frame::DeltaTime();

        switch (easingPhase_) {
        case 1: // 1回目のイージング
        {
            float t = std::min(easingTimer_ / easingDuration_, 1.0f);

            // 位置のイージング
            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos_, t, 1.0f);

            // 回転のイージング
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, easingTargetRot_, t, 1.0f);

            wt_.UpdateMatrix();

            // ViewProjectionを更新
            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            // 1回目のイージング完了チェック
            if (t >= 1.0f) {
                easingPhase_ = 2; // 待機フェーズへ
                easingTimer_ = 0.0f;
            }
            break;
        }

        case 2: // 1回目完了後の待機
        {
            if (easingTimer_ >= waitDuration_) {
                // 2回目のイージング開始
                easingPhase_ = 3;
                easingTimer_ = 0.0f;
                easingStartPos_ = wt_.translation_;
                easingStartRot_ = wt_.eulerRotation_;
            }
            break;
        }

        case 3: // 2回目のイージング
        {
            float t = std::min(easingTimer_ / easingDuration_, 1.0f);

            // 位置のイージング
            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos2_, t, 1.0f);

            // 回転のイージング
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, easingTargetRot2_, t, 1.0f);

            wt_.UpdateMatrix();

            // ViewProjectionを更新
            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            // 2回目のイージング完了チェック
            if (t >= 1.0f) {
                easingPhase_ = 4; // 最終待機フェーズへ
                easingTimer_ = 0.0f;
            }
            break;
        }

       case 4: // 2回目完了後の待機
        {
            if (easingTimer_ >= waitDuration_) {
                // 3回目のイージング開始（targetVpへ）
                easingPhase_ = 5;
                easingTimer_ = 0.0f;
                easingStartPos_ = wt_.translation_;
                easingStartRot_ = wt_.eulerRotation_;
            }
            break;
        }

        case 5: // 3回目のイージング（targetVpへ）
        {
            float t = std::min(easingTimer_ / easingDuration_, 1.0f);

            // 位置のイージング
            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, targetVp_.translation_, t, 1.0f);

            // 回転のイージング
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, targetVp_.eulerRotation_, t, 1.0f);

            wt_.UpdateMatrix();

            // ViewProjectionを更新
            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            // 3回目のイージング完了チェック
            if (t >= 1.0f) {
                easingPhase_ = 6; // 最終待機フェーズへ
                easingTimer_ = 0.0f;
            }
            break;
        }

        case 6: // 3回目完了後の待機
        {
            if (easingTimer_ >= finalWaitDuration_) {
                // 完了
                easingPhase_ = 7;
                isEasing_ = false;
                isComplete_ = true;
            }
            break;
        }
        }

        return;
    }

    // カメラ位置を計算(中心点の周りを回転)
    wt_.translation_.x = centerPos_.x + radius_ * std::cos(angle_);
    wt_.translation_.y = 42.0f;
    wt_.translation_.z = centerPos_.z + radius_ * std::sin(angle_);

    // カメラから中心点への方向ベクトルを計算
    Vector3 toCenter = (centerPos_ - wt_.translation_).Normalize();

    // Y軸回転を計算
    float yaw = std::atan2(toCenter.x, toCenter.z);

    // ピッチを計算
    float horizontalDistance = std::sqrt(toCenter.x * toCenter.x + toCenter.z * toCenter.z);
    float pitch = std::atan2(-toCenter.y, horizontalDistance);

    wt_.eulerRotation_ = {pitch, yaw, 0.0f};

    wt_.UpdateMatrix();

    // ViewProjectionを更新
    vp_.translation_ = wt_.translation_;
    vp_.eulerRotation_ = wt_.eulerRotation_;
    vp_.UpdateMatrix();
}

void StartCamera::Move() {
    angle_ += speed_ * Frame::DeltaTime();

    if (angle_ > 1.0f * std::numbers::pi_v<float> && !isEasing_) {
        isEasing_ = true;
        easingPhase_ = 1;
        easingTimer_ = 0.0f;
        easingStartPos_ = wt_.translation_;
        easingStartRot_ = wt_.eulerRotation_;
    }
}

void StartCamera::imgui() {
#ifdef USE_IMGUI
    ImGui::Begin("StartCamera");
    ImGui::DragFloat("Speed", &speed_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat3("Center", &centerPos_.x, 0.1f, -100.0f, 100.0f);
    ImGui::DragFloat("Radius", &radius_, 0.1f, 1.0f, 200.0f);
    ImGui::End();
#endif // USE_IMGUI
}
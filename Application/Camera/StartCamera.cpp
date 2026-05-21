#define NOMINMAX
#include "StartCamera.h"
#include <Frame.h>
#include <GamePad.h>
#include <Input.h>

void StartCamera::Init() {
    // ViewProjectionの初期設定
    vp_.farZ = kFarZ;
    vp_.Initialize("");
    wt_.Initialize();

    // 初期角度を設定
    angle_ = degreesToRadians(kInitialAngleDegrees);

    // 初期位置を計算
    wt_.translation_.x = centerPos_.x + radius_ * std::cos(angle_);
    wt_.translation_.y = kInitialHeight;
    wt_.translation_.z = centerPos_.z + radius_ * std::sin(angle_);

    wt_.UpdateMatrix();

    // 入力の初期化
    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);
    input_ = Input::GetInstance();

    isSkipping_ = false;
}

bool StartCamera::CheckSkipInput() {
    // 入力がなければ終了
    if (!input_ || !gamePad_) {
        return false;
    }

    if (!gamePad_->IsConnected()) {
        // キーボード操作：スペースキーでスキップ
        return input_->TriggerKey(DIK_SPACE);
    } else {
        // コントローラー操作：Aボタンでスキップ
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_A);
    }
}

void StartCamera::Update() {
    // ゲームパッドの更新
    if (gamePad_) {
        gamePad_->Update();
    }

    // スキップ入力チェック
    if (CheckSkipInput() && !isComplete_) {
        isSkipping_ = true;
    }

    // イージング演出の更新
    if (isEasing_) {
        // スキップ中はタイマーを加速
        float deltaTime = Frame::DeltaTime();
        if (isSkipping_) {
            deltaTime *= kSkipSpeedMultiplier;
        }
        easingTimer_ += deltaTime;

        // 各フェーズに応じたイージング処理
        switch (easingPhase_) {
        case 1: // 1回目のイージング
        {
            float t = std::min(easingTimer_ / easingDuration_, kMaxBlendValue);

            // 位置と回転のイージング
            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos_, t, kEasingMaxValue);
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, easingTargetRot_, t, kEasingMaxValue);

            wt_.UpdateMatrix();

            // ViewProjectionを更新
            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            // 完了チェック
            if (t >= kMaxBlendValue) {
                easingPhase_ = kPhaseWait1;
                easingTimer_ = kTimerReset;
                isSkipping_ = false;
            }
            break;
        }

        case 2: // 1回目完了後の待機
        {
            if (easingTimer_ >= waitDuration_) {
                easingPhase_ = kPhaseEasing2;
                easingTimer_ = kTimerReset;
                easingStartPos_ = wt_.translation_;
                easingStartRot_ = wt_.eulerRotation_;
                isSkipping_ = false;
            }
            break;
        }

        case 3: // 2回目のイージング
        {
            float t = std::min(easingTimer_ / easingDuration_, kMaxBlendValue);

            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos2_, t, kEasingMaxValue);
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, easingTargetRot2_, t, kEasingMaxValue);

            wt_.UpdateMatrix();

            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            if (t >= kMaxBlendValue) {
                easingPhase_ = kPhaseWait2;
                easingTimer_ = kTimerReset;
                isSkipping_ = false;
            }
            break;
        }

        case 4: // 2回目完了後の待機
        {
            if (easingTimer_ >= waitDuration_) {
                easingPhase_ = kPhaseEasing3;
                easingTimer_ = kTimerReset;
                easingStartPos_ = wt_.translation_;
                easingStartRot_ = wt_.eulerRotation_;
                isSkipping_ = false;
            }
            break;
        }

        case 5: // 3回目のイージング（最終目標へ）
        {
            float t = std::min(easingTimer_ / easingDuration_, kMaxBlendValue);

            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, targetVp_.translation_, t, kEasingMaxValue);
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, targetVp_.eulerRotation_, t, kEasingMaxValue);

            wt_.UpdateMatrix();

            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            if (t >= kMaxBlendValue) {
                easingPhase_ = kPhaseWait3;
                easingTimer_ = kTimerReset;
                isSkipping_ = false;
            }
            break;
        }

        case 6: // 3回目完了後の待機
        {
            if (easingTimer_ >= finalWaitDuration_) {
                easingPhase_ = kPhaseComplete;
                isEasing_ = false;
                isComplete_ = true;
                isSkipping_ = false;
            }
            break;
        }
        }

        return;
    }

    // 通常の回転移動（イージング開始前）
    wt_.translation_.x = centerPos_.x + radius_ * std::cos(angle_);
    wt_.translation_.y = kInitialHeight;
    wt_.translation_.z = centerPos_.z + radius_ * std::sin(angle_);

    // 中心点を向くように回転を計算
    Vector3 toCenter = (centerPos_ - wt_.translation_).Normalize();
    float yaw = std::atan2(toCenter.x, toCenter.z);
    float horizontalDistance = std::sqrt(toCenter.x * toCenter.x + toCenter.z * toCenter.z);
    float pitch = std::atan2(-toCenter.y, horizontalDistance);

    wt_.eulerRotation_ = {pitch, yaw, kZeroRotation};
    wt_.UpdateMatrix();

    // ViewProjectionの更新
    vp_.translation_ = wt_.translation_;
    vp_.eulerRotation_ = wt_.eulerRotation_;
    vp_.UpdateMatrix();
}

void StartCamera::Move() {
    // 角度を更新
    float deltaTime = Frame::DeltaTime();
    if (isSkipping_) {
        deltaTime *= kSkipSpeedMultiplier;
    }
    angle_ += speed_ * deltaTime;

    // 一定角度を超えたらイージングフェーズへ移行
    if (angle_ > kHalfPi && !isEasing_) {
        isEasing_ = true;
        easingPhase_ = kPhaseEasing1;
        easingTimer_ = kTimerReset;
        easingStartPos_ = wt_.translation_;
        easingStartRot_ = wt_.eulerRotation_;
        isSkipping_ = false;
    }
}

void StartCamera::imgui() {
#ifdef USE_IMGUI
    ImGui::Begin("StartCamera");
    ImGui::DragFloat("Speed", &speed_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat3("Center", &centerPos_.x, 0.1f, -100.0f, 100.0f);
    ImGui::DragFloat("Radius", &radius_, 0.1f, 1.0f, 200.0f);
    ImGui::Checkbox("IsSkipping", &isSkipping_);
    ImGui::End();
#endif // USE_IMGUI
}
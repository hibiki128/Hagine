#define NOMINMAX
#include "StartCamera.h"
#include <Frame.h>
#include <GamePad.h>
#include <Input.h>

using namespace Hagine;
void StartCamera::Init()
{
    // ViewProjectionの初期設定
    vp_.farZ_ = kFarZ;
    vp_.Initialize("");
    wt_.Initialize();

    // 初期角度の設定と初期座標の算出
    angle_ = degreesToRadians(kInitialAngleDegrees);
    wt_.translation_.x = centerPos_.x + radius_ * std::cos(angle_);
    wt_.translation_.y = kInitialHeight;
    wt_.translation_.z = centerPos_.z + radius_ * std::sin(angle_);

    wt_.UpdateMatrix();

    // 入力システムの初期化
    pGamePad_ = std::make_unique<GamePad>();
    pGamePad_->Init(0);
    pInput_ = Input::GetInstance();

    isSkipping_ = false;
}

bool StartCamera::CheckSkipInput()
{
    // 入力インスタンスの存在確認
    if (!pInput_ || !pGamePad_)
    {
        return false;
    }

    if (!pGamePad_->IsConnected())
    {
        // キーボード：スペースキーでスキップ
        return pInput_->TriggerKey(DIK_SPACE);
    }
    else
    {
        // ゲームパッド：Aボタンでスキップ
        return pGamePad_->IsTrigger(XINPUT_GAMEPAD_A);
    }
}

void StartCamera::Update()
{
    // ゲームパッド情報の更新
    if (pGamePad_)
    {
        pGamePad_->Update();
    }

    // スキップ入力の監視（未完了時のみ）
    if (CheckSkipInput() && !isComplete_)
    {
        isSkipping_ = true;
    }

    // イージング演出の更新処理
    if (isEasing_)
    {
        // スキップ中は演出速度を加速させる
        float deltaTime = Frame::DeltaTime();
        if (isSkipping_)
        {
            deltaTime *= kSkipSpeedMultiplier;
        }
        easingTimer_ += deltaTime;

        // フェーズに応じたイージング処理の分岐
        switch (easingPhase_)
        {
        case 1: // 第1フェーズ：位置と回転のイージング
        {
            float t = std::min(easingTimer_ / easingDuration_, kMaxBlendValue);

            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos_, t, kEasingMaxValue);
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, easingTargetRot_, t, kEasingMaxValue);

            wt_.UpdateMatrix();

            // ビュープロジェクションへの反映
            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            if (t >= kMaxBlendValue)
            {
                easingPhase_ = kPhaseWait1;
                easingTimer_ = kTimerReset;
                isSkipping_ = false;
            }
            break;
        }

        case 2: // 第2フェーズ：待機
        {
            if (easingTimer_ >= waitDuration_)
            {
                easingPhase_ = kPhaseEasing2;
                easingTimer_ = kTimerReset;
                easingStartPos_ = wt_.translation_;
                easingStartRot_ = wt_.eulerRotation_;
                isSkipping_ = false;
            }
            break;
        }

        case 3: // 第3フェーズ：第2目標へのイージング
        {
            float t = std::min(easingTimer_ / easingDuration_, kMaxBlendValue);

            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, easingTargetPos2_, t, kEasingMaxValue);
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, easingTargetRot2_, t, kEasingMaxValue);

            wt_.UpdateMatrix();

            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            if (t >= kMaxBlendValue)
            {
                easingPhase_ = kPhaseWait2;
                easingTimer_ = kTimerReset;
                isSkipping_ = false;
            }
            break;
        }

        case 4: // 第4フェーズ：待機
        {
            if (easingTimer_ >= waitDuration_)
            {
                easingPhase_ = kPhaseEasing3;
                easingTimer_ = kTimerReset;
                easingStartPos_ = wt_.translation_;
                easingStartRot_ = wt_.eulerRotation_;
                isSkipping_ = false;
            }
            break;
        }

        case 5: // 第5フェーズ：最終目標（現在のカメラ位置）へのイージング
        {
            float t = std::min(easingTimer_ / easingDuration_, kMaxBlendValue);

            wt_.translation_ = ApplyEasing(EasingType::InOutQuad, easingStartPos_, targetVp_.translation_, t, kEasingMaxValue);
            wt_.eulerRotation_ = ApplyEasing(EasingType::InOutQuad, easingStartRot_, targetVp_.eulerRotation_, t, kEasingMaxValue);

            wt_.UpdateMatrix();

            vp_.translation_ = wt_.translation_;
            vp_.eulerRotation_ = wt_.eulerRotation_;
            vp_.UpdateMatrix();

            if (t >= kMaxBlendValue)
            {
                easingPhase_ = kPhaseWait3;
                easingTimer_ = kTimerReset;
                isSkipping_ = false;
            }
            break;
        }

        case 6: // 第6フェーズ：最終待機
        {
            if (easingTimer_ >= finalWaitDuration_)
            {
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

    // 通常時の回転移動処理
    wt_.translation_.x = centerPos_.x + radius_ * std::cos(angle_);
    wt_.translation_.y = kInitialHeight;
    wt_.translation_.z = centerPos_.z + radius_ * std::sin(angle_);

    // 常に中心点（ターゲット）を注視するように回転を算出
    Vector3 toCenter = (centerPos_ - wt_.translation_).Normalize();
    float yaw = std::atan2(toCenter.x, toCenter.z);
    float horizontalDistance = std::sqrt(toCenter.x * toCenter.x + toCenter.z * toCenter.z);
    float pitch = std::atan2(-toCenter.y, horizontalDistance);

    wt_.eulerRotation_ = {pitch, yaw, kZeroRotation};
    wt_.UpdateMatrix();

    // トランスフォーム情報の反映
    vp_.translation_ = wt_.translation_;
    vp_.eulerRotation_ = wt_.eulerRotation_;
    vp_.UpdateMatrix();
}

void StartCamera::Move()
{
    // 時間経過による角度の更新
    float deltaTime = Frame::DeltaTime();
    if (isSkipping_)
    {
        deltaTime *= kSkipSpeedMultiplier;
    }
    angle_ += speed_ * deltaTime;

    // 特定の角度に達した際、イージング演出フェーズへ移行
    if (angle_ > kHalfPi && !isEasing_)
    {
        isEasing_ = true;
        easingPhase_ = kPhaseEasing1;
        easingTimer_ = kTimerReset;
        easingStartPos_ = wt_.translation_;
        easingStartRot_ = wt_.eulerRotation_;
        isSkipping_ = false;
    }
}

void StartCamera::imgui()
{
#ifdef USE_IMGUI
    ImGui::Begin("StartCamera");
    ImGui::DragFloat("Speed", &speed_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat3("Center", &centerPos_.x, 0.1f, -100.0f, 100.0f);
    ImGui::DragFloat("Radius", &radius_, 0.1f, 1.0f, 200.0f);
    ImGui::Checkbox("IsSkipping", &isSkipping_);
    ImGui::End();
#endif // USE_IMGUI
}

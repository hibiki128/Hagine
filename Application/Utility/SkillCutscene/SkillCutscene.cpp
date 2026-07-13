#include "SkillCutscene.h"
#include <Application/Camera/FollowCamera/FollowCamera.h>
#include <Utility/Debug/GameParam/GameParamHub.h>

using namespace Hagine;

void SkillCutscene::Start(BaseObject *performer, FollowCamera *camera, std::function<void()> onActivate)
{
    if (IsActive())
    {
        return;
    }

    camera_ = camera;
    onActivate_ = std::move(onActivate);
    timer_ = 0.0f;

    if (camera_ && performer)
    {
        camera_->StartSkillCloseUp(performer);
        phase_ = Phase::CloseUp;
    }
    else
    {
        // カメラが無い場合は顔アップを省略し、発動遅延のみ行う
        phase_ = Phase::Delay;
    }
}

void SkillCutscene::Update(float dt)
{
    if (phase_ == Phase::Idle)
    {
        return;
    }

    timer_ += dt;

    if (phase_ == Phase::CloseUp)
    {
        if (timer_ >= closeUpDuration_)
        {
            // 顔アップ終了：通常カメラへ補間なしで即復帰する
            if (camera_)
            {
                camera_->EndSkillCloseUp();
            }
            phase_ = Phase::Delay;
            timer_ = 0.0f;
        }
        return;
    }

    // Phase::Delay（カメラ復帰後の発動待ち）
    if (timer_ >= activationDelay_)
    {
        phase_ = Phase::Idle;
        timer_ = 0.0f;
        if (onActivate_)
        {
            // コールバック内から Start が呼ばれても安全なように先に取り出す
            auto callback = std::move(onActivate_);
            onActivate_ = nullptr;
            callback();
        }
    }
}

void SkillCutscene::Cancel()
{
    if (phase_ == Phase::CloseUp && camera_)
    {
        camera_->EndSkillCloseUp();
    }
    phase_ = Phase::Idle;
    timer_ = 0.0f;
    onActivate_ = nullptr;
}

void SkillCutscene::RegisterParams(const std::string &owner)
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register(owner, "顔アップ時間(秒)", &closeUpDuration_, {0.05f, 0.1f, 5.0f});
    hub->Register(owner, "発動遅延(秒)", &activationDelay_, {0.05f, 0.0f, 3.0f});
}

#define NOMINMAX
#include "PlayerStateFlyMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

void PlayerStateFlyMove::Enter(Player &player) {
    player.GetAcceleration().y = kAccelerationZero;
    player.GetVelocity().y = kVelocityZero;
    fallInputTime_ = kInitialTime;
    fallInputCount_ = kInitialCount;
    isBoosting_ = false;
    spaceHeldTime_ = kInitialTime;
}

void PlayerStateFlyMove::Update(Player &player) {
    player.Move();
    AirMove(player);
    ChangeState(player);
    fallInputTime_ += player.GetDt();
    player.DirectionUpdate();
}

void PlayerStateFlyMove::Exit(Player &player) {
}

void PlayerStateFlyMove::AirMove(Player &player) {
    bool ascendInput = false;
    bool descendInput = false;
    bool descendTrigger = false;

    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力
        ascendInput = Input::GetInstance()->PushKey(DIK_SPACE);
        descendInput = Input::GetInstance()->PushKey(DIK_LSHIFT);
        descendTrigger = Input::GetInstance()->TriggerKey(DIK_LSHIFT);
    } else {
        // ゲームパッド入力
        ascendInput = player.GetGamePad()->IsPress(XINPUT_GAMEPAD_RIGHT_SHOULDER); // RB → 上昇
        descendInput = player.GetGamePad()->GetRightTrigger() > 0.25f;             // RT → 下降
        descendTrigger = player.GetGamePad()->IsRightTriggerTriggered(0.25f);      // RT トリガー判定
    }

    if (ascendInput) {
        float &vy = player.GetVelocity().y;
        vy = std::min(vy + kFlyAcceleration * player.GetDt(), kFlyMaxSpeed);
    } else if (descendInput) {
        float &vy = player.GetVelocity().y;
        vy = std::max(vy - kFlyAcceleration * player.GetDt(), -kFlyMaxSpeed);
    } else {
        float &vy = player.GetVelocity().y;
        vy *= kVelocityDampingFactor;
        if (std::abs(vy) < kVelocityStopThreshold) {
            vy = kVelocityZero;
        }
    }

    if (descendTrigger) {
        if (fallInputTime_ < kFallThresholdTime) {
            fallInputCount_++;
        } else {
            fallInputCount_ = kInitialCount;
        }
        fallInputTime_ = kInitialTime;
    }
}

void PlayerStateFlyMove::ChangeState(Player &player) {
    player.ChangeRush();

    if (player.GetCurrentStateName() == "Rush") {
        return;
    }

    if (fallInputCount_ >= kFallInputThreshold) {
        player.ChangeState("Air");
        fallInputCount_ = kInitialCount;
        return;
    }

    if (player.GetWorldTransform()) {
        if (player.GetLocalPosition().y <= kGroundLevel) {
            player.ChangeState("Idle");
            return;
        }
    }

    bool hasInput = false;

    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力
        if (Input::GetInstance()->TriggerKey(DIK_LSHIFT) ||
            Input::GetInstance()->PushKey(DIK_LSHIFT) ||
            Input::GetInstance()->PushKey(DIK_SPACE) ||
            Input::GetInstance()->PushKey(DIK_D) ||
            Input::GetInstance()->PushKey(DIK_A) ||
            Input::GetInstance()->PushKey(DIK_W) ||
            Input::GetInstance()->PushKey(DIK_S)) {
            hasInput = true;
        }
    } else {
        // ゲームパッド入力
        float leftStickX = player.GetGamePad()->GetLeftStickX();
        float leftStickY = player.GetGamePad()->GetLeftStickY();

        if (player.GetGamePad()->IsRightTriggerTriggered(0.25f) ||         // RT トリガー
            player.GetGamePad()->GetRightTrigger() > 0.25f ||              // RT 押下
            player.GetGamePad()->IsPress(XINPUT_GAMEPAD_RIGHT_SHOULDER) || // RB 押下
            leftStickX != 0.0f || leftStickY != 0.0f) {
            hasInput = true;
        }
    }

    if (!hasInput &&
        fallInputTime_ > kFallThresholdTime &&
        fallInputCount_ < kFallInputThreshold) {
        player.ChangeState("FlyIdle");
        return;
    }
}
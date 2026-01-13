#define NOMINMAX
#include "PlayerStateFlyIdle.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

void PlayerStateFlyIdle::Enter(Player &player) {
    player.GetAcceleration().y = kAccelerationZero;
    player.GetVelocity().y = kVelocityZero;
    fallInputTime_ = kInitialTime;
    fallInputCount_ = kInitialCount;
    isBoosting_ = false;
    spaceHeldTime_ = kInitialTime;
}

void PlayerStateFlyIdle::Update(Player &player) {
    AirMove(player);
    ChangeState(player);
    player.DirectionUpdate();
}

void PlayerStateFlyIdle::Exit(Player &player) {
}

void PlayerStateFlyIdle::AirMove(Player &player) {
    float &vx = player.GetVelocity().x;
    float &vz = player.GetVelocity().z;
    float &vy = player.GetVelocity().y;

    if (std::abs(vx) < kVelocityStopThreshold) {
        vx = kVelocityZero;
    } else {
        vx *= kDampingFactor;
    }

    if (std::abs(vz) < kVelocityStopThreshold) {
        vz = kVelocityZero;
    } else {
        vz *= kDampingFactor;
    }

    if (std::abs(vy) < kVelocityStopThreshold) {
        vy = kVelocityZero;
    } else {
        vy *= kDampingFactor;
    }

    if (vx == kVelocityZero && vz == kVelocityZero && vy == kVelocityZero) {
        player.GetMoveSpeed() = kMoveSpeedZero;
    }
}

void PlayerStateFlyIdle::ChangeState(Player &player) {
    player.ChangeRush();

    if (player.GetLocalPosition().y <= kGroundLevel) {
        player.ChangeState("Idle");
        return;
    }

    bool hasInput = false;

    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力
        if (Input::GetInstance()->PushKey(DIK_A) ||
            Input::GetInstance()->PushKey(DIK_D) ||
            Input::GetInstance()->PushKey(DIK_S) ||
            Input::GetInstance()->PushKey(DIK_W) ||
            Input::GetInstance()->PushKey(DIK_SPACE) ||
            Input::GetInstance()->PushKey(DIK_LSHIFT)) {
            hasInput = true;
        }
    } else {
        // ゲームパッド入力
        float leftStickX = player.GetGamePad()->GetLeftStickX();
        float leftStickY = player.GetGamePad()->GetLeftStickY();

        if (leftStickX != 0.0f || leftStickY != 0.0f ||
            player.GetGamePad()->IsPress(XINPUT_GAMEPAD_A) ||
            player.GetGamePad()->IsPress(XINPUT_GAMEPAD_B)) {
            hasInput = true;
        }
    }

    if (hasInput) {
        player.ChangeState("FlyMove");
        return;
    }
}
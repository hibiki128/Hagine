#include "PlayerStateIdle.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <Frame.h>
#include <application/Utility/MotionEditor/MotionEditor.h>

void PlayerStateIdle::Enter(Player &player) {
}

void PlayerStateIdle::Update(Player &player) {
    player.GetCanJump() = player.GetIsGrounded();

    player.GetVelocity().y = kGroundPullVelocity;

    float &vx = player.GetVelocity().x;
    float &vz = player.GetVelocity().z;

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

    if (vx == kVelocityZero && vz == kVelocityZero) {
        player.GetMoveSpeed() = kMoveSpeedZero;
    }

    if (Input::GetInstance()->PushKey(DIK_A) ||
        Input::GetInstance()->PushKey(DIK_D) ||
        Input::GetInstance()->PushKey(DIK_S) ||
        Input::GetInstance()->PushKey(DIK_W)) {
        player.ChangeState("Move");
        return;
    }

    if (Input::GetInstance()->TriggerKey(DIK_SPACE) && player.GetCanJump()) {
        player.ChangeState("Jump");
        return;
    }

    player.DirectionUpdate();
}

void PlayerStateIdle::Exit(Player &player) {
}
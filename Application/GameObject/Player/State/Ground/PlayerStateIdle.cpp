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

    bool isMoving = false;

    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力
        if (Input::GetInstance()->PushKey(DIK_A) ||
            Input::GetInstance()->PushKey(DIK_D) ||
            Input::GetInstance()->PushKey(DIK_S) ||
            Input::GetInstance()->PushKey(DIK_W)) {
            isMoving = true;
        }
    } else {
        // ゲームパッド入力
        float leftStickX = player.GetGamePad()->GetLeftStickX();
        float leftStickY = player.GetGamePad()->GetLeftStickY();

        if (leftStickX != 0.0f || leftStickY != 0.0f) {
            isMoving = true;
        }
    }

    if (isMoving) {
        player.ChangeState("Move");
        return;
    }

    bool jumpInput = false;

    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力
        jumpInput = Input::GetInstance()->TriggerKey(DIK_SPACE);
    } else {
        // ゲームパッド入力
        jumpInput = player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_RIGHT_SHOULDER);
    }

    if (jumpInput && player.GetCanJump()) {
        player.ChangeState("Jump");
        return;
    }

    player.DirectionUpdate();
}

void PlayerStateIdle::Exit(Player &player) {
}
#include "PlayerStateMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/Camera/FollowCamera.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

void PlayerStateMove::Enter(Player &player) {
    float currentHorizontalSpeed = sqrt(player.GetVelocity().x * player.GetVelocity().x +
                                        player.GetVelocity().z * player.GetVelocity().z);
    if (currentHorizontalSpeed < kMinInitialSpeed) {
        player.GetMoveSpeed() = kMinInitialSpeed;
    } else {
        player.GetMoveSpeed() = currentHorizontalSpeed;
    }
    if (player.GetMoveSpeed() > player.GetMaxSpeed()) {
        player.GetMoveSpeed() = player.GetMaxSpeed();
    }
}

void PlayerStateMove::Update(Player &player) {
    player.GetCanJump() = player.GetIsGrounded();

    player.GetVelocity().y = kGroundPullVelocity;

    player.Move();

    bool isMoving = false;

    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力
        if (Input::GetInstance()->PushKey(DIK_W) ||
            Input::GetInstance()->PushKey(DIK_A) ||
            Input::GetInstance()->PushKey(DIK_S) ||
            Input::GetInstance()->PushKey(DIK_D)) {
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

    if (!isMoving) {
        player.ChangeState("Idle");
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

void PlayerStateMove::Exit(Player &player) {
}
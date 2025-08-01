#include "PlayerStateMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/Camera/FollowCamera.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <cmath> // ベクトル計算に必要

void PlayerStateMove::Enter(Player &player) {
    float currentHorizontalSpeed = sqrt(player.GetVelocity().x * player.GetVelocity().x +
                                        player.GetVelocity().z * player.GetVelocity().z);
    if (currentHorizontalSpeed < 2.0f) {
        player.GetMoveSpeed() = 2.0f;
    } else {
        player.GetMoveSpeed() = currentHorizontalSpeed;
    }
    if (player.GetMoveSpeed() > player.GetMaxSpeed()) {
        player.GetMoveSpeed() = player.GetMaxSpeed();
    }
}

void PlayerStateMove::Update(Player &player) {
    player.GetCanJump() = player.GetIsGrounded();

    player.GetVelocity().y = -0.1f;

    player.Move();

    if (!Input::GetInstance()->PushKey(DIK_W)&&
        !Input::GetInstance()->PushKey(DIK_A) &&
        !Input::GetInstance()->PushKey(DIK_S) &&
        !Input::GetInstance()->PushKey(DIK_D)) {
        player.ChangeState("Idle");
        return;
    }

    if (Input::GetInstance()->TriggerKey(DIK_SPACE) && player.GetCanJump()) {
        player.ChangeState("Jump");
        return;
    }

    // 方向の更新
    player.DirectionUpdate();
}

void PlayerStateMove::Exit(Player &player) {
}
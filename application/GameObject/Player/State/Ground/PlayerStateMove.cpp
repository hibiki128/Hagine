#include "PlayerStateMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/Camera/FollowCamera.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <cmath> // ベクトル計算に必要

void PlayerStateMove::Enter(Player &player) {
    // 移動状態の初期化：着地時の衝撃を和らげるため、
    // 既に持っていた速度を一部継続させる
    // 水平速度は維持するが、最低速度を確保して滑らかに移動開始
    float currentHorizontalSpeed = sqrt(player.GetVelocity().x * player.GetVelocity().x +
                                        player.GetVelocity().z * player.GetVelocity().z);
    if (currentHorizontalSpeed < 2.0f) {
        player.GetMoveSpeed() = 2.0f; // 最低速度を設定
    } else {
        player.GetMoveSpeed() = currentHorizontalSpeed;
    }
    // 最大速度を超えないように
    if (player.GetMoveSpeed() > player.GetMaxSpeed()) {
        player.GetMoveSpeed() = player.GetMaxSpeed();
    }
}

void PlayerStateMove::Update(Player &player) {
    // 接地している場合のみジャンプ可能
    player.GetCanJump() = player.GetIsGrounded();

    // 重力をわずかに適用して地面にとどまるようにする
    player.GetVelocity().y = -0.1f;

    player.Move();

    if (!Input::GetInstance()->PushKey(DIK_W)&&
        !Input::GetInstance()->PushKey(DIK_A) &&
        !Input::GetInstance()->PushKey(DIK_S) &&
        !Input::GetInstance()->PushKey(DIK_D)) {
        player.ChangeState("Idle");
        return;
    }

    // ジャンプ入力があればジャンプ状態へ
    if (Input::GetInstance()->TriggerKey(DIK_SPACE) && player.GetCanJump()) {
        player.ChangeState("Jump");
        return;
    }

    // 方向の更新
    player.DirectionUpdate();
}

void PlayerStateMove::Exit(Player &player) {
    // 特に何もしない
}
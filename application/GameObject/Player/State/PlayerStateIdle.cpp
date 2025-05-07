#include "PlayerStateIdle.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"

void PlayerStateIdle::Enter(Player &player) {
    // アイドル状態に入ったら速度をリセット
    player.GetVelocity().x = 0.0f;
    player.GetVelocity().y = 0.0f;
    player.GetVelocity().z = 0.0f;
    player.GetMoveSpeed() = 0.0f; // 移動速度をリセット
}

void PlayerStateIdle::Update(Player &player) {
    // 接地している場合のみジャンプ可能
    player.GetCanJump() = player.GetIsGrounded();

    // 重力をわずかに適用して地面にとどまるようにする
    player.GetVelocity().y = -0.1f;

    // 移動入力があれば移動状態へ
    if (Input::GetInstance()->PushKey(DIK_A) ||
        Input::GetInstance()->PushKey(DIK_D) ||
        Input::GetInstance()->PushKey(DIK_S) ||
        Input::GetInstance()->PushKey(DIK_W)) {
        player.ChangeState("Move");
        return;
    }

    // ジャンプ入力があればジャンプ状態へ
    if (Input::GetInstance()->TriggerKey(DIK_SPACE) && player.GetCanJump()) {
        player.ChangeState("Jump");
        return;
    }

    player.DirectionUpdate();
}

void PlayerStateIdle::Exit(Player &player) {
    // 特に何もしない
}
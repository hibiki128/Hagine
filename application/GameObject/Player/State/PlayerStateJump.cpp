#include "PlayerStateJump.h"
#include "application/GameObject/Player/Player.h"

void PlayerStateJump::Enter(Player &player) {
    // ジャンプ初速度の設定
    player.GetVelocity().y = player.GetJumpSpeed();
    player.GetCanJump() = false;    // ジャンプしたらジャンプフラグをリセット
    player.GetIsGrounded() = false; // 地面から離れる
}

void PlayerStateJump::Update(Player &player) {
    // ジャンプ状態はフレーム単位のため、すぐに空中状態に移行
    player.ChangeState("Air");
}

void PlayerStateJump::Exit(Player &player) {
    // 特に何もしない
}
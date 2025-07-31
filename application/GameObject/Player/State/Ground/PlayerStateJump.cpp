#include "PlayerStateJump.h"
#include "application/GameObject/Player/Player.h"

void PlayerStateJump::Enter(Player &player) {
    player.GetVelocity().y = player.GetJumpSpeed();
    player.GetCanJump() = false;  
    player.GetIsGrounded() = false; 
}

void PlayerStateJump::Update(Player &player) {
    player.ChangeState("Air");
}

void PlayerStateJump::Exit(Player &player) {
}
#include "PlayerStateJump.h"
#include "application/GameObject/Player/Player.h"

using namespace Hagine;
using namespace Math;
using namespace Graphics;
using namespace Camera;
using namespace Collision;

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
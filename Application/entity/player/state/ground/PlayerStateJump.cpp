#include "PlayerStateJump.h"
#include "application/entity/player/Player.h"

using namespace Hagine;
void PlayerStateJump::Enter(Player &player)
{
    player.GetVelocity().y = player.GetJumpSpeed();
    player.GetCanJump() = false;
    player.GetIsGrounded() = false;
}

void PlayerStateJump::Update(Player &player)
{
    player.ChangeState("Air");
}

void PlayerStateJump::Exit(Player &player)
{
}
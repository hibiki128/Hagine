#define NOMINMAX
#include "PlayerStateMove.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

using namespace Hagine;
void PlayerStateMove::Enter(Player &player)
{
    // 現在の水平速度を取得し、最小初期速度と最大速度の範囲に収める
    float currentHorizontalSpeed = std::sqrt(
        player.GetVelocity().x * player.GetVelocity().x +
        player.GetVelocity().z * player.GetVelocity().z);

    player.GetMoveSpeed() = std::max(currentHorizontalSpeed, kMinInitialSpeed);
    player.GetMoveSpeed() = std::min(player.GetMoveSpeed(), player.GetMaxSpeed());
}

void PlayerStateMove::Update(Player &player)
{
    player.GetCanJump() = player.GetIsGrounded();
    player.GetVelocity().y = kGroundPullVelocity;

    player.Move();

    if (!HasMovementInput(player))
    {
        player.ChangeState("Idle");
        return;
    }

    if (IsJumpOrFlyTriggered(player) && player.GetCanJump())
    {
        player.ChangeState("Jump");
        return;
    }

    player.DirectionUpdate();
    player.ChangeEnergyCharge();
}

void PlayerStateMove::Exit(Player &player)
{
}

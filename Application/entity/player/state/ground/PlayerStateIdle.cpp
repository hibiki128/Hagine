#include "PlayerStateIdle.h"
#include "application/entity/player/Player.h"

using namespace Hagine;
void PlayerStateIdle::Enter(Player &player)
{
}

void PlayerStateIdle::Update(Player &player)
{
    player.GetCanJump() = player.GetIsGrounded();
    player.GetVelocity().y = kGroundPullVelocity;

    // 水平速度の減衰
    float &vx = player.GetVelocity().x;
    float &vz = player.GetVelocity().z;

    if (std::abs(vx) < kVelocityStopThreshold)
    {
        vx = kVelocityZero;
    }
    else
    {
        vx *= kDampingFactor;
    }

    if (std::abs(vz) < kVelocityStopThreshold)
    {
        vz = kVelocityZero;
    }
    else
    {
        vz *= kDampingFactor;
    }

    if (vx == kVelocityZero && vz == kVelocityZero)
    {
        player.GetMoveSpeed() = kMoveSpeedZero;
    }

    if (HasMovementInput(player))
    {
        player.ChangeState("Move");
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

void PlayerStateIdle::Exit(Player &player)
{
}

#include "PlayerStateMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/Camera/FollowCamera.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

using namespace Hagine;
using namespace Math;
using namespace Graphics;
using namespace Camera;
using namespace Collision;

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

    if (!Input::GetInstance()->PushKey(DIK_W) &&
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

    player.DirectionUpdate();
}

void PlayerStateMove::Exit(Player &player) {
}
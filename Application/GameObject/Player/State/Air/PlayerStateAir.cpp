#include "PlayerStateAir.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

using namespace Hagine;
using namespace Math;
using namespace Graphics;
using namespace Camera;
using namespace Collision;

void PlayerStateAir::Enter(Player &player) {
    player.GetAcceleration().y = player.GetFallSpeed();
    elapsedTime_ = kInitialElapsedTime;
}

void PlayerStateAir::Update(Player &player) {

    player.Move();

    if (Frame::DeltaTime() <= kMaxDeltaTime) {
        player.GetVelocity().y += player.GetAcceleration().y * Frame::DeltaTime();
    }

    player.DirectionUpdate();

    elapsedTime_ += Frame::DeltaTime();

    if (elapsedTime_ < kFlyTransitionTime && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
        player.ChangeState("FlyIdle");
        return;
    }
}

void PlayerStateAir::Exit(Player &player) {
}
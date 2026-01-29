#define NOMINMAX
#include "PlayerStateFlyIdle.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

using namespace Hagine;
using namespace Math;
using namespace Graphics;
using namespace Camera;
using namespace Collision;

void PlayerStateFlyIdle::Enter(Player &player) {
    player.GetAcceleration().y = kAccelerationZero;
    player.GetVelocity().y = kVelocityZero;
    fallInputTime_ = kInitialTime;
    fallInputCount_ = kInitialCount;
    isBoosting_ = false;
    spaceHeldTime_ = kInitialTime;
}

void PlayerStateFlyIdle::Update(Player &player) {
    AirMove(player);
    ChangeState(player);
    player.DirectionUpdate();
}

void PlayerStateFlyIdle::Exit(Player &player) {
}

void PlayerStateFlyIdle::AirMove(Player &player) {
    float &vx = player.GetVelocity().x;
    float &vz = player.GetVelocity().z;
    float &vy = player.GetVelocity().y;

    if (std::abs(vx) < kVelocityStopThreshold) {
        vx = kVelocityZero;
    } else {
        vx *= kDampingFactor;
    }

    if (std::abs(vz) < kVelocityStopThreshold) {
        vz = kVelocityZero;
    } else {
        vz *= kDampingFactor;
    }

    if (std::abs(vy) < kVelocityStopThreshold) {
        vy = kVelocityZero;
    } else {
        vy *= kDampingFactor;
    }

    if (vx == kVelocityZero && vz == kVelocityZero && vy == kVelocityZero) {
        player.GetMoveSpeed() = kMoveSpeedZero;
    }
}

void PlayerStateFlyIdle::ChangeState(Player &player) {
    player.ChangeRush();

    if (player.GetLocalPosition().y <= kGroundLevel) {
        player.ChangeState("Idle");
        return;
    }

    if (Input::GetInstance()->PushKey(DIK_A) ||
        Input::GetInstance()->PushKey(DIK_D) ||
        Input::GetInstance()->PushKey(DIK_S) ||
        Input::GetInstance()->PushKey(DIK_W) ||
        Input::GetInstance()->PushKey(DIK_SPACE) ||
        Input::GetInstance()->PushKey(DIK_LSHIFT)) {
        player.ChangeState("FlyMove");
        return;
    }
}
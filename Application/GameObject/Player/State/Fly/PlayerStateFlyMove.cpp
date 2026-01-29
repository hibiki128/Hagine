#define NOMINMAX
#include "PlayerStateFlyMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

using namespace Hagine;
using namespace Math;
using namespace Graphics;
using namespace Camera;
using namespace Collision;

void PlayerStateFlyMove::Enter(Player &player) {
    player.GetAcceleration().y = kAccelerationZero;
    player.GetVelocity().y = kVelocityZero;
    fallInputTime_ = kInitialTime;
    fallInputCount_ = kInitialCount;
    isBoosting_ = false;
    spaceHeldTime_ = kInitialTime;
}

void PlayerStateFlyMove::Update(Player &player) {
    player.Move();
    AirMove(player);
    ChangeState(player);
    fallInputTime_ += player.GetDt();
    player.DirectionUpdate();
}

void PlayerStateFlyMove::Exit(Player &player) {
}

void PlayerStateFlyMove::AirMove(Player &player) {
    if (Input::GetInstance()->PushKey(DIK_SPACE)) {
        float &vy = player.GetVelocity().y;
        vy = std::min(vy + kFlyAcceleration * player.GetDt(), kFlyMaxSpeed);
    } else if (Input::GetInstance()->PushKey(DIK_LSHIFT)) {
        float &vy = player.GetVelocity().y;
        vy = std::max(vy - kFlyAcceleration * player.GetDt(), -kFlyMaxSpeed);
    } else {
        float &vy = player.GetVelocity().y;
        vy *= kVelocityDampingFactor;
        if (std::abs(vy) < kVelocityStopThreshold) {
            vy = kVelocityZero;
        }
    }

    if (Input::GetInstance()->TriggerKey(DIK_LSHIFT)) {
        if (fallInputTime_ < kFallThresholdTime) {
            fallInputCount_++;
        } else {
            fallInputCount_ = kInitialCount;
        }
        fallInputTime_ = kInitialTime;
    }
}

void PlayerStateFlyMove::ChangeState(Player &player) {
    player.ChangeRush();

    if (fallInputCount_ >= kFallInputThreshold) {
        player.ChangeState("Air");
        fallInputCount_ = kInitialCount;
        return;
    }

    if (player.GetWorldTransform()) {
        if (player.GetLocalPosition().y <= kGroundLevel) {
            player.ChangeState("Idle");
            return;
        }
    }

    if (!Input::GetInstance()->TriggerKey(DIK_LSHIFT) &&
        !Input::GetInstance()->PushKey(DIK_LSHIFT) &&
        !Input::GetInstance()->PushKey(DIK_SPACE) &&
        !Input::GetInstance()->PushKey(DIK_D) &&
        !Input::GetInstance()->PushKey(DIK_A) &&
        !Input::GetInstance()->PushKey(DIK_W) &&
        !Input::GetInstance()->PushKey(DIK_S) &&
        fallInputTime_ > kFallThresholdTime &&
        fallInputCount_ < kFallInputThreshold) {
        player.ChangeState("FlyIdle");
        return;
    }
}
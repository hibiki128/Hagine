#define NOMINMAX
#include "PlayerStateFlyMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>
namespace {
const float kFlyAcceleration = 30.0f;
const float kFlyMaxSpeed = 15.0f;
const float kFlyMaxSpeedBoost = 20.0f;
const float kFallThresholdTime = 0.3f;
}

void PlayerStateFlyMove::Enter(Player &player) {
    player.GetAcceleration().y = 0.0f;
    player.GetVelocity().y = 0.0f;
    fallInputTime_ = 0.0f;
    fallInputCount_ = 0;
    isBoosting_ = false;
    spaceHeldTime_ = 0.0f;
}

void PlayerStateFlyMove::Update(Player &player) {
    player.Move();

    AirMove(player);

    ChangeState(player);

    fallInputTime_ += player.GetDt();

    // 向き更新
    player.DirectionUpdate();
}

void PlayerStateFlyMove::Exit(Player &player) {
}

void PlayerStateFlyMove::AirMove(Player &player) {

    // 上昇処理（Space長押し）
    if (Input::GetInstance()->PushKey(DIK_SPACE)) {
        float &vy = player.GetVelocity().y;
        vy = std::min(vy + kFlyAcceleration * player.GetDt(), kFlyMaxSpeed);
    }
    // 下降処理（LShift長押し）
    else if (Input::GetInstance()->PushKey(DIK_LSHIFT)) {
        float &vy = player.GetVelocity().y;
        vy = std::max(vy - kFlyAcceleration * player.GetDt(), -kFlyMaxSpeed);
    } else {
        float &vy = player.GetVelocity().y;
        vy *= 0.70f;
        if (std::abs(vy) < 0.05f) {
            vy = 0.0f;
        }
    }

    // 下降2回押し → Airに移行
    if (Input::GetInstance()->TriggerKey(DIK_LSHIFT)) {
        if (fallInputTime_ < kFallThresholdTime) {
            fallInputCount_++;
        } else {
            fallInputCount_ = 0;
        }
        fallInputTime_ = 0.0f;
    }
}

void PlayerStateFlyMove::ChangeState(Player &player) {

   player.ChangeRush();

    if (fallInputCount_ >= 1) {
        player.ChangeState("Air");
        fallInputCount_ = 0;
        return;
    }

    // 地面に到達 → Idleへ
    if (player.GetWorldTransform()) {
        if (player.GetLocalPosition().y <= 0.0f) {
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
        fallInputCount_ < 1) {
        player.ChangeState("FlyIdle");
        return;
    }
}

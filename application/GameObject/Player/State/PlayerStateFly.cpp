#define NOMINMAX
#include "PlayerStateFly.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

namespace {
const float kFlyAcceleration = 30.0f;
const float kFlyMaxSpeed = 15.0f;
const float kFlyMaxSpeedBoost = 20.0f;
const float kFallThresholdTime = 0.3f; // 二回押しの間隔
} // namespace

void PlayerStateFly::Enter(Player &player) {
    // 浮遊時は重力をゼロに
    player.GetAcceleration().y = 0.0f;
    player.GetVelocity().y = 0.0f;
    fallInputTime_ = 0.0f;
    fallInputCount_ = 0;
    isBoosting_ = false;
    spaceHeldTime_ = 0.0f;
}

void PlayerStateFly::Update(Player &player) {
    player.Move();

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
            fallInputCount_ = 1;
        }
        fallInputTime_ = 0.0f;
    }

    if (fallInputCount_ >= 2) {
        player.ChangeState("Air");
        return;
    }

    fallInputTime_ += player.GetDt();

    // 地面に到達 → Idleへ
    if (player.GetWorldPosition().y <= 0.0f) {
        player.ChangeState("Idle");
        return;
    }

    // 向き更新
    player.DirectionUpdate();
}

void PlayerStateFly::Exit(Player &player) {
    // 重力の復帰は次のステートに任せる
}

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
    float dt = Frame::DeltaTime();

    // WASD入力の取得
    float xInput = 0.0f;
    float zInput = 0.0f;
    if (Input::GetInstance()->PushKey(DIK_D))
        xInput += 1.0f;
    if (Input::GetInstance()->PushKey(DIK_A))
        xInput -= 1.0f;
    if (Input::GetInstance()->PushKey(DIK_W))
        zInput += 1.0f;
    if (Input::GetInstance()->PushKey(DIK_S))
        zInput -= 1.0f;

    bool isMoving = (xInput != 0.0f || zInput != 0.0f);

    // Ctrlキーで最大速度を一時的に2倍に
    float speedMultiplier = (Input::GetInstance()->PushKey(DIK_LCONTROL) || Input::GetInstance()->PushKey(DIK_RCONTROL)) ? 2.0f : 1.0f;
    float maxSpeed = player.GetMaxSpeed() * speedMultiplier;

    if (isMoving) {
        // 正規化（斜め移動対策）
        float length = sqrt(xInput * xInput + zInput * zInput);
        xInput /= length;
        zInput /= length;

        // 加速
        player.GetMoveSpeed() += player.GetAccelRate() * dt;
        if (player.GetMoveSpeed() > maxSpeed) {
            player.GetMoveSpeed() = maxSpeed;
        }

        // 速度ベクトルを設定
        player.GetVelocity().x = xInput * player.GetMoveSpeed();
        player.GetVelocity().z = zInput * player.GetMoveSpeed();
    } else {
        // 減速
        player.GetMoveSpeed() -= player.GetAccelRate() * 2.0f * dt;
        if (player.GetMoveSpeed() < 0.0f) {
            player.GetMoveSpeed() = 0.0f;
        }

        // 慣性でゆっくり止まる
        player.GetVelocity().x *= 0.9f;
        player.GetVelocity().z *= 0.9f;
    }

    // 上昇処理（Space長押し）
    if (Input::GetInstance()->PushKey(DIK_SPACE)) {
        float &vy = player.GetVelocity().y;
        vy = std::min(vy + kFlyAcceleration * dt, kFlyMaxSpeed);
    }
    // 下降処理（LShift長押し）
    else if (Input::GetInstance()->PushKey(DIK_LSHIFT)) {
        float &vy = player.GetVelocity().y;
        vy = std::max(vy - kFlyAcceleration * dt, -kFlyMaxSpeed);
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

    fallInputTime_ += dt;

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

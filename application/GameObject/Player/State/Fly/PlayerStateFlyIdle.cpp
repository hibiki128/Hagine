#define NOMINMAX
#include "PlayerStateFlyIdle.h"
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

void PlayerStateFlyIdle::Enter(Player &player) {
    // 浮遊時は重力をゼロに
    player.GetAcceleration().y = 0.0f;
    player.GetVelocity().y = 0.0f;
    fallInputTime_ = 0.0f;
    fallInputCount_ = 0;
    isBoosting_ = false;
    spaceHeldTime_ = 0.0f;
}

void PlayerStateFlyIdle::Update(Player &player) {
    // --- 減速処理（X,Z方向） ---
    float &vx = player.GetVelocity().x;
    float &vz = player.GetVelocity().z;
    float &vy = player.GetVelocity().y;

    // 慣性を表す減衰率
    const float damping = 0.75f;

    // 一定以下になったら止める
    if (std::abs(vx) < 0.01f) {
        vx = 0.0f;
    } else {
        vx *= damping;
    }

    if (std::abs(vz) < 0.01f) {
        vz = 0.0f;
    } else {
        vz *= damping;
    }

    if (std::abs(vy) < 0.01f) {
        vy = 0.0f;
    } else {
        vy *= damping;
    }

    // 完全に止まっているときは移動速度もゼロに
    if (vx == 0.0f && vz == 0.0f && vy == 0.0f) {
        player.GetMoveSpeed() = 0.0f;
    }

    // 地面に到達 → Idleへ
    if (player.GetLocalPosition().y <= 0.0f) {
        player.ChangeState("Idle");
        return;
    }

    // --- 入力処理 ---
    if (Input::GetInstance()->PushKey(DIK_A) ||
        Input::GetInstance()->PushKey(DIK_D) ||
        Input::GetInstance()->PushKey(DIK_S) ||
        Input::GetInstance()->PushKey(DIK_W) ||
        Input::GetInstance()->PushKey(DIK_SPACE) ||
        Input::GetInstance()->PushKey(DIK_LSHIFT)) {
        player.ChangeState("FlyMove");
        return;
    }

    player.DirectionUpdate();
}

void PlayerStateFlyIdle::Exit(Player &player) {
    // 重力の復帰は次のステートに任せる
}
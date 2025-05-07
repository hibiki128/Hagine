#include "PlayerStateAir.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath> // ベクトルの正規化に必要

void PlayerStateAir::Enter(Player &player) {
    player.GetAcceleration().y = player.GetFallSpeed(); // 落下加速度の設定
}

void PlayerStateAir::Update(Player &player) {
    // 空中での移動処理（空中操作性を制限することも可能）
    float xInput = 0.0f;
    float zInput = 0.0f;

    if (Input::GetInstance()->PushKey(DIK_D)) {
        xInput += 1.0f;
    }
    if (Input::GetInstance()->PushKey(DIK_A)) {
        xInput -= 1.0f;
    }
    if (Input::GetInstance()->PushKey(DIK_W)) {
        zInput += 1.0f;
    }
    if (Input::GetInstance()->PushKey(DIK_S)) {
        zInput -= 1.0f;
    }

    // 斜め移動の正規化（ベクトルの長さを1に）
    if (xInput != 0.0f || zInput != 0.0f) {
        float length = sqrt(xInput * xInput + zInput * zInput);
        xInput /= length;
        zInput /= length;

        // 空中では操作性を制限（地上より遅く）
        float airControlFactor = 0.7f;
        player.GetVelocity().x = xInput * player.GetMoveSpeed() * airControlFactor;
        player.GetVelocity().z = zInput * player.GetMoveSpeed() * airControlFactor;
    } else {
        // 入力がない場合は徐々に減速
        player.GetVelocity().x *= 0.98f;
        player.GetVelocity().z *= 0.98f;
    }

    // 重力の適用
    if (Frame::DeltaTime() <= 1.0f) {
        player.GetVelocity().y += player.GetAcceleration().y * Frame::DeltaTime();
    }

    player.DirectionUpdate();

    // 地面判定は Player::Update で行われるので、ここでは特に処理しない
}

void PlayerStateAir::Exit(Player &player) {
    // 必要に応じて終了処理
}
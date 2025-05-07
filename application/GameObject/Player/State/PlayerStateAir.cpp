#include "PlayerStateAir.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

void PlayerStateAir::Enter(Player &player) {
    player.GetAcceleration().y = player.GetFallSpeed(); // 落下加速度の設定
    elapsedTime_ = 0.0f;                                // 経過時間を初期化
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
        float airControlFactor = 0.5f;

        // 現在の水平方向の速度をある程度維持しつつ、入力方向への変化も可能に
        float currentHorizontalSpeed = sqrt(player.GetVelocity().x * player.GetVelocity().x +
                                            player.GetVelocity().z * player.GetVelocity().z);

        // 目標速度（現在の水平速度か最大速度の小さい方）
        float targetSpeed = player.GetMaxSpeed() * airControlFactor;

        // 現在の速度ベクトル
        Vector2 currentDir = {player.GetVelocity().x, player.GetVelocity().z};
        // 入力ベクトル
        Vector2 inputDir = {xInput, zInput};

        // 内積をとって、逆方向かどうか判定
        float dot = currentDir.x * inputDir.x + currentDir.y * inputDir.y;

        if (dot < 0.0f) {
            // 逆方向なら一旦強めに減速（空中での急反転）
            player.GetVelocity().x *= 0.9f;
            player.GetVelocity().z *= 0.9f;
        }

        // 現在速度から目標速度に徐々に近づける
        float acceleration = 5.0f * Frame::DeltaTime(); // 空中での加速の強さ

        // 速度を更新
        player.GetVelocity().x = player.GetVelocity().x * (1.0f - acceleration) +
                                 xInput * targetSpeed * acceleration;
        player.GetVelocity().z = player.GetVelocity().z * (1.0f - acceleration) +
                                 zInput * targetSpeed * acceleration;
    } else {
        // 入力がない場合は徐々に減速（空気抵抗）
        player.GetVelocity().x *= 0.995f;
        player.GetVelocity().z *= 0.995f;
    }

    // 重力の適用
    if (Frame::DeltaTime() <= 1.0f) {
        player.GetVelocity().y += player.GetAcceleration().y * Frame::DeltaTime();
    }

    player.DirectionUpdate();

    // 時間経過
    elapsedTime_ += Frame::DeltaTime();

    // Spaceキーが押された & 経過時間が1秒未満ならFlyへ
    if (elapsedTime_ < 1.0f && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
        player.ChangeState("Fly");
        return;
    }

    // 接地判定とStateの変更はPlayer::Updateで行う
}

void PlayerStateAir::Exit(Player &player) {
    // 必要に応じて終了処理
}
#include "PlayerStateMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath> // ベクトル計算に必要

void PlayerStateMove::Enter(Player &player) {
    // 移動状態の初期化：着地時の衝撃を和らげるため、
    // 既に持っていた速度を一部継続させる
    // 水平速度は維持するが、最低速度を確保して滑らかに移動開始
    float currentHorizontalSpeed = sqrt(player.GetVelocity().x * player.GetVelocity().x +
                                        player.GetVelocity().z * player.GetVelocity().z);

    if (currentHorizontalSpeed < 2.0f) {
        player.GetMoveSpeed() = 2.0f; // 最低速度を設定
    } else {
        player.GetMoveSpeed() = currentHorizontalSpeed;
    }

    // 最大速度を超えないように
    if (player.GetMoveSpeed() > player.GetMaxSpeed()) {
        player.GetMoveSpeed() = player.GetMaxSpeed();
    }
}

void PlayerStateMove::Update(Player &player) {
    // 接地している場合のみジャンプ可能
    player.GetCanJump() = player.GetIsGrounded();

    // 重力をわずかに適用して地面にとどまるようにする
    player.GetVelocity().y = -0.1f;

    // 移動入力の取得
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

    // 入力があれば徐々に加速、なければ減速
    if (xInput != 0.0f || zInput != 0.0f) {
        // 斜め移動の正規化（ベクトルの長さを1に）
        float length = sqrt(xInput * xInput + zInput * zInput);
        xInput /= length;
        zInput /= length;

        // 加速処理
        player.GetMoveSpeed() += player.GetAccelRate() * Frame::DeltaTime();

        // 最大速度の制限
        if (player.GetMoveSpeed() > player.GetMaxSpeed()) {
            player.GetMoveSpeed() = player.GetMaxSpeed();
        }

        // 正規化したベクトルに速度を掛ける
        player.GetVelocity().x = xInput * player.GetMoveSpeed();
        player.GetVelocity().z = zInput * player.GetMoveSpeed();
    } else {
        // 入力がない場合は減速
        player.GetMoveSpeed() -= player.GetAccelRate() * 2.0f * Frame::DeltaTime();
        if (player.GetMoveSpeed() < 0.0f) {
            player.GetMoveSpeed() = 0.0f;
        }

        // 速度がほぼゼロになったらアイドル状態に戻る
        if (player.GetMoveSpeed() < 0.1f) {
            player.ChangeState("Idle");
            return;
        }

        // 減速中は前回の方向を維持
        player.GetVelocity().x *= 0.95f;
        player.GetVelocity().z *= 0.95f;
    }

    // ジャンプ入力があればジャンプ状態へ
    if (Input::GetInstance()->TriggerKey(DIK_SPACE) && player.GetCanJump()) {
        player.ChangeState("Jump");
        return;
    }

    // 方向の更新
    player.DirectionUpdate();
}

void PlayerStateMove::Exit(Player &player) {
    // 特に何もしない
}
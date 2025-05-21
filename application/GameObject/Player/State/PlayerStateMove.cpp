#include "PlayerStateMove.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/Camera/FollowCamera.h"
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

    // カメラを取得
    FollowCamera *camera = player.GetCamera();
    if (!camera) {
        // カメラが取得できない場合は従来の方法で処理
        DefaultMovement(player);
        return;
    }

    // カメラのViewProjectionを取得
    ViewProjection &viewProjection = camera->GetViewProjection();

    // カメラのヨー角（水平回転角）を取得
    float cameraYaw = viewProjection.rotation_.y;

    // 移動入力の取得
    float xInput = 0.0f;
    float zInput = 0.0f;

    if (Input::GetInstance()->PushKey(DIK_D)) {
        xInput += 1.0f; // 右
    }
    if (Input::GetInstance()->PushKey(DIK_A)) {
        xInput -= 1.0f; // 左
    }
    if (Input::GetInstance()->PushKey(DIK_W)) {
        zInput += 1.0f; // 前
    }
    if (Input::GetInstance()->PushKey(DIK_S)) {
        zInput -= 1.0f; // 後ろ
    }

    // 入力があれば徐々に加速、なければ減速
    if (xInput != 0.0f || zInput != 0.0f) {
        // 斜め移動の正規化（ベクトルの長さを1に）
        if (xInput != 0.0f && zInput != 0.0f) {
            float length = sqrt(xInput * xInput + zInput * zInput);
            xInput /= length;
            zInput /= length;
        }

        // 加速処理
        player.GetMoveSpeed() += player.GetAccelRate() * Frame::DeltaTime();

        // 最大速度の制限
        if (player.GetMoveSpeed() > player.GetMaxSpeed()) {
            player.GetMoveSpeed() = player.GetMaxSpeed();
        }

        // カメラの向きを基準にした移動ベクトルを計算
        // カメラのヨー角に基づいて移動方向を回転させる
        float vx = (xInput * cos(cameraYaw) + zInput * sin(cameraYaw)) * player.GetMoveSpeed();
        float vz = (-xInput * sin(cameraYaw) + zInput * cos(cameraYaw)) * player.GetMoveSpeed();

        // 速度を設定
        player.GetVelocity().x = vx;
        player.GetVelocity().z = vz;

        // 移動方向に応じてプレイヤーの向きを変更
        if (vx != 0.0f || vz != 0.0f) {
            // 移動方向に向かせる
            float targetRotation = atan2(vx, vz);

            // 現在の回転と目標回転の差を計算
            float currentRotation = player.GetWorldRotation().y;
            float rotationDiff = targetRotation - currentRotation;

            // 回転差を-πからπの範囲に正規化
            while (rotationDiff >  std::numbers::pi_v<float>)
                rotationDiff -= 2.0f *  std::numbers::pi_v<float>;
            while (rotationDiff < - std::numbers::pi_v<float>)
                rotationDiff += 2.0f *  std::numbers::pi_v<float>;

            // 回転の補間係数
            float rotationSpeed = 10.0f * Frame::DeltaTime();
            float rotationAmount = std::min(std::abs(rotationDiff), rotationSpeed);
            if (rotationDiff < 0)
                rotationAmount = -rotationAmount;

            // 回転を適用
            player.GetWorldRotation().y += rotationAmount;

            // 回転を0から2πの範囲に正規化
            while (player.GetWorldRotation().y > 2.0f *  std::numbers::pi_v<float>)
                player.GetWorldRotation().y -= 2.0f *  std::numbers::pi_v<float>;
            while (player.GetWorldRotation().y < 0.0f)
                player.GetWorldRotation().y += 2.0f *  std::numbers::pi_v<float>;
        }
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

// カメラが使用できない場合の従来の移動処理
void PlayerStateMove::DefaultMovement(Player &player) {
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
        if (xInput != 0.0f && zInput != 0.0f) {
            float length = sqrt(xInput * xInput + zInput * zInput);
            xInput /= length;
            zInput /= length;
        }

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
}

void PlayerStateMove::Exit(Player &player) {
    // 特に何もしない
}
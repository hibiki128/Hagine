#include "PlayerStateIdle.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <Frame.h>
#include <application/Utility/MotionEditor/MotionEditor.h>

void PlayerStateIdle::Enter(Player &player) {
    
}

void PlayerStateIdle::Update(Player &player) {
    // 接地している場合のみジャンプ可能
    player.GetCanJump() = player.GetIsGrounded();

    // 重力をわずかに適用して地面にとどまるようにする
    player.GetVelocity().y = -0.1f;

    // --- 減速処理（X,Z方向） ---
    float &vx = player.GetVelocity().x;
    float &vz = player.GetVelocity().z;

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

    // 完全に止まっているときは移動速度もゼロに
    if (vx == 0.0f && vz == 0.0f) {
        player.GetMoveSpeed() = 0.0f;
    }

    // --- 入力処理 ---
    if (Input::GetInstance()->PushKey(DIK_A) ||
        Input::GetInstance()->PushKey(DIK_D) ||
        Input::GetInstance()->PushKey(DIK_S) ||
        Input::GetInstance()->PushKey(DIK_W)) {
        player.ChangeState("Move");
        return;
    }

    if (Input::GetInstance()->TriggerKey(DIK_SPACE) && player.GetCanJump()) {
        player.ChangeState("Jump");
        return;
    }

    player.DirectionUpdate();
}

void PlayerStateIdle::Exit(Player &player) {
    // 特に何もしない
}
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

    player.Move();

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
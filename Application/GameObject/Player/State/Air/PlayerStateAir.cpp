#include "PlayerStateAir.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

void PlayerStateAir::Enter(Player &player) {
    player.GetAcceleration().y = player.GetFallSpeed();
    elapsedTime_ = kInitialElapsedTime;
}

void PlayerStateAir::Update(Player &player) {

    player.Move();

    if (Frame::DeltaTime() <= kMaxDeltaTime) {
        player.GetVelocity().y += player.GetAcceleration().y * Frame::DeltaTime();
    }

    player.DirectionUpdate();

    elapsedTime_ += Frame::DeltaTime();

    bool flyInput = false;

    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力
        flyInput = Input::GetInstance()->TriggerKey(DIK_SPACE);
    } else {
        // ゲームパッド入力
        flyInput = player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_A);
    }

    if (elapsedTime_ < kFlyTransitionTime && flyInput) {
        player.ChangeState("FlyIdle");
        return;
    }
}

void PlayerStateAir::Exit(Player &player) {
}
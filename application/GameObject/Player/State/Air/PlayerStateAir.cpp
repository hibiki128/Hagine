#include "PlayerStateAir.h"
#include "Engine/Frame/Frame.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

void PlayerStateAir::Enter(Player &player) {
    player.GetAcceleration().y = player.GetFallSpeed(); 
    elapsedTime_ = 0.0f;                                
}

void PlayerStateAir::Update(Player &player) {

    player.Move();

    if (Frame::DeltaTime() <= 1.0f) {
        player.GetVelocity().y += player.GetAcceleration().y * Frame::DeltaTime();
    }

    player.DirectionUpdate();

    // 時間経過
    elapsedTime_ += Frame::DeltaTime();

    if (elapsedTime_ < 1.0f && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
        player.ChangeState("FlyIdle");
        return;
    }

}

void PlayerStateAir::Exit(Player &player) {
}
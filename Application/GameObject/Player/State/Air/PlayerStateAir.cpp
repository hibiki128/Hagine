#include "PlayerStateAir.h"
#include "Engine/Frame/Frame.h"
#include "application/GameObject/Player/Player.h"

using namespace Hagine;
void PlayerStateAir::Enter(Player &player) {
    player.GetAcceleration().y = player.GetFallSpeed();
    elapsedTime_ = kInitialElapsedTime;
}

void PlayerStateAir::Update(Player &player) {
    player.Move();

    // 過大なデルタタイムによる速度の爆発を防ぐ
    if (Frame::DeltaTime() <= kMaxDeltaTime) {
        player.GetVelocity().y += player.GetAcceleration().y * Frame::DeltaTime();
    }

    player.DirectionUpdate();

    elapsedTime_ += Frame::DeltaTime();

    // 空中に出てから一定時間内に入力があれば飛行 Idle へ遷移
    if (elapsedTime_ < kFlyTransitionTime && IsJumpOrFlyTriggered(player)) {
        player.ChangeState("FlyIdle");
        return;
    }
}

void PlayerStateAir::Exit(Player &player) {
}

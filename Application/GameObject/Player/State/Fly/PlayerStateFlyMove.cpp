#define NOMINMAX
#include "PlayerStateFlyMove.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

using namespace Hagine;
void PlayerStateFlyMove::Enter(Player &player) {
    player.GetAcceleration().y = kAccelerationZero;
    player.GetVelocity().y     = kVelocityZero;
    fallInputTime_             = kInitialTime;
    fallInputCount_            = kInitialCount;

    // Rush から復帰した際などに入力カウンターをリセット
    lControlInputTime_  = 0.0f;
    lControlInputCount_ = 0;
}

void PlayerStateFlyMove::Update(Player &player) {
    player.Move();
    AirMove(player);
    ChangeStateLogic(player);
    fallInputTime_ += player.GetDt();
    player.DirectionUpdate();
}

void PlayerStateFlyMove::Exit(Player &player) {
}

void PlayerStateFlyMove::AirMove(Player &player) {
    float &vy = player.GetVelocity().y;

    if (IsAscendInput(player)) {
        // 上昇入力中：Y速度を加速して上限でクランプ
        vy = std::min(vy + kFlyAcceleration * player.GetDt(), kFlyMaxSpeed);
    } else if (IsDescendInput(player)) {
        // 下降入力中：Y速度を逆方向に加速して下限でクランプ
        vy = std::max(vy - kFlyAcceleration * player.GetDt(), -kFlyMaxSpeed);
    } else {
        // 入力なし：減衰させてゼロに収束させる
        vy *= kVelocityDampingFactor;
        if (std::abs(vy) < kVelocityStopThreshold) {
            vy = kVelocityZero;
        }
    }

    // 下降入力のトリガーをカウント（短時間に 2 回で落下モードへ）
    if (IsDescendTriggered(player)) {
        if (fallInputTime_ < kFallThresholdTime) {
            fallInputCount_++;
        } else {
            fallInputCount_ = kInitialCount;
        }
        fallInputTime_ = kInitialTime;
    }
}

void PlayerStateFlyMove::ChangeStateLogic(Player &player) {
    TryChangeToRush(player);

    if (player.GetCurrentStateName() == "Rush") {
        return;
    }

    // 下降入力が閾値回数に達したら物理落下状態（Air）へ遷移
    if (fallInputCount_ >= kFallInputThreshold) {
        player.ChangeState("Air");
        fallInputCount_ = kInitialCount;
        return;
    }

    // 地面に着地したら地上 Idle へ遷移
    if (player.GetWorldTransform() && player.GetLocalPosition().y <= kGroundLevel) {
        player.ChangeState("Idle");
        return;
    }

    // 一定時間入力がなければ飛行 Idle へ遷移
    if (!HasFlyAnyInput(player) && fallInputTime_ > kFallThresholdTime) {
        player.ChangeState("FlyIdle");
        return;
    }

    player.ChangeEnergyCharge();
}

void PlayerStateFlyMove::TryChangeToRush(Player &player) {
    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力: LCtrl を 2 回押しで Rush に遷移
        if (Input::GetInstance()->TriggerKey(DIK_LCONTROL)) {
            lControlInputCount_++;
            if (lControlInputCount_ == 1) {
                lControlInputTime_ = 0.0f;
            } else if (lControlInputCount_ >= 2 && player.GetIsLockOn() && player.GetEnemy()) {
                if (player.ConsumeEnergy(kRushEnergyCost)) {
                    player.ChangeState("Rush");
                }
                lControlInputCount_ = 0;
                return;
            }
        }

        // 一定時間内に 2 回入力されなければリセット
        if (lControlInputCount_ > 0) {
            lControlInputTime_ += player.GetDt();
            if (lControlInputTime_ >= kInputResetTime) {
                lControlInputCount_ = 0;
                lControlInputTime_  = 0.0f;
            }
        }
    } else {
        // ゲームパッド入力: ダッシュ中かつロックオン中の A ボタントリガーで Rush に遷移
        if (player.GetIsDashing() && player.GetIsLockOn() && player.GetEnemy()) {
            if (!player.GetDashStartedThisFrame() &&
                player.GetDashDuration() > kDashRushMinDuration &&
                player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_A)) {

                if (player.ConsumeEnergy(kRushEnergyCost)) {
                    player.ChangeState("Rush");
                    player.ClearDashState();
                }
                return;
            }
        }
    }
}

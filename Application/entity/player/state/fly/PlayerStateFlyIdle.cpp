#define NOMINMAX
#include "PlayerStateFlyIdle.h"
#include "Input.h"
#include "application/entity/field/ground/Ground.h"
#include "application/entity/player/Player.h"
#include <cmath>

using namespace Hagine;
void PlayerStateFlyIdle::Enter(Player &player)
{
    player.GetAcceleration().y = kAccelerationZero;
    player.GetVelocity().y = kVelocityZero;

    // Rush から復帰した際などに入力カウンターをリセット
    lControlInputTime_ = 0.0f;
    lControlInputCount_ = 0;
}

void PlayerStateFlyIdle::Update(Player &player)
{
    AirMove(player);
    ChangeStateLogic(player);
    player.DirectionUpdate();
}

void PlayerStateFlyIdle::Exit(Player &player)
{
}

void PlayerStateFlyIdle::AirMove(Player &player)
{
    // 全方向の速度を減衰させ、閾値以下になったらゼロにする
    float &vx = player.GetVelocity().x;
    float &vz = player.GetVelocity().z;
    float &vy = player.GetVelocity().y;

    auto damp = [](float &v, float threshold, float factor) {
        if (std::abs(v) < threshold)
        {
            v = 0.0f;
        }
        else
        {
            v *= factor;
        }
    };

    damp(vx, kVelocityStopThreshold, kDampingFactor);
    damp(vz, kVelocityStopThreshold, kDampingFactor);
    damp(vy, kVelocityStopThreshold, kDampingFactor);

    if (vx == kVelocityZero && vz == kVelocityZero && vy == kVelocityZero)
    {
        player.GetMoveSpeed() = kMoveSpeedZero;
    }
}

void PlayerStateFlyIdle::ChangeStateLogic(Player &player)
{
    // 静止（ホバリング）中でも「A → 少し遅れてスティック」でダッシュを開始できるようにする。
    // FlyIdle は Move() を呼ばないため、ここで A トリガーのダッシュ開始だけを拾って
    // 実移動を扱う FlyMove へ移す（開始直後の速度付与・維持は FlyMove の Move() が担当）。
    if (player.TryStartDash())
    {
        player.ChangeState("FlyMove");
        return;
    }

    TryChangeToRush(player);

    if (player.GetCurrentStateName() == "Rush")
    {
        return;
    }

    // 地面に着いたら地上 Idle へ遷移（地形メッシュの高さ基準）
    if (player.GetLocalPosition().y <= Ground::GetStandingY(player.GetLocalPosition().x, player.GetLocalPosition().z))
    {
        player.ChangeState("Idle");
        return;
    }

    // 何らかの入力があれば飛行移動状態へ遷移
    if (HasFlyAnyInput(player))
    {
        player.ChangeState("FlyMove");
        return;
    }

    player.ChangeEnergyCharge();
}

void PlayerStateFlyIdle::TryChangeToRush(Player &player)
{
    if (!player.GetGamePad()->IsConnected())
    {
        // キーボード入力: LCtrl を 2 回押しで Rush に遷移
        if (Input::GetInstance()->TriggerKey(DIK_LCONTROL))
        {
            lControlInputCount_++;
            if (lControlInputCount_ == 1)
            {
                lControlInputTime_ = 0.0f;
            }
            else if (lControlInputCount_ >= 2 && player.GetIsLockOn() && player.GetEnemy())
            {
                if (player.ConsumeEnergy(kRushEnergyCost))
                {
                    player.ChangeState("Rush");
                }
                lControlInputCount_ = 0;
                return;
            }
        }

        // 一定時間内に 2 回入力されなければリセット
        if (lControlInputCount_ > 0)
        {
            lControlInputTime_ += player.GetDt();
            if (lControlInputTime_ >= kInputResetTime)
            {
                lControlInputCount_ = 0;
                lControlInputTime_ = 0.0f;
            }
        }
    }
    else
    {
        // ゲームパッド入力: ダッシュ中かつロックオン中の A ボタントリガーで Rush に遷移
        if (player.GetIsDashing() && player.GetIsLockOn() && player.GetEnemy())
        {
            if (!player.GetDashStartedThisFrame() &&
                player.GetDashDuration() > kDashRushMinDuration &&
                player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_A))
            {

                if (player.ConsumeEnergy(kRushEnergyCost))
                {
                    player.ChangeState("Rush");
                    player.ClearDashState();
                }
                return;
            }
        }
    }
}

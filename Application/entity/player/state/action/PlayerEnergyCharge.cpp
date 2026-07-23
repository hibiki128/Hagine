#include "PlayerEnergyCharge.h"
#include "application/entity/player/Player.h"
#include <Input.h>
#include <particle/gpu/ParticleCSSpawner.h>

using namespace Hagine;
void PlayerEnergyCharge::Enter(Player &player)
{
    beforeChargeRate_ = player.GetChargeRate();
    beforeState_ = player.GetPreviewStateName();
    player.GetVelocity() = {kVelocityZero, kVelocityZero, kVelocityZero};
    player.SetEnergyRecoveryRate(chargeRate_);

    // チャージ開始ごとに Spawn する。更新・描画はエンジンが自動で回す。
    chargeAuraEmitter_ = ParticleCSSpawner::GetInstance()->Spawn("playerAura");
}

void PlayerEnergyCharge::Update(Player &player)
{
    if (chargeAuraEmitter_)
    {
        chargeAuraEmitter_->SetTranslate({player.GetWorldPosition().x, player.GetWorldPosition().y + chargeAuraEmitter_->GetScale().y - 1.0f, player.GetWorldPosition().z});
        chargeAuraEmitter_->SetRotation(player.GetWorldRotation());
        chargeAuraEmitter_->SetAuto(true);
    }

    bool chargeRelease = false;
    bool shouldExitCharge = false;

    if (!player.GetGamePad()->IsConnected())
    {
        // キーボード入力
        chargeRelease = Input::GetInstance()->ReleaseKey(DIK_C);
    }
    else
    {
        // ゲームパッド入力
        bool isLTHeld = player.GetGamePad()->GetLeftTrigger() > 0.25f;

        // LTが押されていない場合はチャージ解除
        if (!isLTHeld)
        {
            shouldExitCharge = true;
        }
        else
        {
            // LT押しながらAボタンが押された場合、ダッシュフラグを立ててチャージ解除
            if (player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_A))
            {
                float leftStickX = -player.GetGamePad()->GetLeftStickX();
                float leftStickY = player.GetGamePad()->GetLeftStickY();

                // ダッシュ入力を記録
                player.SetDashInput(leftStickX, leftStickY);
                player.SetDashing(true);
                shouldExitCharge = true;
            }

            // Yボタンが押された時、スキルが実際に打てる場合のみチャージを中断
            if (player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_Y))
            {
                const float kSkillShotEnergyCost = 65.0f;
                if (player.GetEnergy() >= kSkillShotEnergyCost)
                {
                    shouldExitCharge = true;
                }
            }
        }
    }

    // チャージ解除条件
    if (chargeRelease || shouldExitCharge || player.GetEnergy() >= player.GetMaxEnergy())
    {
        if (beforeState_ == "Idle" ||
            beforeState_ == "Move")
        {
            player.ChangeState("Idle");
        }
        if (beforeState_ == "FlyIdle" ||
            beforeState_ == "FlyMove")
        {
            player.ChangeState("FlyIdle");
        }
    }
}

void PlayerEnergyCharge::Exit(Player &player)
{
    player.SetEnergyRecoveryRate(beforeChargeRate_);
    // 発生を止め、残った粒子が消えたら Spawner が自動で破棄する。
    // 借用ポインタは無効になるので手放しておく（次のチャージ開始で再 Spawn する）。
    if (chargeAuraEmitter_)
    {
        ParticleCSSpawner::GetInstance()->DespawnWhenFinished(chargeAuraEmitter_);
        chargeAuraEmitter_ = nullptr;
    }
}
#include "PlayerEnergyCharge.h"
#include "application/GameObject/Player/Player.h"
#include <Input.h>
#include <Particle/CSParticle/ParticleCSEditor.h>

void PlayerEnergyCharge::Enter(Player &player) {
    beforeChargeRate_ = player.GetChargeRate();
    beforeState_ = player.GetPreviewStateName();
    player.GetVelocity() = {kVelocityZero, kVelocityZero, kVelocityZero};
    player.SetEnergyRecoveryRate(chargeRate_);

    chargeAuraEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("ChargeAura");
}

void PlayerEnergyCharge::Update(Player &player) {
    chargeAuraEmitter_->Update();
    chargeAuraEmitter_->SetTranslate({player.GetWorldPosition().x, player.GetWorldPosition().y + kParticleYOffset, player.GetWorldPosition().z});
    chargeAuraEmitter_->SetAuto(true);

    bool chargeRelease = false;
    bool shouldExitCharge = false;

    if (!player.GetGamePad()->IsConnected()) {
        // キーボード入力
        chargeRelease = Input::GetInstance()->ReleaseKey(DIK_C);
    } else {
        // ゲームパッド入力

        // Yボタンが押された時、スキルが実際に打てる場合のみチャージを中断
        if (player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_Y)) {
            const float kSkillShotEnergyCost = 65.0f;
            if (player.GetEnergy() >= kSkillShotEnergyCost) {
                shouldExitCharge = true;
            }
        }

        // Aボタンが押された時、左スティック入力がある場合のみチャージを中断
        if (player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_A)) {
            float leftStickX = player.GetGamePad()->GetLeftStickX();
            float leftStickY = player.GetGamePad()->GetLeftStickY();

            if (leftStickX != 0.0f || leftStickY != 0.0f) {
                shouldExitCharge = true;
            }
        }

        // RTトリガーのリリース検出
        static bool wasRTPressed = false;
        bool isRTPressed = player.GetGamePad()->GetRightTrigger() > 0.25f;

        chargeRelease = wasRTPressed && !isRTPressed;

        wasRTPressed = isRTPressed;
    }

    // チャージ解除条件
    if (chargeRelease || shouldExitCharge || player.GetEnergy() >= player.GetMaxEnergy()) {
        if (beforeState_ == "Idle" ||
            beforeState_ == "Move") {
            player.ChangeState("Idle");
        }
        if (beforeState_ == "FlyIdle" ||
            beforeState_ == "FlyMove") {
            player.ChangeState("FlyIdle");
        }
    }
}

void PlayerEnergyCharge::Exit(Player &player) {
    player.SetEnergyRecoveryRate(beforeChargeRate_);
    chargeAuraEmitter_->SetAuto(false);
    chargeAuraEmitter_->Update();
    ParticleCSEmitter::ClearNameCounter("ChargeAura");
}

void PlayerEnergyCharge::DrawParticle(Player &player, const ViewProjection &viewProjection) {

    if (chargeAuraEmitter_) {
        chargeAuraEmitter_->Draw(viewProjection);
    }
}
#include "PlayerEnergyCharge.h"
#include "application/GameObject/Player/Player.h"
#include <Input.h>
#include <Particle/CSParticle/ParticleCSEditor.h>

void PlayerEnergyCharge::Enter(Player &player) {
    beforeChargeRate_ = player.GetChargeRate();
    beforeState_ = player.GetPreviewStateName();
    player.GetVelocity() = {0.0f, 0.0f, 0.0f};
    player.SetEnergyRecoveryRate(chargeRate_);

    chargeAuraEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("ChargeAura");
}

void PlayerEnergyCharge::Update(Player &player) {
    chargeAuraEmitter_->Update();
    chargeAuraEmitter_->SetTranslate({player.GetWorldPosition().x, player.GetWorldPosition().y - 1.5f, player.GetWorldPosition().z});
    chargeAuraEmitter_->SetAuto(true);
    if (Input::GetInstance()->ReleaseKey(DIK_C) ||
        player.GetEnergy() >= player.GetMaxEnergy()) {
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

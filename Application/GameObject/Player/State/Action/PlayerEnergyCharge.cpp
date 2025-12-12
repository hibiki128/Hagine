#include "PlayerEnergyCharge.h"
#include "application/GameObject/Player/Player.h"
#include <Input.h>

void PlayerEnergyCharge::Enter(Player &player) {
    beforeChargeRate_ = player.GetChargeRate();
    beforeState_ = player.GetPreviewStateName();
    player.GetVelocity() = {0.0f, 0.0f, 0.0f};
    player.SetEnergyRecoveryRate(chargeRate_);
}

void PlayerEnergyCharge::Update(Player &player) {
    if (Input::GetInstance()->ReleaseKey(DIK_C)||
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
}

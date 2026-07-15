#define NOMINMAX
#include "PlayerStatus.h"
#include "Application/entity/player/Player.h"
#include <data/DataHandler.h>
#include <utility/debug/param/GameParamHub.h>
#include <cmath>

using namespace Hagine;

void PlayerStatus::Init(Player *owner)
{
    pOwner_ = owner;
}

void PlayerStatus::DamageUpdate()
{
    if (damage_ <= kNoDamage)
    {
        return;
    }

    // 無敵中はダメージを無視
    if (isInvincible_)
    {
        damage_ = kNoDamage;
        return;
    }

    // ガード中はダメージ・ノックバックを軽減し、エネルギーを消費する
    float guardMult = isGuarding_ ? guardDamageMultiplier_ : 1.0f;
    if (isGuarding_)
    {
        ConsumeEnergy(guardEnergyCost_);
    }

    HP_ -= damage_ * guardMult;
    damage_ = kNoDamage;

    if (hasKnockback_)
    {
        Vector3 &velocity = pOwner_->GetVelocity();
        velocity.x += knockbackVelocity_.x * guardMult;
        velocity.y += knockbackVelocity_.y * guardMult;
        velocity.z += knockbackVelocity_.z * guardMult;
        if (pOwner_->GetIsGrounded() && knockbackVelocity_.y > 0.0f)
        {
            pOwner_->GetIsGrounded() = false;
        }
        hasKnockback_ = false;
        knockbackVelocity_ = {0.0f, 0.0f, 0.0f};
    }

    // 無敵時間の開始
    isInvincible_ = true;
    invincibleTime_ = kTimerReset;

    // ダメージリアクションの開始（点滅のみ）
    isDamageReact_ = true;
    damageReactTimer_ = kTimerReset;
}

void PlayerStatus::InvincibleUpdate()
{
    invincibleTime_ += pOwner_->GetDt();
    if (invincibleTime_ >= invincibleDuration_)
    {
        isInvincible_ = false;
        invincibleTime_ = kTimerReset;
    }
}

void PlayerStatus::UpdateDamageReact()
{
    if (!isDamageReact_)
    {
        return;
    }

    damageReactTimer_ += pOwner_->GetDt();

    // 高速点滅
    float blinkInterval = kPlayerBlinkInterval;
    int blink = static_cast<int>(damageReactTimer_ / blinkInterval);
    pOwner_->SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kPlayerAlphaTransparent : kAlphaOpaque);

    // 終了処理
    if (damageReactTimer_ >= damageReactDuration_)
    {
        isDamageReact_ = false;
        pOwner_->SetAlpha(kAlphaOpaque);
    }
}

void PlayerStatus::StopDamageReact()
{
    isDamageReact_ = false;
    pOwner_->SetAlpha(kAlphaOpaque);
}

bool PlayerStatus::ConsumeEnergy(float amount)
{
    if (energy_ >= amount)
    {
        energy_ -= amount;
        timeSinceLastShot_ = kTimerReset;
        return true;
    }
    return false;
}

void PlayerStatus::RecoverEnergy()
{
    bool canRecover = false;
    const float dt = pOwner_->GetDt();

    // エナジーチャージ中なら即回復
    if (pOwner_->GetCurrentStateName() == "EnergyCharge")
    {
        canRecover = true;
    }
    // EnergyCharge チュートリアルステップ中は自動回復を停止する
    else if (pOwner_->GetTutorialStep() != TutorialStep::EnergyCharge)
    {
        timeSinceLastShot_ += dt;
        if (timeSinceLastShot_ >= energyRecoveryDelay_)
        {
            canRecover = true;
        }
    }

    if (canRecover)
    {
        energy_ += energyRecoveryRate_ * dt;
        if (energy_ > maxEnergy_)
        {
            energy_ = maxEnergy_;
        }
    }
}

void PlayerStatus::SetKnockback(const Vector3 &direction, float power)
{
    if (power <= 0.0f)
        return;

    Vector3 dir = direction;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f)
        return;

    dir.x /= len;
    dir.y /= len;
    dir.z /= len;

    // 水平方向 + 少し上方に浮かせる
    knockbackVelocity_ = {
        dir.x * power,
        power * 0.25f,
        dir.z * power,
    };
    hasKnockback_ = true;
}

void PlayerStatus::Save(DataHandler *data)
{
    data->Save("maxEnergy", maxEnergy_);
    data->Save("energyRecoveryRate", energyRecoveryRate_);
    data->Save("energyRecoveryDelay", energyRecoveryDelay_);
    data->Save("invincibleDuration", invincibleDuration_);
    data->Save("guardDamageMultiplier", guardDamageMultiplier_);
    data->Save("guardEnergyCost", guardEnergyCost_);
}

void PlayerStatus::Load(DataHandler *data)
{
    maxEnergy_ = data->Load<float>("maxEnergy", 100.0f);
    energyRecoveryRate_ = data->Load<float>("energyRecoveryRate", 0.01f);
    energyRecoveryDelay_ = data->Load<float>("energyRecoveryDelay", 1.0f);
    energy_ = maxEnergy_; // 初期化時は最大値
    invincibleDuration_ = data->Load<float>("invincibleDuration", 0.25f);
    guardDamageMultiplier_ = data->Load<float>("guardDamageMultiplier", 0.20f);
    guardEnergyCost_ = data->Load<float>("guardEnergyCost", 10.0f);
}

void PlayerStatus::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Text("エネルギー: %.1f / %.1f", energy_, maxEnergy_);
    ImGui::DragFloat("エネルギー回復速度", &energyRecoveryRate_, 0.1f, 0.0f, 50.0f);
    ImGui::DragFloat("回復開始遅延", &energyRecoveryDelay_, 0.1f, 0.0f, 5.0f);

    ImGui::Text("無敵状態: %s", isInvincible_ ? "True" : "False");
    if (isInvincible_)
    {
        ImGui::Text("無敵残り時間: %.2f秒", invincibleDuration_ - invincibleTime_);
    }
    ImGui::DragFloat("無敵時間", &invincibleDuration_, 0.01f, 0.0f, 2.0f);

    ImGui::Separator();
    ImGui::Text("ガード設定");
    ImGui::Text("  現在状態: %s", isGuarding_ ? "ガード中" : "解除中");
    // 0.0=完全無敵, 1.0=ガード意味なし。軽減率 = (1 - multiplier)*100 %
    ImGui::DragFloat("ガード被ダメ倍率 (0=無敵, 1=無効)", &guardDamageMultiplier_, 0.01f, 0.0f, 1.0f);
    ImGui::Text("  -> 軽減率 %.0f%%", (1.0f - guardDamageMultiplier_) * 100.0f);
    ImGui::DragFloat("ガード時エネルギー消費", &guardEnergyCost_, 0.5f, 0.0f, 100.0f);
#endif // USE_IMGUI
}

void PlayerStatus::RegisterParams()
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Player", "HP", static_cast<const float *>(&HP_));
    hub->Register("Player", "エネルギー", static_cast<const float *>(&energy_));
    hub->Register("Player", "最大エネルギー", &maxEnergy_, {1.0f, 1.0f, 500.0f});
    hub->Register("Player", "エネルギー回復速度", &energyRecoveryRate_, {0.1f, 0.0f, 50.0f});
    hub->Register("Player", "回復開始遅延", &energyRecoveryDelay_, {0.1f, 0.0f, 5.0f});
    hub->Register("Player", "無敵時間", &invincibleDuration_, {0.01f, 0.0f, 2.0f});
    hub->Register("Player", "ガード被ダメ倍率", &guardDamageMultiplier_, {0.01f, 0.0f, 1.0f});
    hub->Register("Player", "ガード時エネルギー消費", &guardEnergyCost_, {0.5f, 0.0f, 100.0f});
}

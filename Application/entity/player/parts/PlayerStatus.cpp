#define NOMINMAX
#include "PlayerStatus.h"
#include "Application/entity/player/Player.h"
#include <data/DataHandler.h>
#include <utility/debug/param/GameParamHub.h>
#include <cmath>
#include <cstdlib>

using namespace Hagine;

void PlayerStatus::Init(Player *pOwner)
{
    pOwner_ = pOwner;
}

void PlayerStatus::DamageUpdate()
{
    if (damage_ <= kNoDamage)
    {
        // ダメージが無いフレームに予約が残らないようにする
        blowPending_ = false;
        blowGrantsFlinchImmunity_ = false;
        skillBlowPending_ = false;
        return;
    }

    // 無敵中はダメージを無視する。ただし必殺技の被弾（大スタン）だけは通す
    if (isInvincible_ && !skillBlowPending_)
    {
        damage_ = kNoDamage;
        damageIsShot_ = false;
        damageIsSkill_ = false;
        blowPending_ = false;
        blowGrantsFlinchImmunity_ = false;
        return;
    }

    // ガード中はダメージ・ノックバックを軽減し、エネルギーを消費する（必殺技は消費量が大きい）
    float guardMult = isGuarding_ ? guardDamageMultiplier_ : 1.0f;
    if (isGuarding_)
    {
        // 残量が足りなくてもゼロまで削る（必殺技のガードを無償にしない）
        DrainEnergy(damageIsSkill_ ? guardSkillEnergyCost_ : guardEnergyCost_);
    }

    // 必殺技被弾スタン中は行動できず無防備なので、追撃のダメージを軽減する
    const bool inSkillBlow = (reactState_ == PlayerReactState::SkillBlow) && !skillBlowPending_;
    float actualDamage = damage_ * guardMult;
    if (inSkillBlow)
    {
        actualDamage *= skillBlowDamageMultiplier_;
    }

    hp_ -= actualDamage;
    damage_ = kNoDamage;

    // ダメージリアクションの開始（点滅のみ）
    isDamageReact_ = true;
    damageReactTimer_ = 0.0f;

    // スタン中の追撃は落下の軌道を乱さないよう、ノックバックもリアクション変更も行わない
    if (inSkillBlow)
    {
        hasKnockback_ = false;
        knockbackVelocity_ = {0.0f, 0.0f, 0.0f};
        blowPending_ = false;
        blowGrantsFlinchImmunity_ = false;
        damageIsShot_ = false;
        damageIsSkill_ = false;
        return;
    }

    // 無敵時間の開始
    isInvincible_ = true;
    invincibleTime_ = 0.0f;

    // 必殺技被弾：吹き飛ばして落下スタンへ移行する（他のリアクションより優先）
    if (skillBlowPending_)
    {
        StartSkillBlow();
        damageIsShot_ = false;
        damageIsSkill_ = false;
        return;
    }

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

    // 被弾リアクション（ひるみ・吹き飛ばし）。ガード成立時は行動不能にしない
    if (!isGuarding_)
    {
        if (blowPending_)
        {
            // 吹き飛ばし：着地するまでBlowBack、着地でBlowAfterへ
            reactState_ = PlayerReactState::Blow;
            blowLanded_ = false;
            blowAfterTimer_ = 0.0f;
            blowTimer_ = 0.0f;
            flinchImmuneTimer_ = 0.0f; // 吹き飛ばし中は無効時間を進めない（復帰時に張り直す）
            blowImmunityArmed_ = blowGrantsFlinchImmunity_;
            pOwner_->GetIsGrounded() = false;
            // ガード・ダッシュ等のステートを抜けて空中扱いにする（復帰時に整合を取る）
            pOwner_->ChangeState("Air");
        }
        else if (flinchImmuneTimer_ <= 0.0f)
        {
            // ひるみ：時間経過で回復。被弾ごとにアニメをランダム選択（射撃は近接より短い）
            reactState_ = PlayerReactState::Flinch;
            hitStunTimer_ = damageIsShot_ ? hitStunDuration_ * shotFlinchScale_ : hitStunDuration_;
            flinchAnimIndex_ = 1 + std::rand() % 3;
        }
    }
    blowPending_ = false;
    blowGrantsFlinchImmunity_ = false;
    damageIsShot_ = false;
    damageIsSkill_ = false;
}

void PlayerStatus::RequestSkillBlowReaction(const Vector3 &direction)
{
    skillBlowPending_ = true;

    Vector3 dir = {direction.x, 0.0f, direction.z};
    const float len = std::sqrt(dir.x * dir.x + dir.z * dir.z);
    if (len < 0.001f)
    {
        // 方向が取れないときはプレイヤーの背面方向へ飛ばす
        dir = pOwner_->GetForward();
        dir = {-dir.x, 0.0f, -dir.z};
        const float fallbackLen = std::sqrt(dir.x * dir.x + dir.z * dir.z);
        if (fallbackLen < 0.001f)
        {
            skillBlowDirection_ = {0.0f, 0.0f, 0.0f};
            return;
        }
        dir.x /= fallbackLen;
        dir.z /= fallbackLen;
    }
    else
    {
        dir.x /= len;
        dir.z /= len;
    }
    skillBlowDirection_ = dir;
}

void PlayerStatus::StartSkillBlow()
{
    reactState_ = PlayerReactState::SkillBlow;
    skillBlowTimer_ = 0.0f;
    blowLanded_ = false;
    blowAfterTimer_ = 0.0f;
    blowPending_ = false;
    blowGrantsFlinchImmunity_ = false;
    skillBlowPending_ = false;

    // 起き上がり直後の連続被弾を防ぐため、復帰後は必ずひるみ無効時間を与える
    blowImmunityArmed_ = true;
    flinchImmuneTimer_ = 0.0f;

    // 予約済みのノックバックは使わず、必殺技専用の吹き飛ばし速度で上書きする
    hasKnockback_ = false;
    knockbackVelocity_ = {0.0f, 0.0f, 0.0f};

    Vector3 &velocity = pOwner_->GetVelocity();
    velocity.x = skillBlowDirection_.x * skillBlowSpeed_;
    velocity.z = skillBlowDirection_.z * skillBlowSpeed_;
    velocity.y = skillBlowRiseSpeed_; // 一度浮かせてから落下させる

    pOwner_->GetIsGrounded() = false;
    pOwner_->ClearDashState();
    pOwner_->ChangeState("Air");
}

void PlayerStatus::UpdateSkillBlow(float deltaTime)
{
    Vector3 &velocity = pOwner_->GetVelocity();

    if (!blowLanded_)
    {
        skillBlowTimer_ += deltaTime;

        // 横移動の勢いを保ったまま徐々に減速させる（フレームレート非依存の指数減衰）
        const float damping = std::pow(skillBlowHorizontalRetain_, deltaTime);
        velocity.x *= damping;
        velocity.z *= damping;
        ApplyBlowGravity(deltaTime);

        if (pOwner_->GetIsGrounded())
        {
            // 地面に落ちたら着地硬直（BlowAfter）へ
            blowLanded_ = true;
            blowAfterTimer_ = blowAfterDuration_;
        }
        else if (skillBlowTimer_ >= skillBlowMaxDuration_)
        {
            // 地形の穴などで落ち続けた場合の安全策
            RecoverFromBlow(blowImmunityArmed_);
        }
        return;
    }

    blowAfterTimer_ -= deltaTime;
    if (blowAfterTimer_ <= 0.0f)
    {
        RecoverFromBlow(blowImmunityArmed_);
    }
}

void PlayerStatus::ApplyBlowGravity(float deltaTime)
{
    // 吹き飛ばし中はステート更新が止まっているため、ここで落下させる
    if (!pOwner_->GetIsGrounded())
    {
        pOwner_->GetVelocity().y -= blowGravity_ * deltaTime;
    }
}

void PlayerStatus::RecoverFromBlow(bool grantFlinchImmunity)
{
    // 残速度で滑り続けないよう、速度を消してから通常ステートへ戻す
    reactState_ = PlayerReactState::None;
    pOwner_->GetVelocity() = {0.0f, 0.0f, 0.0f};
    pOwner_->ChangeState(pOwner_->GetIsGrounded() ? "Idle" : "Air");

    if (grantFlinchImmunity)
    {
        flinchImmuneTimer_ = flinchImmuneDuration_;
    }
    blowImmunityArmed_ = false;
}

bool PlayerStatus::ConsumeGuardDeflect()
{
    if (!isGuarding_)
    {
        return false;
    }
    // 弾き返しも通常のガードと同じだけエネルギーを消費する
    ConsumeEnergy(guardEnergyCost_);
    return true;
}

void PlayerStatus::UpdateReaction()
{
    const float dt = pOwner_->GetDt();

    // ひるみ無効時間の消化
    if (flinchImmuneTimer_ > 0.0f)
    {
        flinchImmuneTimer_ -= dt;
        if (flinchImmuneTimer_ < 0.0f)
        {
            flinchImmuneTimer_ = 0.0f;
        }
    }

    if (reactState_ == PlayerReactState::SkillBlow)
    {
        UpdateSkillBlow(dt);
        return;
    }

    if (reactState_ == PlayerReactState::Flinch)
    {
        hitStunTimer_ -= dt;
        if (hitStunTimer_ <= 0.0f)
        {
            hitStunTimer_ = 0.0f;
            reactState_ = PlayerReactState::None;
        }
        return;
    }

    if (reactState_ != PlayerReactState::Blow)
    {
        return;
    }

    if (!blowLanded_)
    {
        ApplyBlowGravity(dt);

        // 空中に取り残されないよう、着地しなくても一定時間で強制復帰させる安全策
        blowTimer_ += dt;
        if (blowTimer_ >= blowMaxDuration_)
        {
            RecoverFromBlow(blowImmunityArmed_);
            return;
        }

        // 吹き飛ばされ中。地面に着いたら着地硬直（BlowAfter）へ
        if (pOwner_->GetIsGrounded())
        {
            blowLanded_ = true;
            blowAfterTimer_ = blowAfterDuration_;
        }
        return;
    }

    blowAfterTimer_ -= dt;
    if (blowAfterTimer_ <= 0.0f)
    {
        RecoverFromBlow(blowImmunityArmed_);
    }
}

void PlayerStatus::InvincibleUpdate()
{
    invincibleTime_ += pOwner_->GetDt();
    if (invincibleTime_ >= invincibleDuration_)
    {
        isInvincible_ = false;
        invincibleTime_ = 0.0f;
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
        timeSinceLastShot_ = 0.0f;
        return true;
    }
    return false;
}

void PlayerStatus::DrainEnergy(float amount)
{
    energy_ = (energy_ > amount) ? energy_ - amount : 0.0f;
    timeSinceLastShot_ = 0.0f;
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

void PlayerStatus::SetKnockbackDirect(const Vector3 &velocity)
{
    // 上方成分の固定加算を行わず、指定した速度をそのまま適用する（叩きつけ等に使用）
    knockbackVelocity_ = velocity;
    hasKnockback_ = true;
}

void PlayerStatus::Save(DataHandler *pData)
{
    pData->Save("maxEnergy", maxEnergy_);
    pData->Save("energyRecoveryRate", energyRecoveryRate_);
    pData->Save("energyRecoveryDelay", energyRecoveryDelay_);
    pData->Save("invincibleDuration", invincibleDuration_);
    pData->Save("guardDamageMultiplier", guardDamageMultiplier_);
    pData->Save("guardEnergyCost", guardEnergyCost_);
    pData->Save("guardSkillEnergyCost", guardSkillEnergyCost_);
    pData->Save("hitStunDuration", hitStunDuration_);
}

void PlayerStatus::Load(DataHandler *pData)
{
    maxEnergy_ = pData->Load<float>("maxEnergy", 100.0f);
    energyRecoveryRate_ = pData->Load<float>("energyRecoveryRate", 0.01f);
    energyRecoveryDelay_ = pData->Load<float>("energyRecoveryDelay", 1.0f);
    energy_ = maxEnergy_; // 初期化時は最大値
    invincibleDuration_ = pData->Load<float>("invincibleDuration", 0.25f);
    guardDamageMultiplier_ = pData->Load<float>("guardDamageMultiplier", 0.20f);
    guardEnergyCost_ = pData->Load<float>("guardEnergyCost", 3.0f);
    guardSkillEnergyCost_ = pData->Load<float>("guardSkillEnergyCost", 15.0f);
    hitStunDuration_ = pData->Load<float>("hitStunDuration", 0.5f);
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
    ImGui::DragFloat("ガード時エネルギー消費(必殺技)", &guardSkillEnergyCost_, 0.5f, 0.0f, 100.0f);

    ImGui::Separator();
    ImGui::Text("被弾リアクション");
    const char *reactName = "-";
    switch (reactState_)
    {
    case PlayerReactState::Flinch:
        reactName = "ひるみ";
        break;
    case PlayerReactState::Blow:
        reactName = blowLanded_ ? "吹き飛ばし(着地後)" : "吹き飛ばし";
        break;
    case PlayerReactState::SkillBlow:
        reactName = blowLanded_ ? "必殺技スタン(着地後)" : "必殺技スタン";
        break;
    default:
        break;
    }
    ImGui::Text("  現在状態: %s", reactName);
    ImGui::DragFloat("ひるみ時間", &hitStunDuration_, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("吹き飛ばし着地後硬直", &blowAfterDuration_, 0.01f, 0.0f, 2.0f);
    ImGui::DragFloat("吹き飛ばし中の落下加速度", &blowGravity_, 0.5f, 1.0f, 100.0f);
#endif // USE_IMGUI
}

void PlayerStatus::RegisterParams()
{
    auto *pHub = GameParamHub::GetInstance();
    pHub->Register("Player", "HP", static_cast<const float *>(&hp_));
    pHub->Register("Player", "エネルギー", static_cast<const float *>(&energy_));
    pHub->Register("Player", "最大エネルギー", &maxEnergy_, {1.0f, 1.0f, 500.0f});
    pHub->Register("Player", "エネルギー回復速度", &energyRecoveryRate_, {0.1f, 0.0f, 50.0f});
    pHub->Register("Player", "回復開始遅延", &energyRecoveryDelay_, {0.1f, 0.0f, 5.0f});
    pHub->Register("Player", "無敵時間", &invincibleDuration_, {0.01f, 0.0f, 2.0f});
    pHub->Register("Player", "ガード被ダメ倍率", &guardDamageMultiplier_, {0.01f, 0.0f, 1.0f});
    pHub->Register("Player", "ガード時エネルギー消費", &guardEnergyCost_, {0.5f, 0.0f, 100.0f});
    pHub->Register("Player", "ガード時エネルギー消費(必殺技)", &guardSkillEnergyCost_, {0.5f, 0.0f, 100.0f});
    pHub->Register("Player", "ひるみ時間", &hitStunDuration_, {0.01f, 0.0f, 1.0f});
    pHub->Register("Player", "射撃被弾のひるみ倍率", &shotFlinchScale_, {0.01f, 0.0f, 1.0f});
    pHub->Register("Player", "吹き飛ばし復帰後のひるみ無効時間", &flinchImmuneDuration_, {0.05f, 0.0f, 5.0f});
    pHub->Register("Player", "吹き飛ばし着地後硬直", &blowAfterDuration_, {0.01f, 0.0f, 2.0f});
    pHub->Register("Player", "吹き飛ばし最大時間", &blowMaxDuration_, {0.05f, 0.1f, 5.0f});
    pHub->Register("Player", "吹き飛ばし中の落下加速度", &blowGravity_, {0.5f, 1.0f, 100.0f});
    pHub->Register("Player", "必殺技被弾の吹き飛び速度", &skillBlowSpeed_, {0.5f, 0.0f, 100.0f});
    pHub->Register("Player", "必殺技被弾の浮き上がり速度", &skillBlowRiseSpeed_, {0.5f, 0.0f, 50.0f});
    pHub->Register("Player", "必殺技被弾の横速度残存率(1秒)", &skillBlowHorizontalRetain_, {0.01f, 0.001f, 1.0f});
    pHub->Register("Player", "必殺技被弾スタン最大時間", &skillBlowMaxDuration_, {0.1f, 0.5f, 10.0f});
    pHub->Register("Player", "必殺技被弾中の被ダメージ倍率", &skillBlowDamageMultiplier_, {0.01f, 0.0f, 1.0f});
}

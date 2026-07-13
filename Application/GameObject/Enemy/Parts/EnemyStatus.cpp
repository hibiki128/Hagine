#define NOMINMAX
#include "EnemyStatus.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "EnemyMovement.h"
#include <Frame.h>
#include <Utility/Debug/GameParam/GameParamHub.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;

void EnemyStatus::Init(Enemy *owner)
{
    owner_ = owner;
}

void EnemyStatus::DamageUpdate()
{
    EnemyMovement &mv = owner_->Movement();
    Vector3 &velocity = mv.GetVelocity();

    if (damage_ <= kNoDamage)
    {
        // ダメージがなくてもノックバックだけ適用する場合に備えてチェック
        if (hasKnockback_)
        {
            velocity.x += pendingKnockback_.x;
            velocity.y += pendingKnockback_.y;
            velocity.z += pendingKnockback_.z;
            // velocityEaseをキャンセルしてノックバックが上書きされないようにする
            mv.CancelVelocityEase();
            // ノックバックでXZ方向に飛ぶ場合、地上判定を解除して浮かせる
            if (mv.GetIsGrounded() && (pendingKnockback_.y > 0.0f))
            {
                mv.GetIsGrounded() = false;
                mv.GetAcceleration().y = -mv.GetFallSpeed();
            }
            hasKnockback_ = false;
            pendingKnockback_ = {0.0f, 0.0f, 0.0f};
        }
        return;
    }

    // ダメージ計算（ガード時は軽減し、エネルギーを消費する：プレイヤーと同様）
    float actualDamage = damage_;
    if (isGuarding_)
    {
        actualDamage *= kGuardDamageMultiplier;
        ConsumeEnergy(kGuardEnergyCost);
    }
    HP_ -= actualDamage;
    damage_ = kNoDamage;

    // ノックバック適用（ガード中は軽減）
    if (hasKnockback_)
    {
        float knockbackMult = isGuarding_ ? kGuardDamageMultiplier : 1.0f;
        velocity.x += pendingKnockback_.x * knockbackMult;
        velocity.y += pendingKnockback_.y * knockbackMult;
        velocity.z += pendingKnockback_.z * knockbackMult;

        // velocityEaseをキャンセルしてBTの動きがノックバックを上書きしないようにする
        mv.CancelVelocityEase();

        // ノックバックに上方成分があれば空中に飛ばす
        if (mv.GetIsGrounded() && pendingKnockback_.y > 0.0f)
        {
            mv.GetIsGrounded() = false;
            mv.GetAcceleration().y = -mv.GetFallSpeed();
        }
        hasKnockback_ = false;
        pendingKnockback_ = {0.0f, 0.0f, 0.0f};
    }

    // ダメージリアクション開始
    StartDamageReact();
}

void EnemyStatus::UpdateDamageReact()
{
    // ダメージリアクション処理（高速点滅のみ。傾き(のけぞり)演出は廃止）
    if (!isDamageReact_)
    {
        return;
    }

    damageReactTimer_ += Frame::DeltaTime();

    float blinkInterval = kDamageBlinkInterval;
    int blink = static_cast<int>(damageReactTimer_ / blinkInterval);
    owner_->SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kAlphaTransparent : kAlphaOpaque);

    if (damageReactTimer_ >= damageReactDuration_)
    {
        isDamageReact_ = false;
        owner_->SetAlpha(kAlphaOpaque);
    }
}

bool EnemyStatus::ConsumeEnergy(float amount)
{
    if (energy_ >= amount)
    {
        energy_ -= amount;
        timeSinceLastShot_ = kTimerReset;
        return true;
    }
    return false;
}

void EnemyStatus::RecoverEnergy()
{
    timeSinceLastShot_ += Frame::DeltaTime();
    if (timeSinceLastShot_ >= energyRecoveryDelay_)
    {
        energy_ += energyRecoveryRate_ * Frame::DeltaTime();
        if (energy_ > maxEnergy_)
            energy_ = maxEnergy_;
    }
}

void EnemyStatus::SetKnockback(const Vector3 &direction, float power)
{
    if (power <= 0.0f)
    {
        return;
    }

    Vector3 dir = direction;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f)
    {
        return;
    }
    dir.x /= len;
    dir.y /= len;
    dir.z /= len;

    // ノックバック: 水平方向に強く + 少し上方に浮かせる
    pendingKnockback_ = {
        dir.x * power,
        power * 0.3f, // 上方への浮き（固定割合）
        dir.z * power,
    };
    hasKnockback_ = true;
}

void EnemyStatus::StartDamageReact()
{
    isDamageReact_ = true;
    damageReactTimer_ = kTimerReset;
}

void EnemyStatus::SetEnergy(float energy)
{
    energy_ = std::clamp(energy, 0.0f, maxEnergy_);
}

void EnemyStatus::ResetForRevive()
{
    HP_ = maxHP_;
    energy_ = maxEnergy_;
    hasKnockback_ = false;
    pendingKnockback_ = {0.0f, 0.0f, 0.0f};
    isDamageReact_ = false;
}

void EnemyStatus::RegisterParams()
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Enemy", "HP", static_cast<const float *>(&HP_));
    hub->Register("Enemy", "エネルギー", static_cast<const float *>(&energy_));
    hub->Register("Enemy", "エネルギー回復速度", &energyRecoveryRate_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "被弾リアクション時間", &damageReactDuration_, {0.05f, 0.0f, 3.0f});
}

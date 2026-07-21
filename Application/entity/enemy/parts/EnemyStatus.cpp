#define NOMINMAX
#include "EnemyStatus.h"
#include "Application/entity/enemy/Enemy.h"
#include "EnemyMovement.h"
#include <Frame.h>
#include <utility/debug/param/GameParamHub.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace Hagine;

void EnemyStatus::Init(Enemy *owner)
{
    pOwner_ = owner;
}

void EnemyStatus::DamageUpdate()
{
    EnemyMovement &mv = pOwner_->Movement();
    Vector3 &velocity = mv.GetVelocity();

    if (damage_ <= kNoDamage)
    {
        blowPending_ = false; // ダメージが無いフレームに予約が残らないようにする
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

    // ダメージリアクション開始（点滅）
    StartDamageReact();

    // 被弾リアクション（ひるみ・吹き飛ばし）。ガード成立時は行動不能にしない
    if (!isGuarding_)
    {
        if (blowPending_)
        {
            // 吹き飛ばし：着地するまでBlowBack、着地でBlowAfterへ
            reactState_ = EnemyReactState::Blow;
            blowLanded_ = false;
            blowAfterTimer_ = 0.0f;
            blowTimer_ = 0.0f; // 空中滞留時の強制復帰タイマーを被弾ごとにリセット
        }
        else
        {
            // ひるみ：時間経過で回復。被弾ごとにアニメをランダム選択
            reactState_ = EnemyReactState::Flinch;
            reactTimer_ = flinchDuration_;
            flinchAnimIndex_ = 1 + std::rand() % 3;
        }
    }
    blowPending_ = false;
}

void EnemyStatus::UpdateReaction()
{
    const float dt = Frame::DeltaTime();

    if (reactState_ == EnemyReactState::Flinch)
    {
        reactTimer_ -= dt;
        if (reactTimer_ <= 0.0f)
        {
            reactState_ = EnemyReactState::None;
        }
    }
    else if (reactState_ == EnemyReactState::Blow)
    {
        if (!blowLanded_)
        {
            // 空中に取り残されたまま吹き飛ばされっぱなしにならないよう、着地しなくても
            // 一定時間で強制復帰させる安全策。コンボが続く間は被弾ごとに blowTimer_ が
            // リセットされるため、殴られ続けている間はこのタイマーで復帰することはない
            blowTimer_ += dt;
            if (blowTimer_ >= blowMaxDuration_)
            {
                // 残った吹き飛ばし速度で飛び続けたり、上方向へ漂い続けたりしないよう
                // 速度を消してから通常状態へ戻す
                reactState_ = EnemyReactState::None;
                pOwner_->SetVelocity({0.0f, 0.0f, 0.0f});
                return;
            }

            // 吹き飛ばされ中。地面に着いたら着地硬直（BlowAfter）へ
            if (pOwner_->GetIsGrounded())
            {
                blowLanded_ = true;
                blowAfterTimer_ = blowAfterDuration_;
            }
        }
        else
        {
            blowAfterTimer_ -= dt;
            if (blowAfterTimer_ <= 0.0f)
            {
                // 復帰時に残速度を消して、横滑りや上方向への漂いを防ぐ
                reactState_ = EnemyReactState::None;
                pOwner_->SetVelocity({0.0f, 0.0f, 0.0f});
            }
        }
    }
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
    pOwner_->SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kAlphaTransparent : kAlphaOpaque);

    if (damageReactTimer_ >= damageReactDuration_)
    {
        isDamageReact_ = false;
        pOwner_->SetAlpha(kAlphaOpaque);
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

void EnemyStatus::SetKnockbackDirect(const Vector3 &velocity)
{
    // 上方成分の固定加算を行わず、指定した速度をそのまま適用する（叩きつけ等に使用）
    pendingKnockback_ = velocity;
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
    reactState_ = EnemyReactState::None;
    blowPending_ = false;
    blowLanded_ = false;
}

void EnemyStatus::RegisterParams()
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Enemy", "HP", static_cast<const float *>(&HP_));
    hub->Register("Enemy", "エネルギー", static_cast<const float *>(&energy_));
    hub->Register("Enemy", "エネルギー回復速度", &energyRecoveryRate_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "被弾リアクション時間", &damageReactDuration_, {0.05f, 0.0f, 3.0f});
    hub->Register("Enemy", "ひるみ時間", &flinchDuration_, {0.01f, 0.0f, 2.0f});
    hub->Register("Enemy", "吹き飛ばし着地後硬直", &blowAfterDuration_, {0.01f, 0.0f, 2.0f});
    hub->Register("Enemy", "吹き飛ばし最大時間", &blowMaxDuration_, {0.05f, 0.1f, 5.0f});
}

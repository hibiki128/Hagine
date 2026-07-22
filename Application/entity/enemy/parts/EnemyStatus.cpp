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
        blowPending_ = false;              // ダメージが無いフレームに予約が残らないようにする
        blowGrantsFlinchImmunity_ = false; // 予約と同時に付与指定も落とす
        damageIsShot_ = false;
        damageIsSkill_ = false;
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

    // ダメージ計算（ガード時は軽減し、エネルギーを消費する。必殺技は消費量が大きい）
    float actualDamage = damage_;
    if (isGuarding_)
    {
        actualDamage *= kGuardDamageMultiplier;
        // 残量が足りなくてもゼロまで削る（必殺技のガードを無償にしない）
        DrainEnergy(damageIsSkill_ ? kGuardSkillEnergyCost : kGuardEnergyCost);
    }

    // 必殺技被弾スタン中は行動できず無防備なので、追撃のダメージを軽減する
    const bool inSkillBlow = (reactState_ == EnemyReactState::SkillBlow) && !skillBlowPending_;
    if (inSkillBlow)
    {
        actualDamage *= skillBlowDamageMultiplier_;
    }

    HP_ -= actualDamage;
    damage_ = kNoDamage;

    // スタン中の追撃は落下の軌道を乱さないよう、ノックバックもリアクション変更も行わない
    if (inSkillBlow)
    {
        hasKnockback_ = false;
        pendingKnockback_ = {0.0f, 0.0f, 0.0f};
        blowPending_ = false;
        blowGrantsFlinchImmunity_ = false;
        damageIsShot_ = false;
        damageIsSkill_ = false;
        StartDamageReact();
        return;
    }

    // 必殺技被弾：吹き飛ばして落下スタンへ移行する（他のリアクションより優先）
    if (skillBlowPending_)
    {
        StartSkillBlow();
        StartDamageReact();
        damageIsShot_ = false;
        damageIsSkill_ = false;
        return;
    }

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
            blowTimer_ = 0.0f;         // 空中滞留時の強制復帰タイマーを被弾ごとにリセット
            flinchImmuneTimer_ = 0.0f; // 吹き飛ばし中は無効時間を進めない（復帰時に張り直す）
            blowImmunityArmed_ = blowGrantsFlinchImmunity_;
        }
        else if (flinchImmuneTimer_ <= 0.0f)
        {
            // ひるみ：時間経過で回復。被弾ごとにアニメをランダム選択（射撃は近接より短い）
            reactState_ = EnemyReactState::Flinch;
            reactTimer_ = damageIsShot_ ? flinchDuration_ * shotFlinchScale_ : flinchDuration_;
            flinchAnimIndex_ = 1 + std::rand() % 3;
        }
    }
    blowPending_ = false;
    blowGrantsFlinchImmunity_ = false;
    damageIsShot_ = false;
    damageIsSkill_ = false;
}

bool EnemyStatus::ConsumeGuardDeflect()
{
    if (!isGuarding_)
    {
        return false;
    }
    // 弾き返しも通常のガードと同じだけエネルギーを消費する
    ConsumeEnergy(kGuardEnergyCost);
    return true;
}

void EnemyStatus::RequestSkillBlowReaction(const Vector3 &direction)
{
    skillBlowPending_ = true;

    Vector3 dir = {direction.x, 0.0f, direction.z};
    const float len = std::sqrt(dir.x * dir.x + dir.z * dir.z);
    if (len < 0.001f)
    {
        // 方向が取れないときは敵の背面方向へ飛ばす
        dir = pOwner_->GetForward();
        dir.y = 0.0f;
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

void EnemyStatus::StartSkillBlow()
{
    EnemyMovement &mv = pOwner_->Movement();

    reactState_ = EnemyReactState::SkillBlow;
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
    pendingKnockback_ = {0.0f, 0.0f, 0.0f};

    Vector3 &velocity = mv.GetVelocity();
    velocity.x = skillBlowDirection_.x * skillBlowSpeed_;
    velocity.z = skillBlowDirection_.z * skillBlowSpeed_;
    velocity.y = skillBlowRiseSpeed_; // 一度浮かせてから落下させる

    // BTの移動イージング・飛行状態を打ち切り、重力で落ちる状態にする
    mv.CancelVelocityEase();
    mv.SetIsFlying(false);
    mv.GetIsGrounded() = false;

    // 重力加速度が未設定（0）でも必ず落下するようフォールバックを用意する
    float gravity = std::abs(mv.GetFallSpeed());
    if (gravity < 0.001f)
    {
        gravity = kSkillBlowFallbackGravity;
    }
    mv.GetAcceleration().y = -gravity;
}

void EnemyStatus::RecoverFromBlow(bool grantFlinchImmunity)
{
    // 残速度で飛び続けたり上方向へ漂ったりしないよう、速度を消してから戻す
    reactState_ = EnemyReactState::None;
    pOwner_->SetVelocity({0.0f, 0.0f, 0.0f});

    if (grantFlinchImmunity)
    {
        flinchImmuneTimer_ = flinchImmuneDuration_;
    }
    blowImmunityArmed_ = false;
}

void EnemyStatus::UpdateReaction()
{
    const float dt = Frame::DeltaTime();

    // ひるみ無効時間の消化
    if (flinchImmuneTimer_ > 0.0f)
    {
        flinchImmuneTimer_ -= dt;
        if (flinchImmuneTimer_ < 0.0f)
        {
            flinchImmuneTimer_ = 0.0f;
        }
    }

    if (reactState_ == EnemyReactState::SkillBlow)
    {
        UpdateSkillBlow(dt);
        return;
    }

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
        }
        else
        {
            blowAfterTimer_ -= dt;
            if (blowAfterTimer_ <= 0.0f)
            {
                RecoverFromBlow(blowImmunityArmed_);
            }
        }
    }
}

void EnemyStatus::UpdateSkillBlow(float deltaTime)
{
    Vector3 &velocity = pOwner_->Movement().GetVelocity();

    if (!blowLanded_)
    {
        skillBlowTimer_ += deltaTime;

        // 横移動の勢いを保ったまま徐々に減速させる（フレームレート非依存の指数減衰）
        const float damping = std::pow(skillBlowHorizontalRetain_, deltaTime);
        velocity.x *= damping;
        velocity.z *= damping;

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

void EnemyStatus::DrainEnergy(float amount)
{
    energy_ = (energy_ > amount) ? energy_ - amount : 0.0f;
    timeSinceLastShot_ = kTimerReset;
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
    blowGrantsFlinchImmunity_ = false;
    blowImmunityArmed_ = false;
    blowLanded_ = false;
    skillBlowPending_ = false;
    skillBlowTimer_ = 0.0f;
    flinchImmuneTimer_ = 0.0f;
}

void EnemyStatus::RegisterParams()
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Enemy", "HP", static_cast<const float *>(&HP_));
    hub->Register("Enemy", "エネルギー", static_cast<const float *>(&energy_));
    hub->Register("Enemy", "エネルギー回復速度", &energyRecoveryRate_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "被弾リアクション時間", &damageReactDuration_, {0.05f, 0.0f, 3.0f});
    hub->Register("Enemy", "ひるみ時間", &flinchDuration_, {0.01f, 0.0f, 2.0f});
    hub->Register("Enemy", "射撃被弾のひるみ倍率", &shotFlinchScale_, {0.01f, 0.0f, 1.0f});
    hub->Register("Enemy", "吹き飛ばし復帰後のひるみ無効時間", &flinchImmuneDuration_, {0.05f, 0.0f, 5.0f});
    hub->Register("Enemy", "吹き飛ばし着地後硬直", &blowAfterDuration_, {0.01f, 0.0f, 2.0f});
    hub->Register("Enemy", "吹き飛ばし最大時間", &blowMaxDuration_, {0.05f, 0.1f, 5.0f});
    hub->Register("Enemy", "必殺技被弾の吹き飛び速度", &skillBlowSpeed_, {0.5f, 0.0f, 100.0f});
    hub->Register("Enemy", "必殺技被弾の浮き上がり速度", &skillBlowRiseSpeed_, {0.5f, 0.0f, 50.0f});
    hub->Register("Enemy", "必殺技被弾の横速度残存率(1秒)", &skillBlowHorizontalRetain_, {0.01f, 0.001f, 1.0f});
    hub->Register("Enemy", "必殺技被弾スタン最大時間", &skillBlowMaxDuration_, {0.1f, 0.5f, 10.0f});
    hub->Register("Enemy", "必殺技被弾中の被ダメージ倍率", &skillBlowDamageMultiplier_, {0.01f, 0.0f, 1.0f});
}

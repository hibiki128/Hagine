#include "EnemyAttackCollider.h"
#include "Application/entity/enemy/Enemy.h"
#include "Application/entity/player/Player.h"
#include "collider/CollisionManager.h"
#include "particle/ParticleEditor.h"
#include "scene/SceneManager.h"

using namespace Hagine;

namespace {
// 叩きつけ（Slam）時、下方向の強さに対して前方へ流す割合
constexpr float kSlamHorizontalRatio = 0.3f;
} // namespace

void EnemyAttackCollider::Init(Enemy *pEnemy, Player *player)
{
    pEnemy_ = pEnemy;
    pPlayer_ = player;

    // OBBコライダーを生成・設定
    // 敵の向きに追従して回転できるようOBBを使用
    pCollider_ = pEnemy_->AddOBBCollider("EnemyAttackFront");
    pCollider_->SetTag("EnemyHand");
    pCollider_->AddCollisionMask("Player");
    pCollider_->SetEnabled(false); // 初期状態は無効

    // 衝突コールバックの設定
    pCollider_->SetOnCollision([this](ColliderBase *pOther) {
        this->OnCollision(pOther);
    });

    // ※ AddOBBCollider() の時点で CollisionManager へ登録済みのため、ここで再登録しない
    //   （二重登録の原因になる。解除はコライダーのデストラクタで自動的に行われる）

    // ヒットエフェクト・カメラシェイク初期化
    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("punchEmitter");
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(
        SceneManager::GetInstance()->GetBaseScene()->GetViewProjection(), "punchHit");
}

void EnemyAttackCollider::Update(float deltaTime)
{
    if (!shake_)
    {
        return;
    }
    shake_->Update();

    // 遅延処理：activateDelay_ 待機後に有効化
    if (isPending_)
    {
        delayTimer_ -= deltaTime;
        if (delayTimer_ <= 0.0f)
        {
            isPending_ = false;
            isActive_ = true;
            hasHitThisActivation_ = false;
            activeTimer_ = 0.0f;
            pCollider_->SetEnabled(true);
        }
        return;
    }

    // 有効時間の管理：activeDuration_ 経過後に自動無効化
    if (isActive_)
    {
        activeTimer_ += deltaTime;
        if (activeTimer_ >= activeDuration_)
        {
            Deactivate();
        }
    }
}

void EnemyAttackCollider::DrawParticle(const ViewProjection &viewProjection)
{
    if (hitEmitter_)
    {
        hitEmitter_->Draw(viewProjection);
    }
}

void EnemyAttackCollider::Activate(float damage, float knockbackPower,
                                   float activeDuration, float activateDelay,
                                   EnemyMeleeHitReaction reaction)
{
    // 既存の判定をリセット
    Deactivate();

    currentDamage_ = damage;
    currentKnockback_ = knockbackPower;
    currentReaction_ = reaction;
    activeDuration_ = activeDuration;
    hasHitThisActivation_ = false;

    if (activateDelay > 0.0f)
    {
        // 遅延がある場合は待機状態へ
        isPending_ = true;
        delayTimer_ = activateDelay;
        pCollider_->SetEnabled(false);
    }
    else
    {
        // 遅延がない場合は即座に有効化
        isPending_ = false;
        isActive_ = true;
        activeTimer_ = 0.0f;
        pCollider_->SetEnabled(true);
    }
}

void EnemyAttackCollider::Deactivate()
{
    isActive_ = false;
    isPending_ = false;
    activeTimer_ = 0.0f;
    delayTimer_ = 0.0f;
    if (pCollider_)
    {
        pCollider_->SetEnabled(false);
    }
}

void EnemyAttackCollider::OnCollision(ColliderBase *pOther)
{
    // 非アクティブ時や、既にこの回でヒット済みの場合は無視
    if (!isActive_ || hasHitThisActivation_)
    {
        return;
    }
    if (!pEnemy_ || !pPlayer_)
    {
        return;
    }
    if (pOther->GetTag() != "Player")
    {
        return;
    }

    // 連続ヒット防止
    hasHitThisActivation_ = true;

    // 吹き飛ばし予約はダメージ適用より先に行う（DamageUpdateで参照されるため）
    Vector3 knockbackDir = pEnemy_->GetForward();
    switch (currentReaction_)
    {
    case EnemyMeleeHitReaction::Slam:
    {
        // 下方向へ叩きつける（前方に少しだけ流しつつ強く下へ）。
        // 起き上がり直後にそのまま殴られ続けないよう、復帰後はしばらくひるまない
        Vector3 slamVel = {
            knockbackDir.x * currentKnockback_ * kSlamHorizontalRatio,
            -currentKnockback_,
            knockbackDir.z * currentKnockback_ * kSlamHorizontalRatio,
        };
        pPlayer_->RequestBlowReaction(true);
        pPlayer_->SetKnockbackDirect(slamVel);
        break;
    }
    case EnemyMeleeHitReaction::Launch:
        // 前方へ大きく吹き飛ばす（着地するまで行動不能）
        pPlayer_->RequestBlowReaction();
        pPlayer_->SetKnockback(knockbackDir, currentKnockback_);
        break;
    case EnemyMeleeHitReaction::Normal:
    default:
        pPlayer_->SetKnockback(knockbackDir, currentKnockback_);
        break;
    }

    // ダメージ適用
    pPlayer_->SetDamage(currentDamage_);

    // ヒットエフェクトの発生
    if (hitEmitter_)
    {
        Vector3 forward = pEnemy_->GetForward();
        Vector3 enemyPos = pEnemy_->GetWorldPosition();
        Vector3 hitPos = {
            enemyPos.x + forward.x * forwardOffset_,
            enemyPos.y + heightOffset_,
            enemyPos.z + forward.z * forwardOffset_,
        };
        hitEmitter_->SetPosition(hitPos);
        hitEmitter_->UpdateOnce();
    }

    // カメラシェイクの開始
    if (shake_)
    {
        shake_->StartShake();
    }

    // エネルギー回復処理
    if (pEnemy_)
    {
        float currentEnergy = pEnemy_->GetEnergy();
        float maxEnergy = pEnemy_->GetMaxEnergy();
        float newEnergy = currentEnergy + energyRecoveryAmount_;
        if (newEnergy > maxEnergy)
        {
            newEnergy = maxEnergy;
        }
        pEnemy_->GetEnergy() = newEnergy;
    }
}
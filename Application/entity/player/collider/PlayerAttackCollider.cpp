#include "PlayerAttackCollider.h"
#include "Application/entity/enemy/Enemy.h"
#include "Application/entity/player/Player.h"
#include "collider/CollisionManager.h"
#include "particle/ParticleEditor.h"
#include "scene/SceneManager.h"

using namespace Hagine;
void PlayerAttackCollider::Init(Player *player, Enemy *enemy)
{
    pPlayer_ = player;
    pEnemy_ = enemy;

    // OBBコライダー（プレイヤーの向きに追従して前方へ判定を出す）
    pCollider_ = pPlayer_->AddOBBCollider("PlayerAttackFront");
    pCollider_->SetTag("PlayerHand"); // 既存の衝突タグを流用
    pCollider_->AddCollisionMask("Enemy");
    pCollider_->SetEnabled(false); // 初期は無効

    // 衝突開始コールバック
    pCollider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
    });

    // AddOBBCollider() で登録済みのため再登録しない（二重登録防止）

    // ヒットエフェクト・カメラシェイク初期化
    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("punchEmitter");
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(
        SceneManager::GetInstance()->GetBaseScene()->GetViewProjection(), "punchHit");
}

void PlayerAttackCollider::Update(float deltaTime)
{
    if (!shake_)
    {
        return;
    }
    shake_->Update();

    // 遅延処理：delay > 0 なら一定時間後に有効化
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

    // 有効時間の管理：activeDuration_ を超えたら自動無効化
    if (isActive_)
    {
        activeTimer_ += deltaTime;
        if (activeTimer_ >= activeDuration_)
        {
            Deactivate();
        }
    }
}

void PlayerAttackCollider::DrawParticle(const ViewProjection &viewProjection)
{
    if (hitEmitter_)
    {
        hitEmitter_->Draw(viewProjection);
    }
}

void PlayerAttackCollider::Activate(float damage, float knockbackPower,
                                    float activeDuration, float activateDelay)
{
    // 前の判定を確実にリセットしてから開始
    Deactivate();

    currentDamage_ = damage;
    currentKnockback_ = knockbackPower;
    activeDuration_ = activeDuration;
    hasHitThisActivation_ = false;

    if (activateDelay > 0.0f)
    {
        // 遅延あり：ペンディング状態で待機
        isPending_ = true;
        delayTimer_ = activateDelay;
        pCollider_->SetEnabled(false);
    }
    else
    {
        // 遅延なし：即座に有効化
        isPending_ = false;
        isActive_ = true;
        activeTimer_ = 0.0f;
        pCollider_->SetEnabled(true);
    }
}

void PlayerAttackCollider::Deactivate()
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

void PlayerAttackCollider::OnCollision(ColliderBase *other)
{
    // アクティブでない場合・すでにこのアクティブ中にヒット済みの場合は無視
    if (!isActive_ || hasHitThisActivation_)
    {
        return;
    }
    if (!pEnemy_ || !pPlayer_)
    {
        return;
    }
    if (other->GetTag() != "Enemy")
    {
        return;
    }

    // 1アクティブ中1ヒットのみ（連続ヒット防止）
    hasHitThisActivation_ = true;

    // ダメージ適用
    pEnemy_->SetDamage(currentDamage_);

    // ノックバック適用
    Vector3 knockbackDir = pPlayer_->GetForward();
    pEnemy_->SetKnockback(knockbackDir, currentKnockback_);

    // ヒットエフェクト
    if (hitEmitter_)
    {
        Vector3 forward = pPlayer_->GetForward();
        Vector3 playerPos = pPlayer_->GetWorldPosition();
        Vector3 hitPos = {
            playerPos.x + forward.x * forwardOffset_,
            playerPos.y + heightOffset_,
            playerPos.z + forward.z * forwardOffset_,
        };
        hitEmitter_->SetPosition(hitPos);
        hitEmitter_->UpdateOnce();
    }

    // 攻撃ヒット時のエネルギー回復
    if (pPlayer_)
    {
        float currentEnergy = pPlayer_->GetEnergy();
        float maxEnergy = pPlayer_->GetMaxEnergy();
        float newEnergy = currentEnergy + energyRecoveryAmount_;
        if (newEnergy > maxEnergy)
        {
            newEnergy = maxEnergy;
        }
        pPlayer_->GetEnergy() = newEnergy;
    }

    // カメラシェイク
    if (shake_)
    {
        shake_->StartShake();
    }
}
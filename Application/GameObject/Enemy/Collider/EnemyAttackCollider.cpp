#include "EnemyAttackCollider.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "Collider/CollisionManager.h"
#include "Particle/ParticleEditor.h"
#include "Scene/SceneManager.h"

void EnemyAttackCollider::Init(Enemy *enemy, Player *player) {
    enemy_ = enemy;
    player_ = player;

    // -----------------------------------------------
    // OBBコライダーを生成・設定
    // OBBを選んだ理由：敵の向きに追従して回転できるため
    // 前方に正確に判定を出せる（PlayerAttackColliderと対称）
    // -----------------------------------------------
    collider_ = enemy_->AddOBBCollider("EnemyAttackFront");
    collider_->SetTag("EnemyHand"); // 既存の衝突タグを流用
    collider_->AddCollisionMask("Player");
    collider_->SetSize({2.0f, 1.5f, 2.0f}); // 横2・縦1.5・奥行2：やや広めで当てやすく
    collider_->SetEnabled(false);           // 初期は無効

    // -----------------------------------------------
    // 前方オフセット設定
    // 敵の前方方向（Z-）へ判定を押し出す
    // -----------------------------------------------
    collider_->SetPositionOffSet({0.0f, 0.0f, forwardOffset_});

    // 衝突コールバック
    collider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
    });

    // CollisionManagerに登録（デストラクタで自動解除される）
    CollisionManager::GetInstance()->Register(collider_);

    // ヒットエフェクト・カメラシェイク初期化
    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("punchEmitter");
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(
        SceneManager::GetInstance()->GetBaseScene()->GetViewProjection(), "punchHit");
}

void EnemyAttackCollider::Update(float deltaTime) {
    if (!shake_) {
        return;
    }
    shake_->Update();

    // -----------------------------------------------
    // 遅延処理：delay > 0 の場合、一定時間後にコライダーを有効化
    // -----------------------------------------------
    if (isPending_) {
        delayTimer_ -= deltaTime;
        if (delayTimer_ <= 0.0f) {
            isPending_ = false;
            isActive_ = true;
            hasHitThisActivation_ = false;
            activeTimer_ = 0.0f;
            collider_->SetEnabled(true);
        }
        return;
    }

    // -----------------------------------------------
    // 有効時間の管理：activeDuration_を超えたら自動無効化
    // -----------------------------------------------
    if (isActive_) {
        activeTimer_ += deltaTime;
        if (activeTimer_ >= activeDuration_) {
            Deactivate();
        }
    }
}

void EnemyAttackCollider::DrawParticle(const ViewProjection &viewProjection) {
    if (hitEmitter_) {
        hitEmitter_->Draw(viewProjection);
    }
}

void EnemyAttackCollider::Activate(float damage, float knockbackPower,
                                   float activeDuration, float activateDelay) {
    // 前の判定を確実にリセットしてから開始
    Deactivate();

    currentDamage_ = damage;
    currentKnockback_ = knockbackPower;
    activeDuration_ = activeDuration;
    hasHitThisActivation_ = false;

    if (activateDelay > 0.0f) {
        // 遅延あり：ペンディング状態で待機
        isPending_ = true;
        delayTimer_ = activateDelay;
        collider_->SetEnabled(false);
    } else {
        // 遅延なし：即座に有効化
        isPending_ = false;
        isActive_ = true;
        activeTimer_ = 0.0f;
        collider_->SetEnabled(true);
    }
}

void EnemyAttackCollider::Deactivate() {
    isActive_ = false;
    isPending_ = false;
    activeTimer_ = 0.0f;
    delayTimer_ = 0.0f;
    if (collider_) {
        collider_->SetEnabled(false);
    }
}

void EnemyAttackCollider::OnCollision(ColliderBase *other) {
    // アクティブでない場合・すでにこのアクティブ中にヒット済みの場合は無視
    if (!isActive_ || hasHitThisActivation_) {
        return;
    }
    if (!enemy_ || !player_) {
        return;
    }
    if (other->GetTag() != "Player") {
        return;
    }

    // 1アクティブ中1ヒットのみ（連続ヒット防止）
    hasHitThisActivation_ = true;

    // -----------------------------------------------
    // ダメージ適用
    // -----------------------------------------------
    player_->SetDamage(currentDamage_);

    // -----------------------------------------------
    // ノックバック適用
    // ※ Player側に SetKnockback(Vector3 dir, float power) の実装が必要
    // -----------------------------------------------
    Vector3 knockbackDir = enemy_->GetForward();
    player_->SetKnockback(knockbackDir, currentKnockback_);

    // -----------------------------------------------
    // ヒットエフェクト
    // -----------------------------------------------
    if (hitEmitter_) {
        Vector3 forward = enemy_->GetForward();
        Vector3 enemyPos = enemy_->GetWorldPosition();
        Vector3 hitPos = {
            enemyPos.x + forward.x * forwardOffset_,
            enemyPos.y + heightOffset_,
            enemyPos.z + forward.z * forwardOffset_,
        };
        hitEmitter_->SetPosition(hitPos);
        hitEmitter_->UpdateOnce();
    }

    // -----------------------------------------------
    // カメラシェイク
    // -----------------------------------------------
    if (shake_) {
        shake_->StartShake();
    }

    // -----------------------------------------------
    // 敵エネルギー回復（EnemyHandのOnCollisionEnterと同様）
    // -----------------------------------------------
    float newEnergy = enemy_->GetEnergy() + enemy_->GetEnergyRecoveryRate();
    if (newEnergy > enemy_->GetMaxEnergy()) {
        newEnergy = enemy_->GetMaxEnergy();
    }
    enemy_->GetEnergy() = newEnergy;
}
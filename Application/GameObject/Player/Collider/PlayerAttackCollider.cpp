#include "PlayerAttackCollider.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "Collider/CollisionManager.h"
#include "Particle/ParticleEditor.h"
#include "Scene/SceneManager.h"

void PlayerAttackCollider::Init(Player *player, Enemy *enemy) {
    player_ = player;
    enemy_ = enemy;

    // -----------------------------------------------
    // OBBコライダーを生成・設定
    // OBBを選んだ理由：プレイヤーの向きに追従して回転できるため
    // 空中戦でも前方に正確に判定を出せる
    // -----------------------------------------------
    collider_ = player_->AddOBBCollider("PlayerAttackFront");
    collider_->SetTag("PlayerHand"); // 既存の衝突タグを流用
    collider_->AddCollisionMask("Enemy");
    collider_->SetSize({2.0f, 1.5f, 2.0f}); // 横2・縦1.5・奥行2：やや広めで当てやすく
    collider_->SetEnabled(false);           // 初期は無効

    collider_->SetPositionOffSet({0.0f, 0.0f, 2.25f});

    // 衝突開始コールバック
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

void PlayerAttackCollider::Update(float deltaTime) {
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

void PlayerAttackCollider::DrawParticle(const ViewProjection &viewProjection) {
    if (hitEmitter_) {
        hitEmitter_->Draw(viewProjection);
    }
}

void PlayerAttackCollider::Activate(float damage, float knockbackPower,
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

void PlayerAttackCollider::Deactivate() {
    isActive_ = false;
    isPending_ = false;
    activeTimer_ = 0.0f;
    delayTimer_ = 0.0f;
    if (collider_) {
        collider_->SetEnabled(false);
    }
}

void PlayerAttackCollider::OnCollision(ColliderBase *other) {
    // アクティブでない場合・すでにこのアクティブ中にヒット済みの場合は無視
    if (!isActive_ || hasHitThisActivation_) {
        return;
    }
    if (!enemy_ || !player_) {
        return;
    }
    if (other->GetTag() != "Enemy") {
        return;
    }

    // 1アクティブ中1ヒットのみ（連続ヒット防止）
    hasHitThisActivation_ = true;

    // -----------------------------------------------
    // ダメージ適用
    // -----------------------------------------------
    enemy_->SetDamage(currentDamage_);

    // -----------------------------------------------
    // ノックバック適用
    // ※ Enemy側に SetKnockback(Vector3 dir, float power) の実装が必要
    // -----------------------------------------------
    Vector3 knockbackDir = player_->GetForward();
    enemy_->SetKnockback(knockbackDir, currentKnockback_);

    // -----------------------------------------------
    // ヒットエフェクト
    // -----------------------------------------------
    if (hitEmitter_) {
        Vector3 forward = player_->GetForward();
        Vector3 playerPos = player_->GetWorldPosition();
        Vector3 hitPos = {
            playerPos.x + forward.x * forwardOffset_,
            playerPos.y + heightOffset_,
            playerPos.z + forward.z * forwardOffset_,
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
}
#include "EnemyHand.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "Particle/ParticleEditor.h"
#include <Scene/SceneManager.h>

void EnemyHand::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);

    collider_ = AddSphereCollider("enemy_Hand");
    collider_->SetTag("EnemyHand");
    collider_->AddCollisionMask("Player");
    collider_->SetEnabled(false); // ComboSystemが有効化タイミングを管理

    collider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("punchEmitter");
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(
        SceneManager::GetInstance()->GetBaseScene()->GetViewProjection(), "punchHit");
}

void EnemyHand::Update() {
    if (isAlive_) {
        BaseObject::Update();
        shake_->Update();
    }
}

void EnemyHand::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    if (isAlive_) {
        BaseObject::Draw(viewProjection, offSet);
    }
}

void EnemyHand::DrawParticle(const ViewProjection &viewProjection) {
    if (hitEmitter_) {
        hitEmitter_->Draw(viewProjection);
    }
}

void EnemyHand::OnCollisionEnter(ColliderBase *other) {
    if (other->GetTag() != "Player") {
        return;
    }
    if (!player_ || !enemy_) {
        return;
    }

    // -----------------------------------------------
    // ダメージ適用（ComboSystemのコールバックで設定された値を使用）
    // -----------------------------------------------
    player_->SetDamage(damageAmount_);

    // -----------------------------------------------
    // ノックバック適用（敵の前方方向にプレイヤーを弾く）
    // ※ Player側に SetKnockback(Vector3, float) の実装が必要
    // -----------------------------------------------
    if (knockbackPower_ > 0.0f) {
        Vector3 knockbackDir = enemy_->GetForward();
        player_->SetKnockback(knockbackDir, knockbackPower_);
    }

    // -----------------------------------------------
    // ヒットエフェクト
    // -----------------------------------------------
    if (hitEmitter_) {
        hitEmitter_->SetPosition(GetWorldPosition());
        hitEmitter_->SetStartRotate("punchHit", GetWorldRotation().ToEulerDegrees());
        hitEmitter_->SetEndRotate("punchHit", GetWorldRotation().ToEulerDegrees());
        hitEmitter_->UpdateOnce();
    }

    shake_->StartShake();

    // -----------------------------------------------
    // 敵エネルギー回復
    // -----------------------------------------------
    float newEnergy = enemy_->GetEnergy() + energyRecoveryAmount_;
    if (newEnergy > enemy_->GetMaxEnergy()) {
        newEnergy = enemy_->GetMaxEnergy();
    }
    enemy_->GetEnergy() = newEnergy;
}
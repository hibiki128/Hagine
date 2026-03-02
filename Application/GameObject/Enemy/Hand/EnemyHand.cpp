#include "Application/GameObject/Enemy/Enemy.h"
#include "Particle/ParticleEditor.h"
#include "EnemyHand.h"
#include <Scene/SceneManager.h>
void EnemyHand::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    collider_ = AddSphereCollider("enemy_Hand");
    collider_->SetTag("EnemyHand");
    collider_->AddCollisionMask("Player");

    collider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("punchEmitter");
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(SceneManager::GetInstance()->GetBaseScene()->GetViewProjection(), "punchHit");
}

void EnemyHand::Update() {
    if (!isAlive_) {
    } else {
        BaseObject::Update();
        shake_->Update();
    }
}

void EnemyHand::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    if (!isAlive_) {
    } else {

        BaseObject::Draw(viewProjection, offSet);
    }
}

void EnemyHand::DrawParticle(const ViewProjection &viewProjection) {
    hitEmitter_->Draw(viewProjection);
}

void EnemyHand::OnCollisionEnter(ColliderBase *other) {
    if (other->GetTag() == "Player") {

        player_->SetDamage(4);
        hitEmitter_->SetPosition(GetWorldPosition());
        hitEmitter_->SetStartRotate("punchHit", GetWorldRotation().ToEulerDegrees());
        hitEmitter_->SetEndRotate("punchHit", GetWorldRotation().ToEulerDegrees());
        hitEmitter_->UpdateOnce();

        shake_->StartShake();

        // エネルギー回復処理を追加
        if (enemy_) {
            float currentEnergy = enemy_->GetEnergy();
            float maxEnergy = enemy_->GetMaxEnergy();
            float newEnergy = currentEnergy + energyRecoveryAmount_;
            if (newEnergy > maxEnergy) {
                newEnergy = maxEnergy;
            }
            enemy_->GetEnergy() = newEnergy;
        }
    }
}

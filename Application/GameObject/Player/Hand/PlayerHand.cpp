#include "PlayerHand.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Particle/ParticleEditor.h"
#include <Scene/SceneManager.h>
using namespace Hagine;
using namespace Math;
using namespace Graphics;
using namespace Camera;
using namespace Collision;

void PlayerHand::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    collider_ = AddSphereCollider("player_Hand");
    collider_->SetTag("PlayerHand");
    collider_->AddCollisionMask("Enemy");

    collider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("punchEmitter");
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(SceneManager::GetInstance()->GetBaseScene()->GetViewProjection(), "punchHit");
}

void PlayerHand::Update() {
    if (!isAlive_) {
    } else {
        BaseObject::Update();
        shake_->Update();
    }
}

void PlayerHand::Draw(const Camera::ViewProjection &viewProjection, Vector3 offSet) {
    if (!isAlive_) {
    } else {

        BaseObject::Draw(viewProjection, offSet);
    }
}

void PlayerHand::DrawParticle(const Camera::ViewProjection &viewProjection) {
    hitEmitter_->Draw(viewProjection);
}

void PlayerHand::OnCollisionEnter(ColliderBase *other) {
    if (other->GetTag() == "Enemy") {

        enemy_->SetDamage(4);
        hitEmitter_->SetPosition(GetWorldPosition());
        hitEmitter_->SetStartRotate("punchHit", GetWorldRotation().ToEulerDegrees());
        hitEmitter_->SetEndRotate("punchHit", GetWorldRotation().ToEulerDegrees());
        hitEmitter_->UpdateOnce();

        shake_->StartShake();

        // エネルギー回復処理を追加
        if (player_) {
            float currentEnergy = player_->GetEnergy();
            float maxEnergy = player_->GetMaxEnergy();
            float newEnergy = currentEnergy + energyRecoveryAmount_;
            if (newEnergy > maxEnergy) {
                newEnergy = maxEnergy;
            }
            player_->GetEnergy() = newEnergy; // 直接参照で代入
        }
    }
}

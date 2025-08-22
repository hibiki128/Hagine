#include "PlayerHand.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include"Particle/ParticleEditor.h"
#include <Scene/SceneManager.h>
void PlayerHand::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    BaseObject::AddCollider();
    BaseObject::SetCollisionType(CollisionType::Sphere);

    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("punchEmitter");
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(SceneManager::GetInstance()->GetBaseScene()->GetViewProjection(), "punchHit");
}

void PlayerHand::Update() {
    BaseObject::Update();
    shake_->Update();
}

void PlayerHand::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection, offSet);
}

void PlayerHand::DrawParticle(const ViewProjection& viewProjection) {
    hitEmitter_->Draw(viewProjection);
}

void PlayerHand::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<Enemy *>(other)) {
        enemy_->SetDamage(4);
        hitEmitter_->SetPosition(GetWorldPosition());
        hitEmitter_->SetStartRotate("punchHit", GetWorldRotation().ToEulerDegrees());
        hitEmitter_->SetEndRotate("punchHit", GetWorldRotation().ToEulerDegrees());
        hitEmitter_->UpdateOnce();

        shake_->StartShake();
    }
}

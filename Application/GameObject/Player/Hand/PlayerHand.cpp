#include "PlayerHand.h"
#include "Application/GameObject/Enemy/Enemy.h"
void PlayerHand::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    BaseObject::AddCollider();
    BaseObject::SetCollisionType(CollisionType::Sphere);
}

void PlayerHand::Update() {
    BaseObject::Update();
}

void PlayerHand::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection, offSet);
}

void PlayerHand::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<Enemy *>(other)) {
        enemy_->SetDamage(4);
    }
}

#include "PlayerHand.h"

void PlayerHand::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    // コライダーは PlayerAttackCollider 側で管理するため、ここでは生成しない
}

void PlayerHand::Update() {
    if (isAlive_) {
        BaseObject::Update();
    }
}

void PlayerHand::Draw(const ViewProjection &viewProjection) {
    if (isAlive_) {
        BaseObject::Draw(viewProjection);
    }
}
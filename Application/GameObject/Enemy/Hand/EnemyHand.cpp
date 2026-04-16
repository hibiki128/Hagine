#include "EnemyHand.h"

void EnemyHand::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    // コライダーは PlayerAttackCollider 側で管理するため、ここでは生成しない
}

void EnemyHand::Update() {
    if (isAlive_) {
        BaseObject::Update();
    }
}

void EnemyHand::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    if (isAlive_) {
        BaseObject::Draw(viewProjection, offSet);
    }
}
#include "EnemyHand.h"

void EnemyHand::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    // コライダーは EnemyAttackCollider 側で管理するため、ここでは生成しない
}

void EnemyHand::Update() {
    // 生存中のみ更新
    if (isAlive_) {
        BaseObject::Update();
    }
}

void EnemyHand::Draw(const ViewProjection &viewProjection) {
    // 生存中のみ描画
    if (isAlive_) {
        BaseObject::Draw(viewProjection);
    }
}
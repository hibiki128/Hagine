#include "Ground.h"

void Ground::Init(const std::string className) {
    BaseObject::Init(className);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Plane);
    BaseObject::SetTexture("debug/ground.png");
    Collider::SetCollisionEnabled(false);
    transform_.translation_.y = -1.0f;
    transform_.scale_ = {1000.0f, 1000.0f, 1000.0f};
    transform_.rotation_.x = degreesToRadians(-90.0f);
}

void Ground::Update() {
    BaseObject::Update();
}

void Ground::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection);
}

#include "Ground.h"

void Ground::Init(const std::string className) {
    BaseObject::Init(className);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Plane);
    BaseObject::SetTexture("debug/ground.png");
    Collider::SetCollisionEnabled(false);

    // 地面の位置とサイズ
    transform_->translation_.y = -1.0f;
    transform_->scale_ = {1000.0f, 1000.0f, 1000.0f};

    // クォータニオンで x 軸回転 -90度（ラジアンに変換）
    transform_->quateRotation_ = Quaternion::FromEulerAngles(Vector3(degreesToRadians(90.0f), 0.0f, 0.0f));
}

void Ground::Update() {
    BaseObject::Update();
}

void Ground::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection);
}

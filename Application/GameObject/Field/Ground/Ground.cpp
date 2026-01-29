#include "Ground.h"

using namespace Hagine;
using namespace Graphics;
using namespace Math;
using namespace Camera;

void Ground::Init(const std::string className) {
    BaseObject::Init(className);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Plane);
    BaseObject::SetTexture("debug/ground.png");

    transform_->translation_.y = -1.0f;
    transform_->scale_ = {1000.0f, 1000.0f, 1000.0f};

    transform_->quateRotation_ = Math::Quaternion::FromEulerAngles(Vector3(degreesToRadians(90.0f), 0.0f, 0.0f));
}

void Ground::Update() {
    BaseObject::Update();
}

void Ground::Draw(const Camera::ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection);
}

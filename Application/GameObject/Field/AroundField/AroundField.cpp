#include "AroundField.h"

void AroundField::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cylinder);
    AroundField_ = AddCylinderCollider("Around_Field");
    AroundField_->SetRadius(500.0f);
    AroundField_->SetHeight(500.0f);
    AroundField_->SetPositionGetter([]() -> Vector3{ return Vector3(0.0f, 200.0f, 0.0f); });
    AroundField_->SetInward(true);
    AroundField_->SetTag("CylinderField");
}

void AroundField::Update() {
    //BaseObject::Update();
}

void AroundField::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection, offSet);
}
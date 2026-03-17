#include "AroundField.h"
#include "Particle/CSParticle/ParticleCSEditor.h"

void AroundField::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cylinder);
    AroundField_ = AddCylinderCollider("Around_Field");
    AroundField_->SetRadius(150.0f);
    AroundField_->SetHeight(150.0f);
    AroundField_->SetPositionGetter([]() -> Vector3 { return Vector3(0.0f, 70.0f, 0.0f); });
    AroundField_->SetInward(true);
    AroundField_->SetTag("CylinderField");

    fieldParticle_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("AroundField");
}

void AroundField::Update() {
    // BaseObject::Update();
    if (!fieldParticle_->GetAcitve()) {
        fieldParticle_->SetActive(true);
    }
    fieldParticle_->Update();
}

void AroundField::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
   // BaseObject::Draw(viewProjection, offSet);
}

void AroundField::DrawParticle(const ViewProjection &viewProjection) {
    fieldParticle_->Draw(viewProjection);
}

void AroundField::Debug() {
    fieldParticle_->DrawImGui();
}

void AroundField::Finalize() {
    ParticleCSEmitter::ClearNameCounter("AroundField");
}

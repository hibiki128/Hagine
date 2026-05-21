#include "AroundField.h"
#include "Particle/CSParticle/ParticleCSEditor.h"

void AroundField::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cylinder);

    // 円柱状のコライダーを設定（フィールドの境界として使用）
    AroundField_ = AddCylinderCollider("Around_Field");
    AroundField_->SetRadius(150.0f);
    AroundField_->SetHeight(150.0f);
    AroundField_->SetPositionGetter([]() -> Vector3 { return Vector3(0.0f, 70.0f, 0.0f); });
    AroundField_->SetInward(true); // 内側への押し戻しを設定
    AroundField_->SetTag("CylinderField");

    // コンピュートシェーダパーティクルの生成
    fieldParticle_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("AroundField");
}

void AroundField::Update() {
    // パーティクルの更新
    if (!fieldParticle_->GetAcitve()) {
        fieldParticle_->SetActive(true);
    }
    fieldParticle_->Update();
}

void AroundField::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    // モデルとしての描画は行わない（パーティクルで表現するため）
}

void AroundField::DrawParticle(const ViewProjection &viewProjection) {
    // フィールドパーティクルの描画
    fieldParticle_->Draw(viewProjection);
}

void AroundField::Debug() {
    // パーティクルのデバッグ用GUI
    fieldParticle_->DrawImGui();
}

void AroundField::Finalize() {
    // パーティクルのカウンターをクリア
    ParticleCSEmitter::ClearNameCounter("AroundField");
}

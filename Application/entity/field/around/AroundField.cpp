#include "AroundField.h"
#include "particle/gpu/ParticleCSEditor.h"

using namespace Hagine;

AroundField::~AroundField()
{
    // 自身がアクティブフィールドとして登録されている場合のみ解除する
    if (activeField_ == AroundField_)
    {
        activeField_ = nullptr;
    }
}

void AroundField::Init(const std::string objectName)
{
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cylinder);

    // 円柱状のコライダーを設定（フィールドの境界として使用）
    AroundField_ = AddCylinderCollider("Around_Field");
    AroundField_->SetRadius(150.0f);
    AroundField_->SetHeight(150.0f);
    AroundField_->SetPositionGetter([]() -> Vector3 { return Vector3(0.0f, 70.0f, 0.0f); });
    AroundField_->SetInward(true); // 内側への押し戻しを設定
    AroundField_->SetTag("CylinderField");
    activeField_ = AroundField_;

    // コンピュートシェーダパーティクルの生成
    fieldParticle_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("AroundField");
}

void AroundField::Update()
{
    // パーティクルの更新
    if (!fieldParticle_->GetActive())
    {
        fieldParticle_->SetActive(true);
    }
    fieldParticle_->Update();
}

void AroundField::Draw(const ViewProjection &viewProjection)
{
    // モデルとしての描画は行わない（パーティクルで表現するため）
}

void AroundField::DrawParticleCompute(const ViewProjection &viewProjection)
{
    fieldParticle_->DrawCompute(viewProjection);
}

void AroundField::DrawParticle(const ViewProjection &viewProjection)
{
    fieldParticle_->DrawGraphics(viewProjection);
}

void AroundField::Debug()
{
    // パーティクルのデバッグ用GUI
    fieldParticle_->DrawImGui();
}

void AroundField::Finalize()
{
    // パーティクルのカウンターをクリア
    ParticleCSEmitter::ClearNameCounter("AroundField");
}

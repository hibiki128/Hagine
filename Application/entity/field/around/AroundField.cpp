#include "AroundField.h"
#include "particle/gpu/ParticleCSSpawner.h"

using namespace Hagine;

AroundField::~AroundField()
{
    // 自身がアクティブフィールドとして登録されている場合のみ解除する
    if (activeField_ == aroundField_)
    {
        activeField_ = nullptr;
    }
}

void AroundField::Init(const std::string objectName)
{
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cylinder);

    // 円柱状のコライダーを設定（フィールドの境界として使用）
    aroundField_ = AddCylinderCollider("Around_Field");
    aroundField_->SetRadius(150.0f);
    aroundField_->SetHeight(150.0f);
    aroundField_->SetPositionGetter([]() -> Vector3 { return Vector3(0.0f, 70.0f, 0.0f); });
    aroundField_->SetInward(true); // 内側への押し戻しを設定
    aroundField_->SetTag("CylinderField");
    activeField_ = aroundField_;

    // コンピュートシェーダパーティクルの生成。
    // Spawn したエミッターの更新・描画はエンジンが自動で回すので、
    // このクラス側で Update / DrawCompute / DrawGraphics を呼ぶ必要はない。
    fieldParticle_ = ParticleCSSpawner::GetInstance()->Spawn("AroundField");
}

void AroundField::Update()
{
    // パーティクルはエンジンが自動で駆動するためここでの更新は不要
}

void AroundField::Draw(const ViewProjection &viewProjection)
{
    // モデルとしての描画は行わない（パーティクルで表現するため）
}

void AroundField::Debug()
{
    // パーティクルのデバッグ用GUI
    if (fieldParticle_)
    {
        fieldParticle_->DrawImGui();
    }
}

void AroundField::Finalize()
{
    // 実体は ParticleCSSpawner が所有し、シーン遷移時にまとめて破棄されるため何もしない。
}

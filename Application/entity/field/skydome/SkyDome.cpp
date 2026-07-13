#include "SkyDome.h"

using namespace Hagine;
void SkyDome::Init(const std::string className)
{
    BaseObject::Init(className);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    BaseObject::SetTexture("debug/Fine_Basin.jpg");
    transform_->scale_ = {1000.0f, 1000.0f, 1000.0f};
}

void SkyDome::Update()
{
    BaseObject::Update();
}

void SkyDome::Draw(const ViewProjection &viewProjection)
{
    BaseObject::Draw(viewProjection);
}

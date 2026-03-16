#pragma once
#include "Collider/type/CylinderCollider.h"
#include "Object/Base/BaseObject.h"
class AroundField : public BaseObject {
  public:
    void Init(const std::string objectName) override;

    void Update() override;

    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

  private:
    CylinderCollider *AroundField_ = nullptr;
};

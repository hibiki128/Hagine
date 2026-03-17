#pragma once
#include "Collider/type/CylinderCollider.h"
#include "Object/Base/BaseObject.h"
#include <Particle/CSParticle/ParticleCSEmitter.h>
class AroundField : public BaseObject {
  public:
    void Init(const std::string objectName) override;

    void Update() override;

    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    void DrawParticle(const ViewProjection &viewProjection);

    void Debug();

    void Finalize();

  private:
    CylinderCollider *AroundField_ = nullptr;
    std::unique_ptr<ParticleCSEmitter> fieldParticle_ = nullptr;
};

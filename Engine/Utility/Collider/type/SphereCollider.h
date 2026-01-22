#pragma once
#include "../ColliderBase.h"

namespace Hagine::Collision {
class SphereCollider : public ColliderBase {
  public:
    SphereCollider() = default;
    ~SphereCollider() override = default;

    void SetRadius(float radius) { radius_ = radius; }
    float GetRadius() const { return radius_; }

    void SetOffset(const Vector3 &offset) { offset_ = offset; }
    const Vector3 &GetOffset() const { return offset_; }

    Sphere GetSphere() const {
        Sphere sphere;
        sphere.center = GetCenterPosition() + offset_;
        sphere.radius = radius_;
        return sphere;
    }

    ColliderType GetType() const override { return ColliderType::Sphere; }
    void UpdateWorldTransform() override;
    void DebugDraw(const Camera::ViewProjection &viewProjection) override;

    void SaveToJson() override;
    void LoadFromJson() override;

  private:
    float radius_ = 1.0f;
    Vector3 offset_ = {0.0f, 0.0f, 0.0f};
    Sphere cachedSphere_;
};
} // namespace Hagine::Collision
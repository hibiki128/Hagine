#pragma once
#include "../ColliderBase.h"

class AABBCollider : public ColliderBase {
  public:
    AABBCollider() = default;
    ~AABBCollider() override = default;

    void SetSize(const Vector3 &size) { size_ = size; }
    const Vector3 &GetSize() const { return size_; }

    void SetOffset(const Vector3 &offset) { offset_ = offset; }
    const Vector3 &GetOffset() const { return offset_; }

    AABB GetAABB() const {
        Vector3 center = GetCenterPosition() + offset_;
        Vector3 halfSize = size_ * 0.5f;
        return AABB{center - halfSize, center + halfSize};
    }

    ColliderType GetType() const override { return ColliderType::AABB; }
    void UpdateWorldTransform() override;
    void DebugDraw(const ViewProjection &viewProjection) override;

    void SaveToJson() override;
    void LoadFromJson() override;

  private:
    Vector3 size_ = {1.0f, 1.0f, 1.0f};
    Vector3 offset_ = {0.0f, 0.0f, 0.0f};
    AABB cachedAABB_;
};
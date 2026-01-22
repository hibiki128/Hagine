#pragma once
#include "../ColliderBase.h"

namespace Hagine::Collision {
class OBBCollider : public ColliderBase {
  public:
    OBBCollider() = default;
    ~OBBCollider() override = default;

    void SetSize(const Vector3 &size) { size_ = size; }
    const Vector3 &GetSize() const { return size_; }

    void SetRotationOffset(const Vector3 &offset) { rotationOffset_ = offset; }
    const Vector3 &GetRotationOffset() const { return rotationOffset_; }

    void SetPositionOffSet(const Vector3 &offset) { positionOffset_ = offset; }
    const Vector3 &GetPositionOffset() const { return positionOffset_; }

    OBB GetOBB() const { return cachedOBB_; }

    ColliderType GetType() const override { return ColliderType::OBB; }
    void UpdateWorldTransform() override;
    void DebugDraw(const Camera::ViewProjection &viewProjection) override;

    void SaveToJson() override;
    void LoadFromJson() override;

    void SetAnchorPoint(const Vector3 &anchor) { anchorPoint_ = anchor; }
    const Vector3 &GetAnchorPoint() const { return anchorPoint_; }

  private:
    void MakeOBBOrientations(const Math::Quaternion &rotation);
    void UpdateOBBScaleCenter();
    void DrawRotationCenter(const Camera::ViewProjection &viewProjection);

    Vector3 size_ = {1.0f, 1.0f, 1.0f};
    Vector3 rotationOffset_ = {0.0f, 0.0f, 0.0f};
    Vector3 positionOffset_ = {0.0f, 0.0f, 0.0f};
    OBB cachedOBB_;
    Vector3 anchorPoint_ = {0.5f, 0.5f, 0.5f};
};
} // namespace Hagine::Collision
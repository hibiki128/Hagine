#pragma once
#include "../ColliderBase.h"

class OBBCollider : public ColliderBase {
  public:
    OBBCollider() = default;
    ~OBBCollider() override = default;

    void SetSize(const Vector3 &size) { size_ = size; }
    const Vector3 &GetSize() const { return size_; }

    void SetRotationOffset(const Vector3 &offset) { rotationOffset_ = offset; }
    const Vector3 &GetRotationOffset() const { return rotationOffset_; }

    void SetScaleOffset(const Vector3 &offset) { scaleOffset_ = offset; }
    const Vector3 &GetScaleOffset() const { return scaleOffset_; }

    OBB GetOBB() const { return cachedOBB_; }

    ColliderType GetType() const override { return ColliderType::OBB; }
    void UpdateWorldTransform() override;
    void DebugDraw(const ViewProjection &viewProjection) override;

    void SaveToJson() override;
    void LoadFromJson() override;

  private:
    void MakeOBBOrientations(const Quaternion &rotation);
    void UpdateOBBScaleCenter();
    void DrawRotationCenter(const ViewProjection &viewProjection);

    Vector3 size_ = {1.0f, 1.0f, 1.0f};
    Vector3 rotationOffset_ = {0.0f, 0.0f, 0.0f};
    Vector3 scaleOffset_ = {0.0f, 0.0f, 0.0f};
    OBB cachedOBB_;
};
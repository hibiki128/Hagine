#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Collider/ColliderTagManager.h"
#include "Data/DataHandler.h"
#include "myMath.h"
#include "type/Quaternion.h"
#include "type/Vector3.h"
#include <functional>
#include <memory>
#include <string>

enum class ColliderType {
    Sphere,
    AABB,
    OBB,
    Cylinder
};

class CollisionManager;

class ColliderBase {
  public:
    using CollisionCallback = std::function<void(ColliderBase *)>;

    ColliderBase() = default;
    virtual ~ColliderBase() = default;

    // 純粋仮想関数
    virtual void UpdateWorldTransform() = 0;
    virtual void DebugDraw(const ViewProjection &viewProjection) = 0;
    virtual ColliderType GetType() const = 0;

    // コールバック設定
    void SetOnCollisionEnter(CollisionCallback callback) { onCollisionEnter_ = callback; }
    void SetOnCollision(CollisionCallback callback) { onCollision_ = callback; }
    void SetOnCollisionExit(CollisionCallback callback) { onCollisionExit_ = callback; }

    // コールバック実行
    void TriggerCollisionEnter(ColliderBase *other) {
        if (onCollisionEnter_)
            onCollisionEnter_(other);
    }
    void TriggerCollision(ColliderBase *other) {
        if (onCollision_)
            onCollision_(other);
    }
    void TriggerCollisionExit(ColliderBase *other) {
        if (onCollisionExit_)
            onCollisionExit_(other);
    }

    void SetTag(const std::string &tag);
    const std::string &GetTag() const { return tag_; }

    // 衝突マスク（複数タグ対応）
    void AddCollisionMask(const std::string &tag) {
        if (ColliderTagManager::GetInstance()->HasTag(tag)) {
            collisionMask_.insert(tag);
        }
    }
    void RemoveCollisionMask(const std::string &tag) {
        collisionMask_.erase(tag);
    }
    void ClearCollisionMask() {
        collisionMask_.clear();
    }
    const std::unordered_set<std::string> &GetCollisionMask() const {
        return collisionMask_;
    }

    bool ShouldCollideWith(const ColliderBase *other) const {
        return collisionMask_.find(other->GetTag()) != collisionMask_.end();
    }

    // 有効/無効
    void SetEnabled(bool enabled) { isEnabled_ = enabled; }
    bool IsEnabled() const { return isEnabled_; }

    // 可視性
    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }

    // 名前
    const std::string &GetName() const { return name_; }
    void SetName(const std::string &name) { name_ = name; }

    // 色設定
    void SetColor(const Vector4 &color) { color_ = color; }
    const Vector4 &GetColor() const { return color_; }
    void SetHitColor() { color_ = {1.0f, 0.0f, 0.0f, 1.0f}; }
    void SetDefaultColor() { color_ = {1.0f, 1.0f, 1.0f, 1.0f}; }

    // 衝突状態
    void SetCollidingInCurrentFrame(bool colliding) { isCollidingInCurrentFrame_ = colliding; }
    bool IsCollidingInCurrentFrame() const { return isCollidingInCurrentFrame_; }
    void ResetCollisionFlag() { isCollidingInCurrentFrame_ = false; }

    // セーブ/ロード
    virtual void SaveToJson();
    virtual void LoadFromJson();

    std::function<Vector3()> getPositionFunc_;
    std::function<Quaternion()> getRotationFunc_;

    Vector3 GetCenterPosition() const {
        return getPositionFunc_ ? getPositionFunc_() : Vector3{0, 0, 0};
    }

    Quaternion GetCenterRotation() const {
        return getRotationFunc_ ? getRotationFunc_() : Quaternion::IdentityQuaternion();
    }

    // セッター
    void SetPositionGetter(std::function<Vector3()> func) { getPositionFunc_ = func; }
    void SetRotationGetter(std::function<Quaternion()> func) { getRotationFunc_ = func; }
    bool isRegistered_ = false; // 登録済みフラグ
#ifdef _DEBUG
    // ImGuiでタグ設定UI表示
    void ImGuiTagSettings();
#endif

  protected:
    std::string name_;
    std::string tag_ = "None";
    std::unordered_set<std::string> collisionMask_;
    bool isEnabled_ = true;
    bool isVisible_ = true;
    bool isCollidingInCurrentFrame_ = false;

    Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};

    CollisionCallback onCollisionEnter_;
    CollisionCallback onCollision_;
    CollisionCallback onCollisionExit_;

    std::unique_ptr<DataHandler> dataHandler_;
};
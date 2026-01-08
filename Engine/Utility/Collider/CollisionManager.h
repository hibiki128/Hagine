#pragma once
#define NOMINMAX
#include "ColliderBase.h"
#include "type/AABBCollider.h"
#include "type/OBBCollider.h"
#include "type/SphereCollider.h"
#include <unordered_map>
#include <vector>

class CollisionManager {
  public:
    static CollisionManager *GetInstance() {
        static CollisionManager instance;
        return &instance;
    }

    void Register(ColliderBase *collider);
    void Unregister(ColliderBase *collider);
    void Clear();

    void UpdateColliderTag(ColliderBase *collider, const std::string &oldTag, const std::string &newTag);

    void Update();
    void DebugDraw(const ViewProjection &viewProjection);

#ifdef _DEBUG
    // ImGuiでタグマネージャーUI表示
    void ImGuiTagManager() {
        ColliderTagManager::GetInstance()->ImGuiTagManager();
    }
#endif

  private:
    CollisionManager() = default;
    ~CollisionManager() = default;
    CollisionManager(const CollisionManager &) = delete;
    CollisionManager &operator=(const CollisionManager &) = delete;

    void UpdateColliders();
    void CheckCollisions();
    void CheckCollisionPair(ColliderBase *a, ColliderBase *b);

    bool TestCollision(ColliderBase *a, ColliderBase *b);

    // 各種衝突判定関数
    bool IsCollision(const Sphere &s1, const Sphere &s2);
    bool IsCollision(const AABB &aabb1, const AABB &aabb2);
    bool IsCollision(const OBB &obb1, const OBB &obb2);
    bool IsCollision(const AABB &aabb, const Sphere &sphere);
    bool IsCollision(const OBB &obb, const Sphere &sphere);
    bool IsCollision(const AABB &aabb, const OBB &obb);

    // ヘルパー関数
    void ProjectOBB(const OBB &obb, const Vector3 &axis, float &min, float &max);
    void ProjectAABB(const Vector3 &axis, const AABB &aabb, float &outMin, float &outMax);
    bool TestAxis(const Vector3 &axis, const OBB &obb1, const OBB &obb2);
    bool TestAxis(const Vector3 &axis, const AABB &aabb, const OBB &obb);

    // タグごとにコライダーをグループ化（文字列キー対応）
    std::unordered_map<std::string, std::vector<ColliderBase *>> collidersByTag_;

    // 衝突状態の管理
    struct CollisionPair {
        ColliderBase *a;
        ColliderBase *b;

        bool operator==(const CollisionPair &other) const {
            return (a == other.a && b == other.b) || (a == other.b && b == other.a);
        }
    };

    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair &pair) const {
            auto ptrA = reinterpret_cast<std::uintptr_t>(pair.a);
            auto ptrB = reinterpret_cast<std::uintptr_t>(pair.b);
            return std::hash<std::uintptr_t>{}((std::min)(ptrA, ptrB)) ^
                   (std::hash<std::uintptr_t>{}((std::max)(ptrA, ptrB)) << 1);
        }
    };

    std::unordered_map<CollisionPair, bool, CollisionPairHash> collisionStates_;
};
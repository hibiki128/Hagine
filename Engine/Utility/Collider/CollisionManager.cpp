#include"Collider/CollosionManager.h"
#include "myMath.h"
#include <algorithm>

void CollisionManager::Register(ColliderBase *collider) {
    if (!collider)
        return;

    const std::string &tag = collider->GetTag();
    collidersByTag_[tag].push_back(collider);
    collider->isRegistered_ = true; // 登録フラグを設定
}

void CollisionManager::Unregister(ColliderBase *collider) {
    if (!collider)
        return;

    const std::string &tag = collider->GetTag();
    auto it = collidersByTag_.find(tag);
    if (it != collidersByTag_.end()) {
        auto &colliders = it->second;
        colliders.erase(
            std::remove(colliders.begin(), colliders.end(), collider),
            colliders.end());
    }

    for (auto it = collisionStates_.begin(); it != collisionStates_.end();) {
        if (it->first.a == collider || it->first.b == collider) {
            it = collisionStates_.erase(it);
        } else {
            ++it;
        }
    }

    collider->isRegistered_ = false; // 登録フラグを解除
}

void CollisionManager::UpdateColliderTag(ColliderBase *collider, const std::string &oldTag, const std::string &newTag) {
    if (!collider)
        return;

    // 旧タグのリストから削除
    auto oldIt = collidersByTag_.find(oldTag);
    if (oldIt != collidersByTag_.end()) {
        auto &colliders = oldIt->second;
        colliders.erase(
            std::remove(colliders.begin(), colliders.end(), collider),
            colliders.end());

        // リストが空になったら、マップから削除
        if (colliders.empty()) {
            collidersByTag_.erase(oldIt);
        }
    }

    // 新タグのリストに追加
    collidersByTag_[newTag].push_back(collider);
}

void CollisionManager::Clear() {
    collidersByTag_.clear();
    collisionStates_.clear();
}

void CollisionManager::Update() {
    UpdateColliders();
    CheckCollisions();
}

void CollisionManager::UpdateColliders() {
    for (auto &[tag, colliders] : collidersByTag_) {
        for (auto *collider : colliders) {
            if (!collider->IsEnabled()) {
                continue;
            }

            collider->UpdateWorldTransform();

            if (collider->IsCollidingInCurrentFrame()) {
                collider->SetHitColor();
            } else {
                collider->SetDefaultColor();
            }

            collider->ResetCollisionFlag();
        }
    }
}

void CollisionManager::CheckCollisions() {
    for (auto &[tagA, collidersA] : collidersByTag_) {
        for (auto *colliderA : collidersA) {
            if (!colliderA->IsEnabled())
                continue;

            const std::unordered_set<std::string> &mask = colliderA->GetCollisionMask();

            for (auto &[tagB, collidersB] : collidersByTag_) {
                // マスクにtagBが含まれているかチェック
                if (mask.find(tagB) == mask.end())
                    continue;

                for (auto *colliderB : collidersB) {
                    if (!colliderB->IsEnabled())
                        continue;
                    if (colliderA == colliderB)
                        continue;

                    CheckCollisionPair(colliderA, colliderB);
                }
            }
        }
    }
}

void CollisionManager::CheckCollisionPair(ColliderBase *a, ColliderBase *b) {
    CollisionPair pair{a, b};
    if (a > b) {
        pair = {b, a};
    }

    // 双方向チェック: どちらかが判定を望んでいる場合のみ衝突判定を行う
    bool shouldCheck = a->ShouldCollideWith(b) || b->ShouldCollideWith(a);

    if (!shouldCheck) {
        // 判定が不要な場合は、以前の衝突状態をクリア
        auto it = collisionStates_.find(pair);
        if (it != collisionStates_.end() && it->second) {
            // 以前衝突していた場合はExitイベントを発火
            a->TriggerCollisionExit(b);
            b->TriggerCollisionExit(a);
        }
        collisionStates_[pair] = false;
        return;
    }

    bool isCollidingNow = TestCollision(a, b);
    bool wasColliding = collisionStates_[pair];

    if (isCollidingNow) {
        // 両方に衝突フラグを設定（視覚的フィードバック用）
        a->SetCollidingInCurrentFrame(true);
        b->SetCollidingInCurrentFrame(true);

        if (!wasColliding) {
            a->TriggerCollisionEnter(b);
            b->TriggerCollisionEnter(a);
        }

        a->TriggerCollision(b);
        b->TriggerCollision(a);
    } else {
        if (wasColliding) {
            a->TriggerCollisionExit(b);
            b->TriggerCollisionExit(a);
        }
    }

    collisionStates_[pair] = isCollidingNow;
}

bool CollisionManager::TestCollision(ColliderBase *a, ColliderBase *b) {
    ColliderType typeA = a->GetType();
    ColliderType typeB = b->GetType();

    // Sphere - Sphere
    if (typeA == ColliderType::Sphere && typeB == ColliderType::Sphere) {
        auto *sphereA = static_cast<SphereCollider *>(a);
        auto *sphereB = static_cast<SphereCollider *>(b);
        return IsCollision(sphereA->GetSphere(), sphereB->GetSphere());
    }

    // AABB - AABB
    if (typeA == ColliderType::AABB && typeB == ColliderType::AABB) {
        auto *aabbA = static_cast<AABBCollider *>(a);
        auto *aabbB = static_cast<AABBCollider *>(b);
        return IsCollision(aabbA->GetAABB(), aabbB->GetAABB());
    }

    // OBB - OBB
    if (typeA == ColliderType::OBB && typeB == ColliderType::OBB) {
        auto *obbA = static_cast<OBBCollider *>(a);
        auto *obbB = static_cast<OBBCollider *>(b);
        return IsCollision(obbA->GetOBB(), obbB->GetOBB());
    }

    // AABB - Sphere
    if (typeA == ColliderType::AABB && typeB == ColliderType::Sphere) {
        auto *aabb = static_cast<AABBCollider *>(a);
        auto *sphere = static_cast<SphereCollider *>(b);
        return IsCollision(aabb->GetAABB(), sphere->GetSphere());
    }
    if (typeA == ColliderType::Sphere && typeB == ColliderType::AABB) {
        auto *sphere = static_cast<SphereCollider *>(a);
        auto *aabb = static_cast<AABBCollider *>(b);
        return IsCollision(aabb->GetAABB(), sphere->GetSphere());
    }

    // OBB - Sphere
    if (typeA == ColliderType::OBB && typeB == ColliderType::Sphere) {
        auto *obb = static_cast<OBBCollider *>(a);
        auto *sphere = static_cast<SphereCollider *>(b);
        return IsCollision(obb->GetOBB(), sphere->GetSphere());
    }
    if (typeA == ColliderType::Sphere && typeB == ColliderType::OBB) {
        auto *sphere = static_cast<SphereCollider *>(a);
        auto *obb = static_cast<OBBCollider *>(b);
        return IsCollision(obb->GetOBB(), sphere->GetSphere());
    }

    // AABB - OBB
    if (typeA == ColliderType::AABB && typeB == ColliderType::OBB) {
        auto *aabb = static_cast<AABBCollider *>(a);
        auto *obb = static_cast<OBBCollider *>(b);
        return IsCollision(aabb->GetAABB(), obb->GetOBB());
    }
    if (typeA == ColliderType::OBB && typeB == ColliderType::AABB) {
        auto *obb = static_cast<OBBCollider *>(a);
        auto *aabb = static_cast<AABBCollider *>(b);
        return IsCollision(aabb->GetAABB(), obb->GetOBB());
    }

    return false;
}

void CollisionManager::DebugDraw(const ViewProjection &viewProjection) {
    for (auto &[tag, colliders] : collidersByTag_) {
        for (auto *collider : colliders) {
            collider->DebugDraw(viewProjection);
        }
    }
}

bool CollisionManager::IsCollision(const Sphere &s1, const Sphere &s2) {
    Vector3 diff = s2.center - s1.center;
    float distanceSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    float radiusSum = s1.radius + s2.radius;
    return distanceSquared <= (radiusSum * radiusSum);
}

bool CollisionManager::IsCollision(const AABB &aabb1, const AABB &aabb2) {
    return (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
           (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
           (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z);
}

bool CollisionManager::IsCollision(const OBB &obb1, const OBB &obb2) {
    Vector3 axes[15] = {
        obb1.orientations[0],
        obb1.orientations[1],
        obb1.orientations[2],
        obb2.orientations[0],
        obb2.orientations[1],
        obb2.orientations[2],
        obb1.orientations[0].Cross(obb2.orientations[0]),
        obb1.orientations[0].Cross(obb2.orientations[1]),
        obb1.orientations[0].Cross(obb2.orientations[2]),
        obb1.orientations[1].Cross(obb2.orientations[0]),
        obb1.orientations[1].Cross(obb2.orientations[1]),
        obb1.orientations[1].Cross(obb2.orientations[2]),
        obb1.orientations[2].Cross(obb2.orientations[0]),
        obb1.orientations[2].Cross(obb2.orientations[1]),
        obb1.orientations[2].Cross(obb2.orientations[2]),
    };

    for (const Vector3 &axis : axes) {
        if (axis.Length() > 0.0001f && !TestAxis(axis.Normalize(), obb1, obb2)) {
            return false;
        }
    }

    return true;
}

bool CollisionManager::IsCollision(const AABB &aabb, const Sphere &sphere) {
    Vector3 closestPoint{
        std::clamp(sphere.center.x, aabb.min.x, aabb.max.x),
        std::clamp(sphere.center.y, aabb.min.y, aabb.max.y),
        std::clamp(sphere.center.z, aabb.min.z, aabb.max.z)};

    Vector3 diff = closestPoint - sphere.center;
    float distanceSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

    return distanceSquared <= (sphere.radius * sphere.radius);
}

bool CollisionManager::IsCollision(const OBB &obb, const Sphere &sphere) {
    Matrix4x4 rotateMatrix{};

    rotateMatrix.m[0][0] = obb.orientations[0].x;
    rotateMatrix.m[1][0] = obb.orientations[0].y;
    rotateMatrix.m[2][0] = obb.orientations[0].z;
    rotateMatrix.m[3][0] = 0.0f;

    rotateMatrix.m[0][1] = obb.orientations[1].x;
    rotateMatrix.m[1][1] = obb.orientations[1].y;
    rotateMatrix.m[2][1] = obb.orientations[1].z;
    rotateMatrix.m[3][1] = 0.0f;

    rotateMatrix.m[0][2] = obb.orientations[2].x;
    rotateMatrix.m[1][2] = obb.orientations[2].y;
    rotateMatrix.m[2][2] = obb.orientations[2].z;
    rotateMatrix.m[3][2] = 0.0f;

    rotateMatrix.m[0][3] = 0.0f;
    rotateMatrix.m[1][3] = 0.0f;
    rotateMatrix.m[2][3] = 0.0f;
    rotateMatrix.m[3][3] = 1.0f;

    Matrix4x4 obbWorldMatrix = MakeOBBWorldMatrix(obb, rotateMatrix);

    Matrix4x4 obbWorldMatrixInverse = Inverse(obbWorldMatrix);

    Vector3 centerInOBBLocalSpace = Transformation(sphere.center, obbWorldMatrixInverse);

    AABB aabbOBBLocal = ConvertOBBToAABB(obb);

    Sphere sphereOBBLocal{centerInOBBLocalSpace, sphere.radius};

    return IsCollision(aabbOBBLocal, sphereOBBLocal);
}

bool CollisionManager::IsCollision(const AABB &aabb, const OBB &obb) {
    Vector3 aabbCenter = (aabb.min + aabb.max) * 0.5f;
    Vector3 aabbHalfSize = {
        (aabb.max.x - aabb.min.x) / 2.0f,
        (aabb.max.y - aabb.min.y) / 2.0f,
        (aabb.max.z - aabb.min.z) / 2.0f};

    Vector3 t = obb.scaleCenterRotated - aabbCenter;

    Vector3 axes[15] = {
        {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, obb.orientations[0], obb.orientations[1], obb.orientations[2], Vector3(1, 0, 0).Cross(obb.orientations[0]), Vector3(1, 0, 0).Cross(obb.orientations[1]), Vector3(1, 0, 0).Cross(obb.orientations[2]), Vector3(0, 1, 0).Cross(obb.orientations[0]), Vector3(0, 1, 0).Cross(obb.orientations[1]), Vector3(0, 1, 0).Cross(obb.orientations[2]), Vector3(0, 0, 1).Cross(obb.orientations[0]), Vector3(0, 0, 1).Cross(obb.orientations[1]), Vector3(0, 0, 1).Cross(obb.orientations[2])};

    for (int i = 0; i < 15; i++) {
        if (axes[i].Length() < 1e-6)
            continue;
        axes[i] = axes[i].Normalize();

        float projectionAABB =
            aabbHalfSize.x * std::abs(axes[i].Dot(Vector3(1, 0, 0))) +
            aabbHalfSize.y * std::abs(axes[i].Dot(Vector3(0, 1, 0))) +
            aabbHalfSize.z * std::abs(axes[i].Dot(Vector3(0, 0, 1)));

        float projectionOBB =
            std::abs(obb.orientations[0].Dot(axes[i])) * obb.size.x +
            std::abs(obb.orientations[1].Dot(axes[i])) * obb.size.y +
            std::abs(obb.orientations[2].Dot(axes[i])) * obb.size.z;

        float distance = std::abs(t.Dot(axes[i]));

        if (distance > projectionAABB + projectionOBB) {
            return false;
        }
    }

    return true;
}

void CollisionManager::ProjectOBB(const OBB &obb, const Vector3 &axis, float &min, float &max) {
    Vector3 rotatedCenter = obb.scaleCenterRotated;
    float centerProjection = rotatedCenter.Dot(axis);

    float radius =
        std::abs(obb.orientations[0].Dot(axis)) * obb.size.x +
        std::abs(obb.orientations[1].Dot(axis)) * obb.size.y +
        std::abs(obb.orientations[2].Dot(axis)) * obb.size.z;

    min = centerProjection - radius;
    max = centerProjection + radius;
}

void CollisionManager::ProjectAABB(const Vector3 &axis, const AABB &aabb, float &outMin, float &outMax) {
    Vector3 vertices[8];
    vertices[0] = aabb.min;
    vertices[1] = {aabb.max.x, aabb.min.y, aabb.min.z};
    vertices[2] = {aabb.min.x, aabb.max.y, aabb.min.z};
    vertices[3] = {aabb.max.x, aabb.max.y, aabb.min.z};
    vertices[4] = {aabb.min.x, aabb.min.y, aabb.max.z};
    vertices[5] = {aabb.max.x, aabb.min.y, aabb.max.z};
    vertices[6] = {aabb.min.x, aabb.max.y, aabb.max.z};
    vertices[7] = aabb.max;

    outMin = axis.Dot(vertices[0]);
    outMax = outMin;

    for (int i = 1; i < 8; ++i) {
        float projection = axis.Dot(vertices[i]);
        if (projection < outMin)
            outMin = projection;
        if (projection > outMax)
            outMax = projection;
    }
}

bool CollisionManager::TestAxis(const Vector3 &axis, const OBB &obb1, const OBB &obb2) {
    float min1, max1, min2, max2;
    ProjectOBB(obb1, axis, min1, max1);
    ProjectOBB(obb2, axis, min2, max2);

    float sumSpan = (max1 - min1) + (max2 - min2);
    float longSpan = std::max(max1, max2) - std::min(min1, min2);

    return sumSpan >= longSpan;
}

bool CollisionManager::TestAxis(const Vector3 &axis, const AABB &aabb, const OBB &obb) {
    float aabbMin, aabbMax;
    ProjectAABB(axis, aabb, aabbMin, aabbMax);

    float obbMin, obbMax;
    ProjectOBB(obb, axis, obbMin, obbMax);

    float sumSpan = (aabbMax - aabbMin) + (obbMax - obbMin);
    float longSpan = std::max(aabbMax, obbMax) - std::min(aabbMin, obbMin);

    return sumSpan >= longSpan;
}
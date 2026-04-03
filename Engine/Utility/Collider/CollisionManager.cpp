#include "Collider/CollisionManager.h"
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
    if (isFinalized_)
        return;
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
    isFinalized_ = true;
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

    // OBB - Cylinder
    if (typeA == ColliderType::OBB && typeB == ColliderType::Cylinder) {
        auto *obb = static_cast<OBBCollider *>(a);
        auto *cyl = static_cast<CylinderCollider *>(b);
        return IsCollisionOBBCylinder(obb, cyl);
    }
    if (typeA == ColliderType::Cylinder && typeB == ColliderType::OBB) {
        auto *cyl = static_cast<CylinderCollider *>(a);
        auto *obb = static_cast<OBBCollider *>(b);
        return IsCollisionOBBCylinder(obb, cyl);
    }

    return false;
}

void CollisionManager::DebugDraw(const ViewProjection &viewProjection) {
    if (isVisible_) {
        for (auto &[tag, colliders] : collidersByTag_) {
            for (auto *collider : colliders) {
                collider->DebugDraw(viewProjection);
            }
        }
    }
}

bool CollisionManager::CalculateDepenetration(OBBCollider *colliderA, OBBCollider *colliderB, Vector3 &outMTV) {
    const OBB &obbA = colliderA->GetOBB();
    const OBB &obbB = colliderB->GetOBB();

    // SAT の 15軸（面法線6本 + 辺の外積9本）
    Vector3 axes[15] = {
        obbA.orientations[0],
        obbA.orientations[1],
        obbA.orientations[2],
        obbB.orientations[0],
        obbB.orientations[1],
        obbB.orientations[2],
        obbA.orientations[0].Cross(obbB.orientations[0]),
        obbA.orientations[0].Cross(obbB.orientations[1]),
        obbA.orientations[0].Cross(obbB.orientations[2]),
        obbA.orientations[1].Cross(obbB.orientations[0]),
        obbA.orientations[1].Cross(obbB.orientations[1]),
        obbA.orientations[1].Cross(obbB.orientations[2]),
        obbA.orientations[2].Cross(obbB.orientations[0]),
        obbA.orientations[2].Cross(obbB.orientations[1]),
        obbA.orientations[2].Cross(obbB.orientations[2]),
    };

    // A→B の方向ベクトル（MTV軸の向きを揃えるために使用）
    Vector3 d = obbB.scaleCenterRotated - obbA.scaleCenterRotated;

    float minPenetration = FLT_MAX;
    Vector3 mtvAxis = {};

    for (const Vector3 &axis : axes) {
        if (axis.Length() < 0.0001f) {
            continue; // 零ベクトル（平行辺のクロス積）はスキップ
        }
        Vector3 normalizedAxis = axis.Normalize();

        float minA, maxA, minB, maxB;
        ProjectOBB(obbA, normalizedAxis, minA, maxA);
        ProjectOBB(obbB, normalizedAxis, minB, maxB);

        // 貫通深度 = オーバーラップ量
        float penetration = (std::min)(maxA, maxB) - (std::max)(minA, minB);
        if (penetration < 0.0f) {
            return false; // 分離軸が見つかった → 衝突していない
        }

        if (penetration < minPenetration) {
            minPenetration = penetration;
            // MTV軸を「AをBから離す方向」に揃える
            mtvAxis = (d.Dot(normalizedAxis) < 0.0f) ? -normalizedAxis : normalizedAxis;
        }
    }

    // MTV = 最小貫通軸 × 貫通深度
    outMTV = mtvAxis * minPenetration;
    return true;
}

bool CollisionManager::CalculateDepenetration(AABBCollider *colliderA, AABBCollider *colliderB, Vector3 &outMTV) {
    const AABB &a = colliderA->GetAABB();
    const AABB &b = colliderB->GetAABB();

    // 各軸のオーバーラップ量
    float overlapX = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    float overlapZ = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);
    float overlapY = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);

    // いずれかの軸でオーバーラップがなければ衝突なし
    if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f) {
        return false;
    }

    // AとBの中心間ベクトル（「AがBに対してどちら側にいるか」を判定）
    Vector3 centerA = (a.min + a.max) * 0.5f;
    Vector3 centerB = (b.min + b.max) * 0.5f;
    Vector3 d = centerA - centerB; // B→Aの方向

    // Y軸は絶対に選ばないよう大きな値にしておく
    // XとZのうち浅い方向に押し返す
    if (overlapX <= overlapZ) {
        // X方向に押し返す。dのX成分の符号でAをどちらに押すか決める
        float sign = (d.x >= 0.0f) ? 1.0f : -1.0f;
        outMTV = {sign * overlapX, 0.0f, 0.0f};
    } else {
        // Z方向に押し返す
        float sign = (d.z >= 0.0f) ? 1.0f : -1.0f;
        outMTV = {0.0f, 0.0f, sign * overlapZ};
    }

    return true;
}

bool CollisionManager::IsCollisionOBBCylinder(OBBCollider *obbCol, CylinderCollider *cylinder) {
    Vector3 obbCenter = obbCol->GetOBB().scaleCenterRotated;
    Vector3 cylCenter = cylinder->GetCenterPosition();

    float dx = obbCenter.x - cylCenter.x;
    float dz = obbCenter.z - cylCenter.z;
    float distXZ = std::sqrt(dx * dx + dz * dz);

    const OBB &obb = obbCol->GetOBB();
    float obbRadiusXZ = std::sqrt(obb.size.x * obb.size.x + obb.size.z * obb.size.z);
    float cylRadius = cylinder->GetRadius();
    float halfH = cylinder->GetHeight() * 0.5f;

    float obbTop = obbCenter.y + obb.size.y;
    float obbBot = obbCenter.y - obb.size.y;

    if (cylinder->IsInward()) {
        // 側面・天井・床のいずれかを超えたら衝突
        bool sideOut = (distXZ + obbRadiusXZ) > cylRadius;
        bool ceilOut = obbTop > (cylCenter.y + halfH);
        bool floorOut = obbBot < (cylCenter.y - halfH);
        return sideOut || ceilOut || floorOut;
    }

    return (distXZ - obbRadiusXZ) < cylRadius;
}

bool CollisionManager::CalculateDepenetrationOBBCylinder(OBBCollider *obbCol, CylinderCollider *cylinder, Vector3 &outMTV) {
    Vector3 obbCenter = obbCol->GetOBB().scaleCenterRotated;
    Vector3 cylCenter = cylinder->GetCenterPosition();

    float dx = obbCenter.x - cylCenter.x;
    float dz = obbCenter.z - cylCenter.z;
    float distXZ = std::sqrt(dx * dx + dz * dz);

    const OBB &obb = obbCol->GetOBB();
    float obbRadiusXZ = std::sqrt(obb.size.x * obb.size.x + obb.size.z * obb.size.z);
    float cylRadius = cylinder->GetRadius();
    float halfH = cylinder->GetHeight() * 0.5f;

    if (cylinder->IsInward()) {
        outMTV = {0.0f, 0.0f, 0.0f};
        bool hit = false;

        // ===== 側面 =====
        float penetrationXZ = (distXZ + obbRadiusXZ) - cylRadius;
        if (penetrationXZ > 0.0f) {
            if (distXZ < 0.0001f) {
                outMTV.x += 0.0f;
                outMTV.z += -penetrationXZ;
            } else {
                float nx = dx / distXZ;
                float nz = dz / distXZ;
                outMTV.x += -nx * penetrationXZ;
                outMTV.z += -nz * penetrationXZ;
            }
            hit = true;
        }

        // ===== 天井 =====
        float obbTop = obbCenter.y + obb.size.y;
        float ceilPenetration = obbTop - (cylCenter.y + halfH);
        if (ceilPenetration > 0.0f) {
            outMTV.y += -ceilPenetration; // 下に押し戻す
            hit = true;
        }

        // ===== 床 =====
        float obbBot = obbCenter.y - obb.size.y;
        float floorPenetration = (cylCenter.y - halfH) - obbBot;
        if (floorPenetration > 0.0f) {
            outMTV.y += floorPenetration; // 上に押し戻す
            hit = true;
        }

        if (!hit) {
            return false;
        }

    } else {
        // 障害物（外側に押し出す）側面のみ
        float penetration = cylRadius - (distXZ - obbRadiusXZ);
        if (penetration <= 0.0f) {
            return false;
        }
        if (distXZ < 0.0001f) {
            outMTV = {0.0f, 0.0f, penetration};
        } else {
            float nx = dx / distXZ;
            float nz = dz / distXZ;
            outMTV = {nx * penetration, 0.0f, nz * penetration};
        }
    }

    return true;
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
    // OBBのローカル空間に球を変換する方法

    // OBBの中心から球の中心へのベクトル
    Vector3 diff = sphere.center - obb.scaleCenterRotated;

    // そのベクトルをOBBのローカル座標系に変換
    Vector3 localDiff = {
        diff.Dot(obb.orientations[0]),
        diff.Dot(obb.orientations[1]),
        diff.Dot(obb.orientations[2])};

    // OBBをAABBとして扱う（ローカル空間では軸に平行）
    Vector3 halfSize = obb.size;

    // AABBの最も近い点を求める
    Vector3 closestPoint = {
        std::clamp(localDiff.x, -halfSize.x, halfSize.x),
        std::clamp(localDiff.y, -halfSize.y, halfSize.y),
        std::clamp(localDiff.z, -halfSize.z, halfSize.z)};

    // 最も近い点と球の中心（ローカル空間）との距離を計算
    Vector3 closestDiff = localDiff - closestPoint;
    float distanceSquared = closestDiff.x * closestDiff.x +
                            closestDiff.y * closestDiff.y +
                            closestDiff.z * closestDiff.z;

    return distanceSquared <= (sphere.radius * sphere.radius);
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
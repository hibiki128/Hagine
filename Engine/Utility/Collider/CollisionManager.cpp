#define NOMINMAX
#include "CollisionManager.h"
#include "Object/Object3dCommon.h"
#include "myMath.h"

std::unordered_map<std::string, Collider *> CollisionManager::colliders_;
void CollisionManager::Reset() {
    // リストを空っぽにする
    colliders_.clear();
}

// Colliderを削除する
void CollisionManager::RemoveCollider(Collider *collider) {
    // colliderが存在するか確認し、存在すれば削除
    for (auto it = colliders_.begin(); it != colliders_.end(); ++it) {
        if (it->second == collider) {
            colliders_.erase(it);
            break;
        }
    }
}

void CollisionManager::Initialize() {
}

void CollisionManager::UpdateWorldTransform() {
    for (auto &[name, collider] : colliders_) {
        if (!collider->IsCollisionEnabled()) {
            continue;
        }

        // ワールド変換行列の更新
        collider->UpdateWorldTransform();

        // 当たっているかで色を変える
        if (collider->IsCollidingInCurrentFrame()) {
            collider->SetHitColor();
        } else {
            collider->SetDefaultColor();
        }

        // フレームごとの衝突フラグリセット
        collider->ResetCollisionFlag();
    }
}

void CollisionManager::Draw(const ViewProjection &viewProjection) {
    for (auto &[name, collider] : colliders_) {
        collider->DebugDraw(viewProjection);
    }
}

void CollisionManager::Update() {
    UpdateWorldTransform();
    CheckAllCollisions();
}

void CollisionManager::CheckCollisionPair(Collider *colliderA, Collider *colliderB) {
    // コリジョンが無効化されている場合はチェックをスキップ
    if (!colliderA->IsCollisionEnabled() || !colliderB->IsCollisionEnabled()) {
        return;
    }

    // ペアをソートしてキーを生成
    auto key = std::make_pair(std::min(colliderA, colliderB), std::max(colliderA, colliderB));
    bool isCollidingNow = false;

    // 詳細な衝突判定
    // 球同士の衝突チェック
    if (colliderA->IsSphere() && colliderB->IsSphere()) {
        isCollidingNow = IsCollision(colliderA->GetSphere(), colliderB->GetSphere());
    }
    // AABB同士の衝突チェック
    else if (colliderA->IsAABB() && colliderB->IsAABB()) {
        isCollidingNow = IsCollision(colliderA->GetAABB(), colliderB->GetAABB());
    }
    // OBB同士の衝突チェック
    else if (colliderA->IsOBB() && colliderB->IsOBB()) {
        OBB obbA = colliderA->GetOBB();
        OBB obbB = colliderB->GetOBB();
        isCollidingNow = IsCollision(obbA, obbB);
    }
    // AABBと球の衝突チェック
    else if ((colliderA->IsAABB() && colliderB->IsSphere()) ||
             (colliderA->IsSphere() && colliderB->IsAABB())) {
        if (colliderA->IsAABB() && colliderB->IsSphere()) {
            isCollidingNow = IsCollision(colliderA->GetAABB(), colliderB->GetSphere());
        } else {
            isCollidingNow = IsCollision(colliderB->GetAABB(), colliderA->GetSphere());
        }
    }
    // OBBと球の衝突チェック
    else if ((colliderA->IsOBB() && colliderB->IsSphere()) ||
             (colliderA->IsSphere() && colliderB->IsOBB())) {
        if (colliderA->IsOBB() && colliderB->IsSphere()) {
            Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(colliderA->GetCenterRotation());
            isCollidingNow = IsCollision(colliderA->GetOBB(), colliderB->GetSphere(), rotateMatrix);
        } else {
            Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(colliderB->GetCenterRotation());
            isCollidingNow = IsCollision(colliderB->GetOBB(), colliderA->GetSphere(), rotateMatrix);
        }
    }
    // AABBとOBBの衝突チェック
    else if ((colliderA->IsAABB() && colliderB->IsOBB()) ||
             (colliderA->IsOBB() && colliderB->IsAABB())) {
        if (colliderA->IsAABB() && colliderB->IsOBB()) {
            isCollidingNow = IsCollision(colliderA->GetAABB(), colliderB->GetOBB());
        } else {
            isCollidingNow = IsCollision(colliderB->GetAABB(), colliderA->GetOBB());
        }
    }

    // 衝突状態の設定
    colliderA->SetIsColliding(isCollidingNow);
    colliderB->SetIsColliding(isCollidingNow);

    bool wasColliding = collisionStates[key];

    // 衝突状態の変化に応じたコールバックの呼び出し
    if (isCollidingNow) {
        colliderA->SetIsCollidingInCurrentFrame(true);
        colliderB->SetIsCollidingInCurrentFrame(true);

        // 前フレームで衝突していなかった場合に発生
        if (!wasColliding) {
            colliderA->OnCollisionEnter(colliderB);
            colliderB->OnCollisionEnter(colliderA);
        }

        // 既に衝突している場合
        colliderA->OnCollision(colliderB);
        colliderB->OnCollision(colliderA);
    } else {
        // 衝突が終わった場合
        if (wasColliding) {
            colliderA->OnCollisionOut(colliderB);
            colliderB->OnCollisionOut(colliderA);
        }
    }

    // 衝突状態の更新
    collisionStates[key] = isCollidingNow;
}

void CollisionManager::CheckAllCollisions() {
    // 全てのコライダーペアを総当たり
    for (auto itrA = colliders_.begin(); itrA != colliders_.end(); ++itrA) {
        Collider *colliderA = itrA->second;

        // Aが衝突無効ならスキップ
        if (!colliderA->IsCollisionEnabled()) {
            continue;
        }

        // itrBはitrAの次の要素から
        auto itrB = itrA;
        ++itrB;

        for (; itrB != colliders_.end(); ++itrB) {
            Collider *colliderB = itrB->second;

            // Bが衝突無効ならスキップ
            if (!colliderB->IsCollisionEnabled()) {
                continue;
            }

            // 当たり判定実行
            CheckCollisionPair(colliderA, colliderB);
        }
    }
}

void CollisionManager::AddCollider(Collider *collider) {
    std::string baseName = collider->GetName(); // 元の名前を取得
    std::string uniqueName = baseName;
    int suffix = 1;

    // 同名が存在する限り "_1", "_2", ... を追加してユニーク名にする
    while (colliders_.find(uniqueName) != colliders_.end()) {
        uniqueName = baseName + "_" + std::to_string(suffix++);
    }

    // ユニーク名をコライダーに設定
    collider->GetName() = uniqueName;

    // マップに追加
    colliders_[uniqueName] = collider;
}

bool CollisionManager::IsCollision(const AABB &aabb1, const AABB &aabb2) {
    return (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
           (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
           (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z);
}

bool CollisionManager::IsCollision(const OBB &obb1, const OBB &obb2) {
    // 15個の軸を準備
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

    // 各軸に対してSATを使って衝突判定を行う
    for (const Vector3 &axis : axes) {
        if (axis.Length() > 0.0001f && !testAxis(axis.Normalize(), obb1, obb2)) {
            return false;
        }
    }

    return true; // 全ての軸で衝突している場合はtrue
}

bool CollisionManager::IsCollision(const AABB &aabb, const Sphere &sphere) {
    // 最近接点を求める
    Vector3 closestPoint{
        std::clamp(sphere.center.x, aabb.min.x, aabb.max.x),
        std::clamp(sphere.center.y, aabb.min.y, aabb.max.y),
        std::clamp(sphere.center.z, aabb.min.z, aabb.max.z)};

    Vector3 diff = closestPoint - sphere.center;
    float distanceSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

    return distanceSquared <= (sphere.radius * sphere.radius);
}

bool CollisionManager::IsCollision(const OBB &obb, const Sphere &sphere, const Matrix4x4 &rotateMatrix) {
    // OBBのWorldMatrixを作成
    Matrix4x4 obbWorldMatrix = MakeOBBWorldMatrix(obb, rotateMatrix);

    // OBBのWorldMatrixの逆行列を取得
    Matrix4x4 obbWorldMatrixInverse = Inverse(obbWorldMatrix);

    // Sphereの中心点をOBBのローカル空間に変換
    Vector3 centerInOBBLocalSpace = Transformation(sphere.center, obbWorldMatrixInverse);

    // OBBからAABBを作成
    AABB aabbOBBLocal = ConvertOBBToAABB(obb);

    // ローカル空間でのSphere
    Sphere sphereOBBLocal{centerInOBBLocalSpace, sphere.radius};

    // ローカル空間でのAABB-Sphere判定を使用
    return IsCollision(aabbOBBLocal, sphereOBBLocal);
}

bool CollisionManager::IsCollision(const Sphere &s1, const Sphere &s2) {
    // 距離の二乗と半径の和の二乗を比較（平方根計算を避ける）
    Vector3 diff = s2.center - s1.center;
    float distanceSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    float radiusSum = s1.radius + s2.radius;

    return distanceSquared <= (radiusSum * radiusSum);
}

bool CollisionManager::IsCollision(const AABB &aabb, const OBB &obb) {
    // AABBの中心と半分のサイズを計算
    Vector3 aabbCenter = (aabb.min + aabb.max) * 0.5f;

    Vector3 aabbHalfSize = {
        (aabb.max.x - aabb.min.x) / 2.0f,
        (aabb.max.y - aabb.min.y) / 2.0f,
        (aabb.max.z - aabb.min.z) / 2.0f};

    // OBBの中心とAABBの中心との差
    Vector3 t = obb.scaleCenterRotated - aabbCenter;

    // 分離軸を定義（AABBとOBBの軸）
    Vector3 axes[15] = {
        {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, // AABBの軸
        obb.orientations[0],
        obb.orientations[1],
        obb.orientations[2],                         // OBBの軸
        Vector3(1, 0, 0).Cross(obb.orientations[0]), // AABBのXとOBBのX軸のクロス積
        Vector3(1, 0, 0).Cross(obb.orientations[1]), // AABBのXとOBBのY軸のクロス積
        Vector3(1, 0, 0).Cross(obb.orientations[2]), // AABBのXとOBBのZ軸のクロス積
        Vector3(0, 1, 0).Cross(obb.orientations[0]), // AABBのYとOBBのX軸のクロス積
        Vector3(0, 1, 0).Cross(obb.orientations[1]), // AABBのYとOBBのY軸のクロス積
        Vector3(0, 1, 0).Cross(obb.orientations[2]), // AABBのYとOBBのZ軸のクロス積
        Vector3(0, 0, 1).Cross(obb.orientations[0]), // AABBのZとOBBのX軸のクロス積
        Vector3(0, 0, 1).Cross(obb.orientations[1]), // AABBのZとOBBのY軸のクロス積
        Vector3(0, 0, 1).Cross(obb.orientations[2])  // AABBのZとOBBのZ軸のクロス積
    };

    // 各分離軸に対してチェック
    for (int i = 0; i < 15; i++) {
        if (axes[i].Length() < 1e-6)
            continue; // 0ベクトルの場合は無視
        axes[i] = axes[i].Normalize();

        // AABBの射影範囲
        float projectionAABB =
            aabbHalfSize.x * abs(axes[i].Dot(Vector3(1, 0, 0))) +
            aabbHalfSize.y * abs(axes[i].Dot(Vector3(0, 1, 0))) +
            aabbHalfSize.z * abs(axes[i].Dot(Vector3(0, 0, 1)));

        // OBBの射影範囲
        float projectionOBB = getProjection(axes[i], obb);

        // tを分離軸に射影して距離を計算
        float distance = abs(t.Dot(axes[i]));

        // 重なりがない場合は衝突していない
        if (distance > projectionAABB + projectionOBB) {
            return false;
        }
    }

    // 全ての軸で重なりがあれば衝突
    return true;
}

// 軸に対するOBBの投影範囲を計算する関数
void CollisionManager::projectOBB(const OBB &obb, const Vector3 &axis, float &min, float &max) {
    // scaleCenterRotated を使って中心位置を計算
    Vector3 rotatedCenter = obb.scaleCenterRotated;

    // 軸への投影範囲を計算
    float centerProjection = rotatedCenter.Dot(axis);

    // 回転による半径の投影を計算
    float radius =
        std::abs(obb.orientations[0].Dot(axis)) * obb.size.x +
        std::abs(obb.orientations[1].Dot(axis)) * obb.size.y +
        std::abs(obb.orientations[2].Dot(axis)) * obb.size.z;

    // 投影範囲の計算
    min = centerProjection - radius;
    max = centerProjection + radius;
}

void CollisionManager::projectAABB(const Vector3 &axis, const AABB &aabb, float &outMin, float &outMax) {
    // AABBの8つの頂点を計算する
    Vector3 vertices[8];
    vertices[0] = aabb.min;
    vertices[1] = {aabb.max.x, aabb.min.y, aabb.min.z};
    vertices[2] = {aabb.min.x, aabb.max.y, aabb.min.z};
    vertices[3] = {aabb.max.x, aabb.max.y, aabb.min.z};
    vertices[4] = {aabb.min.x, aabb.min.y, aabb.max.z};
    vertices[5] = {aabb.max.x, aabb.min.y, aabb.max.z};
    vertices[6] = {aabb.min.x, aabb.max.y, aabb.max.z};
    vertices[7] = aabb.max;

    // 初期値を設定（最初の頂点での投影）
    outMin = axis.Dot(vertices[0]);
    outMax = outMin;

    // 他の頂点も投影して、最小値と最大値を更新
    for (int i = 1; i < 8; ++i) {
        float projection = axis.Dot(vertices[i]);
        if (projection < outMin)
            outMin = projection;
        if (projection > outMax)
            outMax = projection;
    }
}

// 軸に投影するための関数
bool CollisionManager::testAxis(const Vector3 &axis, const OBB &obb1, const OBB &obb2) {
    float min1, max1, min2, max2;
    projectOBB(obb1, axis, min1, max1);
    projectOBB(obb2, axis, min2, max2);

    float sumSpan = (max1 - min1) + (max2 - min2);
    float longSpan = std::max(max1, max2) - std::min(min1, min2);

    return sumSpan >= longSpan;
}

bool CollisionManager::testAxis(const Vector3 &axis, const AABB &aabb, const OBB &obb) {
    float aabbMin, aabbMax;
    projectAABB(axis, aabb, aabbMin, aabbMax);

    float obbMin, obbMax;
    projectOBB(obb, axis, obbMin, obbMax);

    float sumSpan = (aabbMax - aabbMin) + (obbMax - obbMin);
    float longSpan = std::max(aabbMax, obbMax) - std::min(aabbMin, obbMin);

    return sumSpan >= longSpan;
}

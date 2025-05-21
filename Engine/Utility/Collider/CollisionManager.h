#pragma once

#include "Collider.h"
#include "Object/Object3d.h"
#include "SceneManager.h"
#include "list"
#include "myMath.h"
class CollisionManager {
  public:
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &pair) const {
            auto hash1 = std::hash<T1>{}(pair.first);
            auto hash2 = std::hash<T2>{}(pair.second);
            return hash1 ^ hash2;
        }
    };

  private:

    // コライダー
    static std::unordered_map<std::string, Collider *> colliders_;
    std::unordered_map<std::pair<Collider *, Collider *>, bool, pair_hash> collisionStates;
    bool isCollidingNow = false;

  public:
    /// <summary>
    /// リセット
    /// </summary>
    static void Reset();

    static void RemoveCollider(Collider *collider);

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// ワールドトランスフォームの更新
    /// </summary>
    void UpdateWorldTransform();

    /// <summary>
    /// 描画
    /// </summary>
    /// <param name="viewProjection"></param>
    void Draw(const ViewProjection &viewProjection);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 衝突判定
    /// </summary>
    /// <param name="colliderA"></param>
    /// <param name="colliderB"></param>
    void CheckCollisionPair(Collider *colliderA, Collider *colliderB);

    /// <summary>
    /// 全ての当たり判定チェック
    /// </summary>
    void CheckAllCollisions();

    /// <summary>
    /// コライダーの登録
    /// </summary>
    static void AddCollider(Collider *collider);

  private:
    bool IsCollision(const AABB &aabb1, const AABB &aabb2);
    bool IsCollision(const OBB &obb1, const OBB &obb2);
    bool IsCollision(const AABB &aabb, const Sphere &sphere);
    bool IsCollision(const OBB &obb, const Sphere &sphere, const Matrix4x4 &rotateMatrix);
    bool IsCollision(const Sphere &s1, const Sphere &s2);
    bool IsCollision(const AABB &aabb, const OBB &obb);

    // 軸に対するOBBの投影範囲を計算する関数
    void projectOBB(const OBB &obb, const Vector3 &axis, float &min, float &max);
    void projectAABB(const Vector3 &axis, const AABB &aabb, float &outMin, float &outMax);

    // 軸に投影するための関数
    bool testAxis(const Vector3 &axis, const OBB &obb1, const OBB &obb2);
    bool testAxis(const Vector3 &axis, const AABB &aabb, const OBB &obb);
};

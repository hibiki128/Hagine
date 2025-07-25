#pragma once
#include "Data/DataHandler.h"
#include "Object/Object3d.h"
#include "type/Vector3.h"
#include "externals/nlohmann/json.hpp"
#include <filesystem>
#include <fstream>

class Collider {
  public:
    enum class CollisionType {
        Sphere,
        AABB,
        OBB
    };

  public:
    Collider();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~Collider();

    /// <summary>
    /// 初期化
    /// </summary>
    Collider &AddCollider(const std::string &objName);

    /// <summary>
    /// ワールドトランスフォームの更新
    /// </summary>
    void UpdateWorldTransform();

    void DebugDraw(const ViewProjection &viewProjection);

    void OffsetImgui();

    /// <summary>
    /// 当たってる間
    /// </summary>
    /// <param name="other"></param>
    virtual void OnCollision([[maybe_unused]] Collider *other) {};

    /// <summary>
    /// 当たった瞬間
    /// </summary>
    /// <param name="other"></param>
    virtual void OnCollisionEnter([[maybe_unused]] Collider *other) {};

    /// <summary>
    /// 当たり終わった瞬間
    /// </summary>
    /// <param name="other"></param>
    virtual void OnCollisionOut([[maybe_unused]] Collider *other) {};

    void SaveToJson();

#pragma region ゲッター
    /// <summary>
    /// getter
    /// </summary>
    /// <returns></returns>
    // 半径を取得
    float GetRadius() { return radius_; }
    // 中心座標を取得
    virtual Vector3 GetCenterPosition() const = 0;
    virtual Vector3 GetCenterRotation() const = 0;

    AABB GetAABB() { return aabb_; }
    OBB GetOBB() { return obb_; }
    Sphere GetSphere() { return sphere_; }
    bool IsCollisionEnabled() const { return isCollisionEnabled_; }
    bool IsColliding() const { return isColliding_; }
    bool WasColliding() const { return wasColliding_; }
    bool IsOBB() { return isOBB_; }
    bool IsSphere() { return isSphere_; }
    bool IsAABB() { return isAABB_; }
    bool IsVisible() { return isVisible_; }

    std::string &GetName() { return objName_; }

#pragma endregion

#pragma region セッター

    void SetIsCollidingInCurrentFrame(bool isColliding) {
        isCollidingInCurrentFrame_ = isColliding;
    }

    bool IsCollidingInCurrentFrame() const {
        return isCollidingInCurrentFrame_;
    }

    void ResetCollisionFlag() {
        isCollidingInCurrentFrame_ = false;
    }

    /// <summary>
    /// setter
    /// </summary>
    /// <param name="radius"></param>
    void SetOBBSize(Vector3 size) { OBBOffset_.size = size; }
    void SetOBBCenter(Vector3 center) { OBBOffset_.scaleCenter = center; }
    void SetRadius(float radius) { radius_ = radius; }
    void SetIsColliding(bool colliding) { isColliding_ = colliding; }
    void SetWasColliding(bool wasColliding) { wasColliding_ = wasColliding; }
    void SetCollisionEnabled(bool enabled) { isCollisionEnabled_ = enabled; }
    void SetHitColor() { color_ = {1.0f, 0.0f, 0.0f, 1.0f}; }
    void SetDefaultColor() { color_ = {1.0f, 1.0f, 1.0f, 1.0f}; }
    void SetCollisionType(CollisionType collisionType);
    void SetVisible(bool isVisible) { isVisible_ = isVisible; }

#pragma endregion

  private:
    void MakeOBBOrientations(OBB &obb, const Vector3 &rotate);
    void UpdateOBB();
    void LoadFromJson();

#pragma region デバッグ描画
    void DrawSphere(const ViewProjection &viewProjection);

    void DrawAABB(const ViewProjection &viewProjection);

    void DrawOBB(const ViewProjection &viewProjection);

    void DrawRotationCenter(const ViewProjection &viewProjection);

    // 球を描画する関数
    void DrawSphereAtCenter(const ViewProjection &viewProjection, const Vector3 &center, float radius);
#pragma endregion

  private:
    // 衝突半径
    float radius_ = 1.0f;

    std::unique_ptr<Object3d> Sphere_;
    std::unique_ptr<Object3d> AABB_;
    std::unique_ptr<Object3d> OBB_;
    std::unique_ptr<DataHandler> ColliderDatas_;

    AABB aabb_;
    OBB obb_;
    Sphere sphere_;
    Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};

    static int counter; // 静的カウンタ
    Sphere SphereOffset_;
    AABB AABBOffset_;
    OBB OBBOffset_;
    std::string objName_;

    bool isCollisionEnabled_ = true;         // デフォルトではコリジョンを有効化
    bool isColliding_ = false;               // 現在のフレームの衝突状態
    bool wasColliding_ = false;              // 前フレームの衝突状態
    bool isCollidingInCurrentFrame_ = false; // 現フレームで衝突しているか

    bool isAABB_ = true;
    bool isOBB_ = true;
    bool isSphere_ = true;
    bool isVisible_ = true;
};
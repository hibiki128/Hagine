#pragma once
#define NOMINMAX
#include "ColliderBase.h"
#include "type/AABBCollider.h"
#include "type/CylinderCollider.h"
#include "type/OBBCollider.h"
#include "type/SphereCollider.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <unordered_map>
#include <vector>

namespace Hagine {

/// <summary>
/// 全コライダーの登録・更新・衝突判定を統括するシングルトン
/// タグごとにグループ化し、衝突マスクに基づいてペアの判定を行う
/// </summary>
class CollisionManager {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// インスタンスを取得
    /// </summary>
    /// <returns>CollisionManager*: シングルトンインスタンス</returns>
    static CollisionManager *GetInstance() {
        static CollisionManager instance;
        return &instance;
    }

    /// <summary>
    /// コライダーを登録
    /// </summary>
    /// <param name="collider">登録するコライダー</param>
    void Register(ColliderBase *collider);

    /// <summary>
    /// コライダーの登録を解除
    /// </summary>
    /// <param name="collider">解除するコライダー</param>
    void Unregister(ColliderBase *collider);

    /// <summary>
    /// 登録済みコライダーを全てクリア
    /// </summary>
    void Clear();

    /// <summary>
    /// コライダーのタグ変更に追従してグループを更新
    /// </summary>
    /// <param name="collider">対象のコライダー</param>
    /// <param name="oldTag">変更前のタグ</param>
    /// <param name="newTag">変更後のタグ</param>
    void UpdateColliderTag(ColliderBase *collider, const std::string &oldTag, const std::string &newTag);

    /// <summary>
    /// 更新処理（ワールド変換更新と衝突判定）
    /// </summary>
    void Update();

    /// <summary>
    /// 全コライダーのデバッグ描画
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DebugDraw(const ViewProjection &viewProjection);

    /// <summary>
    /// OBB同士のめり込み解消ベクトル（MTV）を計算
    /// </summary>
    /// <param name="a">OBBコライダーA</param>
    /// <param name="b">OBBコライダーB</param>
    /// <param name="outMTV">出力されるめり込み解消ベクトル</param>
    /// <returns>bool: めり込みがあれば true</returns>
    bool CalculateDepenetration(OBBCollider *a, OBBCollider *b, Vector3 &outMTV);

    /// <summary>
    /// AABB同士のめり込み解消ベクトル（MTV）を計算
    /// </summary>
    /// <param name="a">AABBコライダーA</param>
    /// <param name="b">AABBコライダーB</param>
    /// <param name="outMTV">出力されるめり込み解消ベクトル</param>
    /// <returns>bool: めり込みがあれば true</returns>
    bool CalculateDepenetration(AABBCollider *a, AABBCollider *b, Vector3 &outMTV);

    /// <summary>
    /// OBBと円柱の衝突判定
    /// </summary>
    /// <param name="obb">OBBコライダー</param>
    /// <param name="cylinder">円柱コライダー</param>
    /// <returns>bool: 衝突していれば true</returns>
    bool IsCollisionOBBCylinder(OBBCollider *obb, CylinderCollider *cylinder);

    /// <summary>
    /// OBBと円柱のめり込み解消ベクトル（MTV）を計算
    /// </summary>
    /// <param name="obb">OBBコライダー</param>
    /// <param name="cylinder">円柱コライダー</param>
    /// <param name="outMTV">出力されるめり込み解消ベクトル</param>
    /// <returns>bool: めり込みがあれば true</returns>
    bool CalculateDepenetrationOBBCylinder(OBBCollider *obb, CylinderCollider *cylinder, Vector3 &outMTV);

#ifdef _DEBUG
    /// <summary>
    /// タグの追加・削除UI（タグ管理タブ）
    /// </summary>
    void ImGuiTagManager() {
        ColliderTagManager::GetInstance()->ImGuiTagManager();
    }

    /// <summary>
    /// コライダーの選択・サイズ調整・デバッグ表示切り替えUI（コライダー設定タブ）。
    /// 登録済みコライダーを一覧から選び、サイズ・色・表示/判定の有効を個別に編集できる
    /// </summary>
    void ImGuiColliderInspector();
#endif

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    CollisionManager() = default;

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~CollisionManager() = default;
    CollisionManager(const CollisionManager &) = delete;
    CollisionManager &operator=(const CollisionManager &) = delete;

    /// <summary>
    /// 全コライダーのワールド変換を更新
    /// </summary>
    void UpdateColliders();

    /// <summary>
    /// 衝突対象となるペアを総当たりで判定
    /// </summary>
    void CheckCollisions();

    /// <summary>
    /// コライダーペアの衝突状態を判定しコールバックを発火
    /// </summary>
    /// <param name="a">コライダーA</param>
    /// <param name="b">コライダーB</param>
    void CheckCollisionPair(ColliderBase *a, ColliderBase *b);

    /// <summary>
    /// 2つのコライダーの形状に応じた衝突判定を実行
    /// </summary>
    /// <param name="a">コライダーA</param>
    /// <param name="b">コライダーB</param>
    /// <returns>bool: 衝突していれば true</returns>
    bool TestCollision(ColliderBase *a, ColliderBase *b);

    /// 各種衝突判定関数
    bool IsCollision(const Sphere &s1, const Sphere &s2);
    bool IsCollision(const AABB &aabb1, const AABB &aabb2);
    bool IsCollision(const OBB &obb1, const OBB &obb2);
    bool IsCollision(const AABB &aabb, const Sphere &sphere);
    bool IsCollision(const OBB &obb, const Sphere &sphere);
    bool IsCollision(const AABB &aabb, const OBB &obb);

    /// 分離軸判定のヘルパー関数
    void ProjectOBB(const OBB &obb, const Vector3 &axis, float &min, float &max);
    void ProjectAABB(const Vector3 &axis, const AABB &aabb, float &outMin, float &outMax);
    bool TestAxis(const Vector3 &axis, const OBB &obb1, const OBB &obb2);
    bool TestAxis(const Vector3 &axis, const AABB &aabb, const OBB &obb);

    /// ===================================================
    /// private variants
    /// ===================================================

    // タグごとにコライダーをグループ化（文字列キー対応）
    std::unordered_map<std::string, std::vector<ColliderBase *>> collidersByTag_;

    /// <summary>
    /// 衝突状態を管理するためのコライダーペア
    /// </summary>
    struct CollisionPair {
        ColliderBase *a;
        ColliderBase *b;

        bool operator==(const CollisionPair &other) const {
            return (a == other.a && b == other.b) || (a == other.b && b == other.a);
        }
    };

    /// <summary>
    /// コライダーペアのハッシュ関数（順序非依存）
    /// </summary>
    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair &pair) const {
            auto ptrA = reinterpret_cast<std::uintptr_t>(pair.a);
            auto ptrB = reinterpret_cast<std::uintptr_t>(pair.b);
            return std::hash<std::uintptr_t>{}((std::min)(ptrA, ptrB)) ^
                   (std::hash<std::uintptr_t>{}((std::max)(ptrA, ptrB)) << 1);
        }
    };

    std::unordered_map<CollisionPair, bool, CollisionPairHash> collisionStates_; // ペアごとの衝突状態

    bool isVisible_ = false; // コライダーのデバッグ表示フラグ（全体）

#ifdef _DEBUG
    ColliderBase *inspectorSelected_ = nullptr; // インスペクタで選択中のコライダー
#endif
};
} // namespace Hagine

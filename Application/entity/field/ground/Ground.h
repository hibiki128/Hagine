#pragma once
#include "object/base/BaseObject.h"

/// <summary>
/// 地面のゲームオブジェクトクラス
/// ステージの基盤となる固定オブジェクト。
/// 隆起した地形モデルからメッシュコライダーを構築し、
/// 地形表面の高さ問い合わせ（接地・カメラ用）を静的APIとして提供する
/// </summary>
class Ground : public Hagine::BaseObject
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デストラクタ（アクティブ地形の登録を解除する）
    /// </summary>
    ~Ground() override;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="className">クラス名</param>
    void Init(const std::string className) override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void Draw(const Hagine::ViewProjection &viewProjection) override;

    /// <summary>
    /// ライティングの有効フラグを設定
    /// </summary>
    /// <param name="isLighting">有効にするか</param>
    void SetLighting(bool isLighting) { isLighting_ = isLighting; }

    /// ===================================================
    /// 地形問い合わせ（静的API）
    /// ===================================================

    /// <summary>
    /// アクティブな地形メッシュコライダーを取得（地面が存在しなければ nullptr）
    /// </summary>
    static Hagine::MeshCollider *GetTerrainCollider() { return activeTerrain_; }

    /// <summary>
    /// 指定XZ位置の地形表面の高さ（ワールドY）を取得する。
    /// 地形が無い・地形の外に出ている場合は平地の高さを返す
    /// </summary>
    /// <param name="x">ワールドX座標</param>
    /// <param name="z">ワールドZ座標</param>
    /// <returns>float: 地形表面のワールドY座標</returns>
    static float GetSurfaceY(float x, float z);

    /// <summary>
    /// キャラクターの接地基準Y（地形表面＋立ちオフセット）を取得する。
    /// 平地では従来の地面レベル 0.0 と一致する
    /// </summary>
    /// <param name="x">ワールドX座標</param>
    /// <param name="z">ワールドZ座標</param>
    /// <returns>float: 接地とみなすワールドY座標</returns>
    static float GetStandingY(float x, float z);

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    static constexpr float kFallbackSurfaceY = -1.0f; // 地形が無い場合の地表高さ（平地部分の高さ）
    static constexpr float kStandOffset = 1.0f;       // キャラ原点と地表の距離（従来の地面レベル0を維持する値）
    static constexpr float kProbeHeight = 100.0f;     // 高さ問い合わせレイの開始高度
    static constexpr float kProbeLength = 400.0f;     // 高さ問い合わせレイの長さ

    Hagine::MeshCollider *pTerrainCollider_ = nullptr; // 自身の地形メッシュコライダー

    static inline Hagine::MeshCollider *activeTerrain_ = nullptr; // アクティブシーンの地形コライダー
};

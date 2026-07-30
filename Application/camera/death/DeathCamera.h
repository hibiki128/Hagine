#pragma once
#include <camera/projection/ViewProjection.h>
#include <transform/WorldTransform.h>

/// <summary>
/// プレイヤー死亡時のカメラ演出を行うクラス
/// 「イージング進捗の計算」「トランスフォームの補間」「ビューへの反映」「注視回転の算出」を
/// それぞれ独立したメソッドに分けている
/// </summary>
class DeathCamera
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Init();

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// イージング演出の開始
    /// </summary>
    /// <param name="currentViewProjection">現在のViewProjection</param>
    /// <param name="targetPosition">プレイヤーの位置</param>
    void StartEasing(const Hagine::ViewProjection &currentViewProjection, const Hagine::Vector3 &targetPosition);

    /// <summary>
    /// デバッグ用のImGui表示
    /// </summary>
    void DrawImGui();

    /// ===================================================
    /// Getter
    /// ===================================================

    /// <summary>
    /// ビュープロジェクションを取得
    /// </summary>
    /// <returns>ViewProjection&amp;: ビュープロジェクション参照</returns>
    Hagine::ViewProjection &GetViewProjection() { return viewProjection_; }

    /// <summary>
    /// イージング完了フラグを取得
    /// </summary>
    /// <returns>bool: 完了していればtrue</returns>
    bool IsComplete() const { return isComplete_; }

    /// <summary>
    /// イージング中間地点フラグを取得
    /// </summary>
    /// <returns>bool: 中間地点に到達していればtrue</returns>
    bool IsHalfway() const { return isHalfway_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// タイマーを進め、0〜1に収めたイージング進捗を返す
    /// </summary>
    /// <returns>float: イージング進捗（0.0〜1.0）</returns>
    float AdvanceEasingProgress();

    /// <summary>
    /// 進捗に応じて位置・回転を補間し、ワールドトランスフォームへ適用する
    /// </summary>
    /// <param name="progress">イージング進捗（0.0〜1.0）</param>
    void ApplyEasedTransform(float progress);

    /// <summary>
    /// ワールドトランスフォームの内容をビュープロジェクションへ反映する
    /// </summary>
    void ApplyToViewProjection();

    /// <summary>
    /// 指定位置から注視点を向く回転を算出する
    /// </summary>
    /// <param name="eyePosition">カメラの位置</param>
    /// <param name="lookAtPosition">注視点</param>
    /// <returns>Quaternion: 注視点を向く回転</returns>
    static Hagine::Quaternion CalcLookAtRotation(const Hagine::Vector3 &eyePosition,
                                                const Hagine::Vector3 &lookAtPosition);

    /// ===================================================
    /// private variables
    /// ===================================================

    // カメラ設定
    static constexpr float kFarZ = 1100.0f; ///< 描画距離の遠面

    // イージング進捗（0.0で開始、1.0で完了）
    static constexpr float kEasingStart = 0.0f;    ///< 進捗の下限（開始）
    static constexpr float kEasingComplete = 1.0f; ///< 進捗の上限（完了）
    static constexpr float kHalfwayRatio = 0.5f;   ///< 中間地点と判定する進捗の割合

    // 注視回転の算出に使うしきい値（基準ベクトルは Hagine::kWorldUp / kWorldRight を使う）
    static constexpr float kParallelThreshold = 0.999f; ///< 上方向と平行とみなす内積のしきい値

    Hagine::ViewProjection viewProjection_; ///< ビュープロジェクション
    Hagine::WorldTransform worldTransform_; ///< ワールドトランスフォーム

    bool isEasing_ = false;       ///< イージング中フラグ
    bool isComplete_ = false;     ///< 完了フラグ
    bool isHalfway_ = false;      ///< 中間地点到達フラグ
    float easingTimer_ = 0.0f;    ///< イージングタイマー
    float easingDuration_ = 0.8f; ///< イージング時間

    Hagine::Vector3 easingStartPos_;     ///< イージング開始位置
    Hagine::Quaternion easingStartRot_;  ///< イージング開始回転
    Hagine::Vector3 easingTargetPos_;    ///< イージング目標位置
    Hagine::Quaternion easingTargetRot_; ///< イージング目標回転

    // プレイヤーからのオフセット（正面やや斜め上）
    Hagine::Vector3 cameraOffset_ = {3.0f, 2.5f, 8.0f}; ///< カメラオフセット
};

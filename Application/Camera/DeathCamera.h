#pragma once
#include <Camera/ViewProjection/ViewProjection.h>
#include <Transform/WorldTransform.h>

/// <summary>
/// プレイヤー死亡時のカメラ演出を行うクラス
/// </summary>
class DeathCamera {
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
    /// <param name="currentVp">現在のViewProjection</param>
    /// <param name="targetPosition">プレイヤーの位置</param>
    void StartEasing(const ViewProjection &currentVp, const Vector3 &targetPosition);

    /// <summary>
    /// デバッグ用のImGui表示
    /// </summary>
    void imgui();

    /// ===================================================
    /// Getter
    /// ===================================================

    /// <summary>
    /// ビュープロジェクションを取得
    /// </summary>
    /// <returns>ViewProjection&: ビュープロジェクション参照</returns>
    ViewProjection &GetViewProjection() { return vp_; }

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
    /// private variants
    /// ===================================================

    // カメラ設定
    static constexpr float kFarZ = 1100.0f; // 描画距離の遠面

    // イージング設定
    static constexpr float kHalfwayRatio = 0.5f;        // 中間地点の割合
    static constexpr float kEasingEndThreshold = 1.0f;  // イージング終了しきい値
    static constexpr float kEasingMaxValue = 1.0f;      // イージングの最大値
    static constexpr float kTimerReset = 0.0f;          // タイマーリセット値

    // ベクトル設定
    static constexpr float kParallelThreshold = 0.999f; // 平行判定のしきい値
    static constexpr float kUpVectorX = 0.0f;           // 上方向ベクトルのX成分
    static constexpr float kUpVectorY = 1.0f;           // 上方向ベクトルのY成分
    static constexpr float kUpVectorZ = 0.0f;           // 上方向ベクトルのZ成分
    static constexpr float kRightVectorX = 1.0f;        // 右方向ベクトルのX成分
    static constexpr float kRightVectorY = 0.0f;        // 右方向ベクトルのY成分
    static constexpr float kRightVectorZ = 0.0f;        // 右方向ベクトルのZ成分

    ViewProjection vp_;    ///< ビュープロジェクション
    WorldTransform wt_;    ///< ワールドトランスフォーム

    bool isEasing_ = false;       ///< イージング中フラグ
    bool isComplete_ = false;     ///< 完了フラグ
    bool isHalfway_ = false;      ///< 中間地点到達フラグ
    float easingTimer_ = 0.0f;    ///< イージングタイマー
    float easingDuration_ = 0.8f; ///< イージング時間

    Vector3 easingStartPos_;      ///< イージング開始位置
    Quaternion easingStartRot_;   ///< イージング開始回転
    Vector3 easingTargetPos_;     ///< イージング目標位置
    Quaternion easingTargetRot_;  ///< イージング目標回転

    // プレイヤーからのオフセット（正面やや斜め上）
    Vector3 cameraOffset_ = {3.0f, 2.5f, 8.0f}; ///< カメラオフセット
};

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
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// イージング開始
    /// </summary>
    /// <param name="currentVp">現在のViewProjection</param>
    /// <param name="targetPosition">プレイヤーの位置</param>
    void StartEasing(const ViewProjection &currentVp, const Vector3 &targetPosition);

    /// <summary>
    /// デバッグ関数
    /// </summary>
    void imgui();

    /// <summary>
    /// Getter
    /// </summary>
    ViewProjection &GetViewProjection() { return vp_; }
    bool IsComplete() const { return isComplete_; }
    bool IsHalfway() const { return isHalfway_; }

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // カメラ設定
    static constexpr float kFarZ = 1100.0f;

    // イージング設定
    static constexpr float kHalfwayRatio = 0.5f;
    static constexpr float kEasingEndThreshold = 1.0f;
    static constexpr float kEasingMaxValue = 1.0f;
    static constexpr float kTimerReset = 0.0f;

    // ベクトル設定
    static constexpr float kParallelThreshold = 0.999f;
    static constexpr float kUpVectorX = 0.0f;
    static constexpr float kUpVectorY = 1.0f;
    static constexpr float kUpVectorZ = 0.0f;
    static constexpr float kRightVectorX = 1.0f;
    static constexpr float kRightVectorY = 0.0f;
    static constexpr float kRightVectorZ = 0.0f;

    ViewProjection vp_;    // ビュープロジェクション
    WorldTransform wt_;    // ワールドトランスフォーム

    bool isEasing_ = false;       // イージング中フラグ
    bool isComplete_ = false;     // 完了フラグ
    bool isHalfway_ = false;      // 中間地点到達フラグ
    float easingTimer_ = 0.0f;    // イージングタイマー
    float easingDuration_ = 0.8f; // イージング時間

    Vector3 easingStartPos_;      // イージング開始位置
    Quaternion easingStartRot_;   // イージング開始回転
    Vector3 easingTargetPos_;     // イージング目標位置
    Quaternion easingTargetRot_;  // イージング目標回転

    // プレイヤーからのオフセット（正面やや斜め上）
    Vector3 cameraOffset_ = {3.0f, 2.5f, 8.0f}; // カメラオフセット
};
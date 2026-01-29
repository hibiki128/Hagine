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
    void StartEasing(const Hagine::Camera::ViewProjection &currentVp, const Hagine::Math::Vector3 &targetPosition);

    /// <summary>
    /// デバッグ関数
    /// </summary>
    void imgui();

    /// <summary>
    /// Getter
    /// </summary>
    Hagine::Camera::ViewProjection &GetViewProjection() { return vp_; }
    bool IsComplete() const { return isComplete_; }
    bool IsHalfway() const { return isHalfway_; }

  private:
    /// ===================================================
    /// private varians
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

    Hagine::Camera::ViewProjection vp_;
    Hagine::Transform::WorldTransform wt_;

    bool isEasing_ = false;
    bool isComplete_ = false;
    bool isHalfway_ = false;
    float easingTimer_ = 0.0f;
    float easingDuration_ = 0.8f;

    Hagine::Math::Vector3 easingStartPos_;
    Hagine::Math::Quaternion easingStartRot_;
    Hagine::Math::Vector3 easingTargetPos_;
    Hagine::Math::Quaternion easingTargetRot_;

    // プレイヤーからのオフセット（正面やや斜め上）
    Hagine::Math::Vector3 cameraOffset_ = {3.0f, 2.5f, 8.0f};
};
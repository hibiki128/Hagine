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
    /// private varians
    /// ===================================================

    ViewProjection vp_;
    WorldTransform wt_;

    bool isEasing_ = false;
    bool isComplete_ = false;
    bool isHalfway_ = false; 
    float easingTimer_ = 0.0f;
    float easingDuration_ = 0.8f;

    Vector3 easingStartPos_;
    Quaternion easingStartRot_;
    Vector3 easingTargetPos_;
    Quaternion easingTargetRot_;

    // プレイヤーからのオフセット（正面やや斜め上）
    Vector3 cameraOffset_ = {3.0f, 2.5f, 8.0f};
};
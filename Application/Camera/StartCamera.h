#pragma once
#include <Camera/ViewProjection/ViewProjection.h>
#include <Transform/WorldTransform.h>

/// <summary>
/// スタート時のカメラの動きを行うカメラクラス
/// </summary>
class StartCamera {
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
    /// デバッグ関数
    /// </summary>
    void imgui();

    /// <summary>
    /// カメラのViewProjectionを取得する関数
    /// </summary>
    /// <returns> ViewProjection </returns>
    ViewProjection &GetViewProjection() { return vp_; }

    /// <summary>
    /// カメラの動きの関数
    /// </summary>
    void Move();

    /// <summary>
    /// カメラの動きが完了したかを取得
    /// </summary>
    /// <returns> 完了フラグ </returns>
    bool IsComplete() const { return isComplete_; }

    void SetTargetVp(ViewProjection &vp) {
        targetVp_.matWorld_ = vp.matWorld_;
        targetVp_.matView_ = vp.matView_;
        targetVp_.matProjection_ = vp.matProjection_;
        targetVp_.translation_ = vp.translation_;
        targetVp_.eulerRotation_ = vp.eulerRotation_;
    }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    // ビュープロジェクション
    ViewProjection vp_;

    ViewProjection targetVp_;

    WorldTransform wt_;

    float speed_ = 1.0f;
    float angle_ = 0.0f;
    float radius_ = 60.0f;
    Vector3 centerPos_ = {0.0f, 0.0f, -21.0f};

    bool isEasing_ = false;
    bool isComplete_ = false;
    float easingTimer_ = 0.0f;
    float easingDuration_ = 2.0f;
    float finalWaitDuration_ = 1.0f;
    Vector3 easingStartPos_;
    Vector3 easingStartRot_;
    Vector3 easingTargetPos_ = {-6.0f, 1.8f, -7.40f};
    Vector3 easingTargetRot_ = {degreesToRadians(8.6f), degreesToRadians(40.0f), degreesToRadians(0.0f)};
    int easingPhase_ = 0;
    float waitDuration_ = 1.0f;
    Vector3 easingTargetPos2_ = {5.0f, 1.8f, -33.0f};
    Vector3 easingTargetRot2_ = {degreesToRadians(9.6f), degreesToRadians(-149.0f), degreesToRadians(0.0f)};
};

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
    /// カメラの動きの関数
    /// </summary>
    void Move();

    /// <summary>
    /// Getter
    /// </summary>
    Hagine::Camera::ViewProjection &GetViewProjection() { return vp_; }
    bool IsComplete() const { return isComplete_; }

    /// <summary>
    /// Setter
    /// </summary>
    void SetTargetVp(Hagine::Camera::ViewProjection &vp) {
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

    // カメラ設定
    static constexpr float kFarZ = 1100.0f;
    static constexpr float kInitialHeight = 42.0f;
    static constexpr float kInitialAngleDegrees = -90.0f;

    // 数値定数
    static constexpr float kTimerReset = 0.0f;
    static constexpr float kMaxBlendValue = 1.0f;
    static constexpr float kEasingMaxValue = 1.0f;
    static constexpr float kZeroRotation = 0.0f;
    static constexpr float kHalfPi = 0.5f * std::numbers::pi_v<float>;

    // フェーズ定数
    static constexpr int kPhaseEasing1 = 1;
    static constexpr int kPhaseWait1 = 2;
    static constexpr int kPhaseEasing2 = 3;
    static constexpr int kPhaseWait2 = 4;
    static constexpr int kPhaseEasing3 = 5;
    static constexpr int kPhaseWait3 = 6;
    static constexpr int kPhaseComplete = 7;

    // ビュープロジェクション
    Hagine::Camera::ViewProjection vp_;

   Hagine::Camera::ViewProjection targetVp_;

    Hagine::Transform::WorldTransform wt_;

    float speed_ = 1.5f;
    float angle_ = 0.0f;
    float radius_ = 60.0f;
    Hagine::Math::Vector3 centerPos_ = {0.0f, 0.0f, -21.0f};

    bool isEasing_ = false;
    bool isComplete_ = false;
    float easingTimer_ = 0.0f;
    float easingDuration_ = 2.0f;
    float finalWaitDuration_ = 1.0f;
    Hagine::Math::Vector3 easingStartPos_;
    Hagine::Math::Vector3 easingStartRot_;
    Hagine::Math::Vector3 easingTargetPos_ = {-6.0f, 1.8f, -7.40f};
    Hagine::Math::Vector3 easingTargetRot_ = {Hagine::Math::degreesToRadians(8.6f), Hagine::Math::degreesToRadians(40.0f), Hagine::Math::degreesToRadians(0.0f)};
    int easingPhase_ = 0;
    float waitDuration_ = 1.0f;
    Hagine::Math::Vector3 easingTargetPos2_ = {5.0f, 1.8f, -33.0f};
    Hagine::Math::Vector3 easingTargetRot2_ = {Hagine::Math::degreesToRadians(9.6f), Hagine::Math::degreesToRadians(-149.0f), Hagine::Math::degreesToRadians(0.0f)};
};
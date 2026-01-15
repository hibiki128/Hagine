#pragma once
#include <Camera/ViewProjection/ViewProjection.h>
#include <Transform/WorldTransform.h>

// 前方宣言
class Input;
class GamePad;

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
    ViewProjection &GetViewProjection() { return vp_; }
    bool IsComplete() const { return isComplete_; }

    /// <summary>
    /// Setter
    /// </summary>
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

    /// <summary>
    /// スキップ入力チェック
    /// </summary>
    bool CheckSkipInput();

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

    // スキップ時のスピード倍率
    static constexpr float kSkipSpeedMultiplier = 5.0f;

    // ビュープロジェクション
    ViewProjection vp_;

    ViewProjection targetVp_;

    WorldTransform wt_;

    float speed_ = 1.5f;
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

    // 入力関連
    Input *input_ = nullptr;
    std::unique_ptr<GamePad> gamePad_ = nullptr;

    // スキップ関連
    bool isSkipping_ = false;
};
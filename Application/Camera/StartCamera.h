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
    /// private variants
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
    ViewProjection vp_;       // ビュープロジェクション
    ViewProjection targetVp_; // 目標のビュープロジェクション
    WorldTransform wt_;       // ワールドトランスフォーム

    float speed_ = 1.5f;   // 回転速度
    float angle_ = 0.0f;   // 現在の角度
    float radius_ = 60.0f; // 回転半径
    Vector3 centerPos_ = {0.0f, 0.0f, -21.0f}; // 中心座標

    bool isEasing_ = false;           // イージング中フラグ
    bool isComplete_ = false;         // 完了フラグ
    float easingTimer_ = 0.0f;        // イージングタイマー
    float easingDuration_ = 2.0f;     // イージング時間
    float finalWaitDuration_ = 1.0f;  // 最終待機時間
    Vector3 easingStartPos_;          // イージング開始位置
    Vector3 easingStartRot_;          // イージング開始回転
    Vector3 easingTargetPos_ = {-6.0f, 1.8f, -7.40f}; // イージング目標位置1
    Vector3 easingTargetRot_ = {degreesToRadians(8.6f), degreesToRadians(40.0f), degreesToRadians(0.0f)}; // イージング目標回転1
    int easingPhase_ = 0;             // イージングフェーズ
    float waitDuration_ = 1.0f;       // 待機時間
    Vector3 easingTargetPos2_ = {5.0f, 1.8f, -33.0f}; // イージング目標位置2
    Vector3 easingTargetRot2_ = {degreesToRadians(9.6f), degreesToRadians(-149.0f), degreesToRadians(0.0f)}; // イージング目標回転2

    // 入力関連
    Input *input_ = nullptr; // 入力
    std::unique_ptr<GamePad> gamePad_ = nullptr; // ゲームパッド

    // スキップ関連
    bool isSkipping_ = false; // スキップ中フラグ
};
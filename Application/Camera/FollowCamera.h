#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Easing.h"
#include "Transform/WorldTransform.h"

/// <summary>
/// ターゲットを追従するカメラクラス
/// </summary>

class Player;
class FollowCamera {
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
    /// Getter
    /// </summary>
    float GetYaw() { return yaw_; }
    ViewProjection &GetViewProjection() { return viewProjection_; }

    /// <summary>
    /// Setter
    /// </summary>
    void SetPlayer(Player *target) { target_ = target; }
    void SetCameraFov(float fov) {
        viewProjection_.fovAngleY = fov * std::numbers::pi_v<float> / 180.0f;
    }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// カメラの動きの関数
    /// </summary>
    void Move();

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    // カメラ設定
    static constexpr float kFarZ = 1100.0f;

    // 閾値定数
    static constexpr float kEpsilon = 0.001f;
    static constexpr float kRotationEpsilon = 0.01f;
    static constexpr float kParallelThreshold = 0.999f;
    static constexpr float kVelocityThreshold = 0.1f;
    static constexpr float kShoulderTargetThreshold = 0.1f;
    static constexpr float kShoulderDiffThreshold = 0.01f;
    static constexpr float kHeightDiffThreshold = 0.01f;

    // ベクトル定数
    static constexpr float kVectorZero = 0.0f;
    static constexpr float kUpVectorY = 1.0f;
    static constexpr float kRightVectorX = 1.0f;

    // 初期値
    static constexpr float kInitialYaw = 0.0f;
    static constexpr float kTimerReset = 0.0f;

    // 正規化・クランプ値
    static constexpr float kNormalizedValue = 1.0f;
    static constexpr float kMaxBlendValue = 1.0f;
    static constexpr float kMaxFollowRate = 1.0f;
    static constexpr float kMinClamp = -1.0f;
    static constexpr float kMaxClamp = 1.0f;
    static constexpr float kEasingMaxValue = 1.0f;

    // Rush関連の倍率
    static constexpr float kHighDistSpeedMultiplier = 3.0f;
    static constexpr float kMidDistSpeedMultiplier = 2.0f;
    static constexpr float kRotationSpeedMultiplier = 0.5f;
    static constexpr float kRushDirectionBlendRatio = 0.3f;

    float manualYawSpeed_ = 0.04f;               // 手動回転速度
    float rushEnemyBehindOffset_ = 3.0f;         // 敵の背後にとる距離
    float rushHighDistThreshold_ = 35.0f;        // Rushカメラが高速追従する距離(遠)
    float rushMidDistThreshold_ = 25.0f;         // Rushカメラが中速追従する距離(中)
    float rushPosArrivalThreshold_ = 0.5f;       // Rush復帰時の完了判定距離
    float rushRotationArrivalThreshold_ = 0.01f; // Rush復帰時の完了判定回転差

    // ビュープロジェクション
    ViewProjection viewProjection_;

    WorldTransform worldTransform_;

    // 追従対象
    Player *target_ = nullptr;

    Quaternion rushCameraRotation_; // Rush中に固定されていたカメラ回転

    Vector3 cameraOffset_ = {0.0f, 5.0f, -25.0f};        // ベースのカメラオフセット
    Vector3 shoulderOffsetTarget_ = {0.0f, 0.0f, 0.0f};  // ターゲット肩オフセット
    Vector3 shoulderOffsetCurrent_ = {0.0f, 0.0f, 0.0f}; // 現在の補間値
    Vector3 shoulderOffsetStart_ = {0.0f, 0.0f, 0.0f};   // リセット開始時の値
    Vector3 rushCameraPosition_;                         // Rush中に固定されていたカメラ位置
    Vector3 rushCameraOffset_ = {0.0f, 8.0f, -20.0f};

    float yaw_{};
    float shoulderMaxOffset_ = 12.5f;        // 肩のズレ最大距離(左右)
    float shoulderLerpSpeed_ = 10.0f;        // 補間速度(大きいほど速く追従)
    float rushCameraResumeDistance_ = 50.0f; // この距離以下になったらカメラ追従を再開
    float rushResumeBlendSpeed_ = 8.0f;      // Rush復帰時の補間速度(通常より高速)
    float rushCameraFollowRate_ = 0.3f;      // Rush中の追従率(0.0-1.0)
    bool isResumeFromRush_ = false;          // Rush状態からの復帰中かどうか
    bool isRushCameraActive_ = false;

    // ロックオン状態管理
    bool wasLockedOn_ = false; // 前フレームのロックオン状態

    // 肩オフセットリセット用
    bool isResettingShoulderOffset_ = false; // 肩オフセットリセット中かどうか
    float shoulderResetTimer_ = 0.0f;        // リセット用タイマー
    float shoulderResetDuration_ = 0.5f;     // リセットにかける時間(秒)

    // ロックオン時の高さオフセット用
    float lockOnHeightOffsetCurrent_ = 5.0f; // 現在の高さオフセット
    float lockOnHeightOffsetTarget_ = 5.0f;  // 目標の高さオフセット
    float lockOnGroundedHeight_ = 5.0f;      // 地上時の高さオフセット
    float lockOnAirborneHeight_ = 0.0f;      // 空中時の高さオフセット
    float lockOnHeightLerpSpeed_ = 5.0f;     // 高さオフセットの補間速度

    // イージングタイマー
    float shoulderLerpTimer_ = 0.0f;      // 肩オフセット補間用タイマー
    float shoulderLerpStartValue_ = 0.0f; // 肩オフセット補間開始時の値
    float rushBlendTimer_ = 0.0f;         // Rush中の位置補間用タイマー
    float rushRotationTimer_ = 0.0f;      // Rush中の回転補間用タイマー
    float rushResumeTimer_ = 0.0f;        // Rush復帰補間用タイマー

    // イージングタイプ設定
    EasingType shoulderEasingType_ = EasingType::OutQuad;       // 肩オフセットのイージング
    EasingType shoulderResetEasingType_ = EasingType::OutCubic; // 肩オフセットリセットのイージング
    EasingType rushCameraEasingType_ = EasingType::OutQuad;     // Rushカメラのイージング
    EasingType rushResumeEasingType_ = EasingType::OutCubic;    // Rush復帰のイージング
};
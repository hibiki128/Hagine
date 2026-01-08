#pragma once
#include "../Base/PlayerBaseState.h"

/// <summary>
/// プレイヤーの飛行移動状態を管理するクラス
/// 空中での移動や状態遷移を処理する
/// </summary>
class PlayerStateFlyMove : public PlayerBaseState {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    PlayerStateFlyMove() = default;

    /// <summary>
    /// 状態開始時の処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void Enter(Player &player) override;

    /// <summary>
    /// 状態更新処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void Update(Player &player) override;

    /// <summary>
    /// 状態終了時の処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void Exit(Player &player) override;

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 空中での移動処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void AirMove(Player &player);

    /// <summary>
    /// 状態遷移判定と処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void ChangeState(Player &player);

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    static constexpr float kFlyAcceleration = 30.0f;       // 飛行加速度
    static constexpr float kFlyMaxSpeed = 15.0f;           // 飛行最大速度
    static constexpr float kVelocityDampingFactor = 0.70f; // 速度減衰率
    static constexpr float kVelocityStopThreshold = 0.05f; // 速度停止閾値
    static constexpr float kFallThresholdTime = 0.3f;      // 落下入力閾値時間
    static constexpr float kVelocityZero = 0.0f;           // 速度ゼロ
    static constexpr float kAccelerationZero = 0.0f;       // 加速度ゼロ
    static constexpr float kGroundLevel = 0.0f;            // 地面レベル
    static constexpr float kInitialTime = 0.0f;            // 初期時間
    static constexpr int kInitialCount = 0;                // 初期カウント
    static constexpr int kFallInputThreshold = 1;          // 落下入力閾値

    float spaceHeldTime_ = 0.0f; // スペースキー保持時間
    bool isBoosting_ = false;    // ブースト中フラグ
    float fallInputTime_ = 0.0f; // 落下入力の保持時間
    int fallInputCount_ = 0;     // 落下入力の回数
};
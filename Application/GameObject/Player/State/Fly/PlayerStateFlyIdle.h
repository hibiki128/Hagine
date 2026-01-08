#pragma once
#include "../Base/PlayerBaseState.h"

/// <summary>
/// プレイヤーの飛行アイドル状態を管理するクラス
/// 空中での待機や移動、状態遷移を処理する
/// </summary>
class PlayerStateFlyIdle : public PlayerBaseState {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

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

    static constexpr float kDampingFactor = 0.75f;         // 減衰率
    static constexpr float kVelocityStopThreshold = 0.01f; // 速度停止閾値
    static constexpr float kVelocityZero = 0.0f;           // 速度ゼロ
    static constexpr float kAccelerationZero = 0.0f;       // 加速度ゼロ
    static constexpr float kMoveSpeedZero = 0.0f;          // 移動速度ゼロ
    static constexpr float kGroundLevel = 0.0f;            // 地面レベル
    static constexpr float kInitialTime = 0.0f;            // 初期時間
    static constexpr int kInitialCount = 0;                // 初期カウント

    float spaceHeldTime_ = 0.0f;         // スペースキー保持時間
    bool isBoosting_ = false;            // ブースト中フラグ
    float fallInputTime_ = 0.0f;         // 落下入力の保持時間
    int fallInputCount_ = 0;             // 落下入力の回数
    float lControlInputTime_ = 0.0f;     // L操作入力の保持時間
    int lControlInputCount_ = 0;         // L操作入力の回数
    const float INPUT_RESET_TIME = 5.0f; // 入力リセット時間
};
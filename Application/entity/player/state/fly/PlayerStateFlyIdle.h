#pragma once
#include "../base/PlayerBaseState.h"

/// <summary>
/// プレイヤーの飛行アイドル状態を管理するクラス
/// 空中での待機や移動、状態遷移を処理する
/// Rush 遷移の入力検出もこのステートが担当する
/// </summary>
class PlayerStateFlyIdle : public PlayerBaseState
{
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
    /// 空中での速度減衰処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void AirMove(Player &player);

    /// <summary>
    /// 状態遷移判定と処理
    /// Rush 遷移の判定も含む
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void ChangeStateLogic(Player &player);

    /// <summary>
    /// Rush 状態への遷移を判定し、条件を満たせば遷移する
    /// キーボードは LCtrl 2 回押し、ゲームパッドはダッシュ中 A ボタンで発動
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void TryChangeToRush(Player &player);

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    static constexpr float kDampingFactor = 0.75f;         // 減衰率
    static constexpr float kVelocityStopThreshold = 0.01f; // 速度停止閾値
    static constexpr float kVelocityZero = 0.0f;           // 速度ゼロ
    static constexpr float kAccelerationZero = 0.0f;       // 加速度ゼロ
    static constexpr float kMoveSpeedZero = 0.0f;          // 移動速度ゼロ

    // Rush 遷移パラメータ
    static constexpr float kRushEnergyCost = 30.0f;     // Rush 遷移のエネルギーコスト
    static constexpr float kInputResetTime = 0.3f;      // キーボード入力リセット時間
    static constexpr float kDashRushMinDuration = 0.1f; // ゲームパッドのダッシュから Rush に移行するための最低ダッシュ時間

    float lControlInputTime_ = 0.0f; // LCtrl 入力の保持時間（キーボード用）
    int lControlInputCount_ = 0;     // LCtrl の入力回数（キーボード用）
};

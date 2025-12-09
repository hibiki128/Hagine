#pragma once
#include "../Base/PlayerBaseState.h"

/// <summary>
/// プレイヤーのエネルギーチャージ状態を管理するクラス
/// エネルギーを回復する
/// </summary>
class PlayerEnergyChageState : public PlayerBaseState {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    PlayerEnergyChageState() = default;

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

    float GetChageRate() { return chargeRate_; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    float chargeRate_ = 1.0f; // エネルギーチャージ速度
};

#pragma once
#include "../Base/PlayerBaseState.h"

/// <summary>
/// プレイヤーの移動状態を管理するクラス
/// 地面上での移動を処理する
/// </summary>
class PlayerStateMove : public PlayerBaseState {
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
    /// private varians
    /// ===================================================

    static constexpr float kMinInitialSpeed    = 2.0f;   // 最小初期速度
    static constexpr float kGroundPullVelocity = -0.1f;  // 地面への引き付け速度
};

#pragma once
#include "../Base/PlayerBaseState.h"

/// <summary>
/// プレイヤーの待機状態を管理するクラス
/// 地面上での待機や入力待ちを処理する
/// </summary>
class PlayerStateIdle : public PlayerBaseState {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    PlayerStateIdle() = default;

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
};
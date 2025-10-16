#pragma once
#include "../Base/PlayerBaseState.h"

/// <summary>
/// プレイヤーの空中状態を管理するクラス
/// ジャンプ中や落下中の状態を処理する
/// </summary>
class PlayerStateAir : public PlayerBaseState {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    PlayerStateAir() = default;

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

    float elapsedTime_ = 0.0f; // 経過時間
};
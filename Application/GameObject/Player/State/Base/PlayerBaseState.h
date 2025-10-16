#pragma once
class Player;

/// <summary>
/// プレイヤー状態の基底クラス
/// 各プレイヤー状態はこのクラスを継承して実装される
/// </summary>
class PlayerBaseState {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~PlayerBaseState() = default;

    /// <summary>
    /// 状態開始時の処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    virtual void Enter(Player &player) = 0;

    /// <summary>
    /// 状態更新処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    virtual void Update(Player &player) = 0;

    /// <summary>
    /// 状態終了時の処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    virtual void Exit(Player &player) = 0;
};
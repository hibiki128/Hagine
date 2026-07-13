#pragma once
#include <camera/projection/ViewProjection.h>
class Player;

/// <summary>
/// プレイヤー状態の基底クラス
/// 各プレイヤー状態はこのクラスを継承して実装される
/// </summary>
class PlayerBaseState
{
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

    /// <summary>
    /// パーティクル描画処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    /// <param name="viewProjection">ビュープロジェクション</param>
    virtual void DrawParticle(Player &player, const Hagine::ViewProjection &viewProjection) {}

  protected:
    /// ===================================================
    /// 入力ヘルパー（派生クラスで共通利用）
    /// ===================================================

    /// <summary>
    /// 移動入力があるか（WASD / 左スティック）
    /// </summary>
    static bool HasMovementInput(Player &player);

    /// <summary>
    /// ジャンプ・飛行開始の瞬間入力か（SPACE / RB トリガー）
    /// </summary>
    static bool IsJumpOrFlyTriggered(Player &player);

    /// <summary>
    /// 上昇入力が押されているか（SPACE 押下 / RB 押下）
    /// </summary>
    static bool IsAscendInput(Player &player);

    /// <summary>
    /// 下降入力が押されているか（LSHIFT 押下 / RT 押下）
    /// </summary>
    static bool IsDescendInput(Player &player);

    /// <summary>
    /// 下降入力の瞬間トリガーか（LSHIFT トリガー / RT トリガー）
    /// </summary>
    static bool IsDescendTriggered(Player &player);

    /// <summary>
    /// 飛行中の任意入力があるか（移動・上昇・下降すべて含む）
    /// </summary>
    static bool HasFlyAnyInput(Player &player);
};

#pragma once
#include "../Base/PlayerBaseState.h"
class PlayerStateGuard : public PlayerBaseState {
  public:
    /// ===================================================
    /// public method
    /// ===================================================
    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    PlayerStateGuard() = default;
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

    /// <summary>
    /// パーティクル描画処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(Player &player, const ViewProjection &viewProjection) override;
};

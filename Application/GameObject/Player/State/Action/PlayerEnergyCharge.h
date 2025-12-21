#pragma once
#include "../Base/PlayerBaseState.h"
#include <string>
#include <Camera/ViewProjection/ViewProjection.h>
#include <Particle/CSParticle/ParticleCSEmitter.h>

/// <summary>
/// プレイヤーのエネルギーチャージ状態を管理するクラス
/// エネルギーを回復する
/// </summary>
class PlayerEnergyCharge : public PlayerBaseState {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    PlayerEnergyCharge() = default;

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

    float GetChageRate() { return chargeRate_; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    float chargeRate_ = 15.0f; // エネルギーチャージ速度
    float beforeChargeRate_ = 0.0f;
    std::string beforeState_ = "";
    std::unique_ptr<ParticleCSEmitter> chargeAuraEmitter_; // チャージオーラパーティクル
};

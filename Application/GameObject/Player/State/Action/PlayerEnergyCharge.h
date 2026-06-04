#pragma once
#include "../Base/PlayerBaseState.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <Particle/CSParticle/ParticleCSEmitter.h>
#include <string>

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
    void DrawParticle(Player &player, const Hagine::ViewProjection &viewProjection) override;

    float GetChageRate() { return chargeRate_; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    static constexpr float kChargeRate = 15.0f;       // エネルギーチャージ速度
    static constexpr float kInitialChargeRate = 0.0f; // 初期チャージレート
    static constexpr float kVelocityZero = 0.0f;      // 速度ゼロ
    static constexpr float kParticleYOffset = -1.5f;  // パーティクルY座標オフセット

    float chargeRate_ = kChargeRate;
    float beforeChargeRate_ = kInitialChargeRate;
    std::string beforeState_ = "";
    std::unique_ptr<Hagine::ParticleCSEmitter> chargeAuraEmitter_; // チャージオーラパーティクル
};
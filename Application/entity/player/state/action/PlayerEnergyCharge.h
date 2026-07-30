#pragma once
#include "../base/PlayerBaseState.h"
#include <camera/projection/ViewProjection.h>
#include <particle/gpu/ParticleCSEmitter.h>
#include <string>

/// <summary>
/// プレイヤーのエネルギーチャージ状態を管理するクラス
/// エネルギーを回復する
/// </summary>
class PlayerEnergyCharge : public PlayerBaseState
{
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

    float GetChargeRate() { return chargeRate_; }

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    static constexpr float kChargeRate = 15.0f;       // エネルギーチャージ速度
    static constexpr float kInitialChargeRate = 0.0f; // 初期チャージレート
    static constexpr float kVelocityZero = 0.0f;      // 速度ゼロ

    float chargeRate_ = kChargeRate;
    float beforeChargeRate_ = kInitialChargeRate;
    std::string beforeState_ = "";
    // チャージオーラ。実体は ParticleCSSpawner が所有する（借用ポインタ）。
    // チャージ開始ごとに Spawn し、終了時に DespawnWhenFinished で自然消滅させる。
    Hagine::ParticleCSEmitter *pChargeAuraEmitter_ = nullptr;
};
#pragma once
#include <type/Vector3.h>

class Enemy;

/// <summary>
/// 敵のステータスパーツクラス
/// HP・エネルギー・被ダメージ・ガード・ノックバック・被弾リアクションを担当する
/// </summary>
class EnemyStatus
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="owner">所有者の敵</param>
    void Init(Enemy *owner);

    /// <summary>
    /// ダメージを受ける処理（ガード軽減・ノックバック適用を含む）
    /// </summary>
    void DamageUpdate();

    /// <summary>
    /// ダメージリアクション（高速点滅）の更新処理
    /// </summary>
    void UpdateDamageReact();

    /// <summary>
    /// エネルギー消費処理
    /// </summary>
    /// <param name="amount">消費量</param>
    /// <returns>bool: 消費できたら true</returns>
    bool ConsumeEnergy(float amount);

    /// <summary>
    /// エネルギー回復処理
    /// </summary>
    void RecoverEnergy();

    /// <summary>
    /// 外部からノックバックを与える
    /// </summary>
    /// <param name="direction">ノックバック方向（正規化済みでなくてもよい）</param>
    /// <param name="power">ノックバック強度</param>
    void SetKnockback(const Hagine::Vector3 &direction, float power);

    /// <summary>
    /// ダメージリアクションを開始する
    /// </summary>
    void StartDamageReact();

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

    /// ===================================================
    /// Getter
    /// ===================================================
    float GetHP() const { return HP_; }
    float GetMaxHP() const { return maxHP_; }
    float &GetEnergy() { return energy_; }
    float GetMaxEnergy() const { return maxEnergy_; }
    float GetEnergyRecoveryRate() const { return energyRecoveryRate_; }
    bool IsGuarding() const { return isGuarding_; }
    /// <summary>ガード可能か（被弾で消費するエネルギーを支払えるか）</summary>
    bool CanGuard() const { return energy_ >= kGuardEnergyCost; }
    bool IsDamageReact() const { return isDamageReact_; }

    /// ===================================================
    /// ImGui 表示用のパラメータ参照
    /// ===================================================
    float &GetHPRef() { return HP_; }
    float &GetMaxHPRef() { return maxHP_; }
    float &GetMaxEnergyRef() { return maxEnergy_; }
    float &GetDamageReactDurationRef() { return damageReactDuration_; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetHP(float hp) { HP_ = hp; }
    void SetMaxHP(float maxHP) { maxHP_ = maxHP; }
    void SetDamage(float damage) { damage_ = damage; }
    void SetGuarding(bool guarding) { isGuarding_ = guarding; }
    void SetEnergy(float energy);
    void SetEnergyRecoveryRate(float rate) { energyRecoveryRate_ = rate; }

    /// <summary>HP・エネルギー・被弾状態を初期化する（復活用）</summary>
    void ResetForRevive();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // ダメージ・HP関連定数
    static constexpr float kNoDamage = 0.0f;
    static constexpr float kMinHP = 0.0f;
    static constexpr float kTimerReset = 0.0f;
    static constexpr float kGuardDamageMultiplier = 0.15f;
    static constexpr float kGuardEnergyCost = 10.0f; // ガード中の被弾で消費するエネルギー（プレイヤーと同様）

    // 点滅関連定数
    static constexpr float kDamageBlinkInterval = 0.03f;
    static constexpr int kBlinkModulo = 2;
    static constexpr int kEvenBlink = 0;
    static constexpr float kAlphaTransparent = 0.0f;
    static constexpr float kAlphaOpaque = 1.0f;

    Enemy *owner_ = nullptr; ///< 所有者の敵

    float HP_ = 100.0f;    ///< HP
    float maxHP_ = 100.0f; ///< 最大HP
    float damage_ = 0.0f;  ///< 受けるダメージ量

    float energy_ = 100.0f;            ///< エネルギー
    float maxEnergy_ = 100.0f;         ///< 最大エネルギー
    float energyRecoveryRate_ = 0.01f; ///< エネルギー回復速度
    float energyRecoveryDelay_ = 1.0f; ///< エネルギー回復遅延
    float timeSinceLastShot_ = 0.0f;   ///< 最終射撃からの経過時間

    bool isGuarding_ = false; ///< ガード中フラグ

    // ノックバック関連
    bool hasKnockback_ = false;                    ///< ノックバック中フラグ
    Hagine::Vector3 pendingKnockback_ = {0, 0, 0}; ///< ノックバック速度

    bool isDamageReact_ = false;       ///< ダメージ反応中フラグ
    float damageReactTimer_ = 0.0f;    ///< ダメージ反応タイマー
    float damageReactDuration_ = 0.5f; ///< ダメージ反応時間
};

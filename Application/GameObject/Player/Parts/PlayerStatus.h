#pragma once
#include <type/Vector3.h>

class Player;
namespace Hagine {
class DataHandler;
}

/// <summary>
/// プレイヤーのステータスパーツクラス
/// HP・エネルギー・被ダメージ・無敵時間・ガード・ノックバックを担当する
/// </summary>
class PlayerStatus
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="owner">所有者のプレイヤー</param>
    void Init(Player *owner);

    /// <summary>
    /// ダメージを受ける処理
    /// </summary>
    void DamageUpdate();

    /// <summary>
    /// 無敵時間の更新処理
    /// </summary>
    void InvincibleUpdate();

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
    /// ステータス関連パラメータを保存する
    /// </summary>
    /// <param name="data">保存先のデータハンドラ</param>
    void Save(Hagine::DataHandler *data);

    /// <summary>
    /// ステータス関連パラメータを読み込む
    /// </summary>
    /// <param name="data">読み込み元のデータハンドラ</param>
    void Load(Hagine::DataHandler *data);

    /// <summary>
    /// ステータス関連のImGui表示
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

    /// ===================================================
    /// Getter
    /// ===================================================
    float GetHP() const { return HP_; }
    float GetMaxHP() const { return maxHP_; }
    float GetEnergy() const { return energy_; }
    float &GetEnergy() { return energy_; }
    float GetMaxEnergy() const { return maxEnergy_; }
    float GetChargeRate() const { return energyRecoveryRate_; }
    bool IsGuarding() const { return isGuarding_; }
    bool CanGuard() const { return energy_ >= guardEnergyCost_; }
    bool IsInvincible() const { return isInvincible_; }
    bool IsDamageReact() const { return isDamageReact_; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetHP(float hp) { HP_ = hp; }
    void SetDamage(float damage) { damage_ = damage; }
    void SetGuarding(bool flag) { isGuarding_ = flag; }
    void SetEnergyRecoveryRate(float rate) { energyRecoveryRate_ = rate; }
    void ResetEnergyForTutorial()
    {
        energy_ = 0.0f;
        timeSinceLastShot_ = 0.0f; // 回復遅延タイマーもリセット
    }
    void ResetShotTimer() { timeSinceLastShot_ = kTimerReset; }
    void StopDamageReact();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // ダメージ・HP関連定数
    static constexpr float kMinHP = 0.0f;
    static constexpr float kTimerReset = 0.0f;
    static constexpr float kNoDamage = 0.0f;

    // 点滅関連定数
    static constexpr float kPlayerBlinkInterval = 0.05f;
    static constexpr int kBlinkModulo = 2;
    static constexpr int kEvenBlink = 0;
    static constexpr float kPlayerAlphaTransparent = 0.3f;
    static constexpr float kAlphaOpaque = 1.0f;

    Player *owner_ = nullptr; ///< 所有者のプレイヤー

    float HP_ = 100.0f;                ///< 現在HP
    float maxHP_ = 100.0f;             ///< 最大HP
    float energy_ = 100.0f;            ///< 現在のエネルギー
    float maxEnergy_ = 100.0f;         ///< 最大エネルギー
    float energyRecoveryRate_ = 0.01f; ///< エネルギー回復速度(秒速)
    float energyRecoveryDelay_ = 1.0f; ///< 回復開始までの遅延時間
    float timeSinceLastShot_ = 0.0f;   ///< 最後に撃ってからの経過時間

    bool isInvincible_ = false;        ///< 無敵状態フラグ
    float invincibleTime_ = 0.0f;      ///< 無敵時間の経過時間
    float invincibleDuration_ = 0.25f; ///< 無敵時間の長さ(秒)
    float damage_ = 0.0f;              ///< 次のDamageUpdateで処理するダメージ量

    bool isGuarding_ = false;             ///< ガード中フラグ
    float guardDamageMultiplier_ = 0.20f; ///< ガード中の被ダメージ倍率（軽減率80%）ImGuiで調整可
    float guardEnergyCost_ = 10.0f;       ///< ガード中に被弾した際のエネルギー消費量

    Hagine::Vector3 knockbackVelocity_ = {0.0f, 0.0f, 0.0f}; ///< 適用待ちノックバック
    bool hasKnockback_ = false;                              ///< ノックバック適用待ちフラグ

    bool isDamageReact_ = false;       ///< リアクション中かどうか（被弾点滅）
    float damageReactTimer_ = 0.0f;    ///< 経過時間
    float damageReactDuration_ = 0.5f; ///< リアクション時間
};

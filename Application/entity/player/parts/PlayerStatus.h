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
    /// ひるみ（ヒットスタン）時間の更新処理。毎フレーム呼ぶ
    /// </summary>
    void UpdateHitStun();

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
    /// エネルギーを削る処理。残量が足りない場合もゼロまで削り取る
    /// </summary>
    /// <param name="amount">削る量</param>
    void DrainEnergy(float amount);

    /// <summary>
    /// エネルギー回復処理
    /// </summary>
    void RecoverEnergy();

    /// <summary>
    /// ガード中なら弾を弾き返せるかを判定する（成立時はガード分のエネルギーを消費する）
    /// </summary>
    /// <returns>bool: 弾き返せたら true</returns>
    bool ConsumeGuardDeflect();

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

    /// <summary>ひるみ（ヒットスタン）中かどうか。true の間はプレイヤーは行動できない</summary>
    bool IsHitStun() const { return hitStunTimer_ > 0.0f; }

    /// <summary>ひるみアニメの番号（1〜3）。被弾ごとにランダムで選ばれる</summary>
    int GetFlinchAnimIndex() const { return flinchAnimIndex_; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetHP(float hp) { HP_ = hp; }

    /// <summary>
    /// 次のDamageUpdateで処理するダメージを設定する
    /// </summary>
    /// <param name="damage">ダメージ量</param>
    /// <param name="isShot">射撃（弾）によるダメージなら true。ひるみが近接より短くなる</param>
    /// <param name="isSkill">必殺技によるダメージなら true。ガード時のエネルギー消費が大きくなる</param>
    void SetDamage(float damage, bool isShot = false, bool isSkill = false)
    {
        damage_ = damage;
        damageIsShot_ = isShot;
        damageIsSkill_ = isSkill;
    }
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

    Player *pOwner_ = nullptr; ///< 所有者のプレイヤー

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
    float guardEnergyCost_ = 3.0f;        ///< ガード中に被弾した際のエネルギー消費量
    float guardSkillEnergyCost_ = 15.0f;  ///< ガード中に必殺技を受けた際のエネルギー消費量（通常より大きく削る）

    Hagine::Vector3 knockbackVelocity_ = {0.0f, 0.0f, 0.0f}; ///< 適用待ちノックバック
    bool hasKnockback_ = false;                              ///< ノックバック適用待ちフラグ

    bool isDamageReact_ = false;       ///< リアクション中かどうか（被弾点滅）
    float damageReactTimer_ = 0.0f;    ///< 経過時間
    float damageReactDuration_ = 0.5f; ///< リアクション時間

    // ─── ひるみ（ヒットスタン）被弾ごとに再充填され、ガード時は発生しない ───
    float hitStunTimer_ = 0.0f;     ///< ひるみ残り時間（>0 で行動不能）
    float hitStunDuration_ = 0.5f;  ///< ひるみ継続時間（秒）コンボ間隔をまたぐ長さ。GameParamで調整可
    int flinchAnimIndex_ = 1;       ///< ひるみアニメ番号（1〜3・被弾ごとにランダム）

    bool damageIsShot_ = false;     ///< 処理待ちのダメージが射撃由来か（ひるみを短くする）
    bool damageIsSkill_ = false;    ///< 処理待ちのダメージが必殺技由来か（ガード時の消費量を切り替える）
    float shotFlinchScale_ = 0.5f;  ///< 射撃被弾時のひるみ時間倍率（近接に対する比率）
};

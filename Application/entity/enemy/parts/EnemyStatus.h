#pragma once
#include <type/Vector3.h>

class Enemy;

/// <summary>
/// 被弾リアクションの状態。
/// Flinch=ひるみ（その場硬直）、Blow=大きく吹き飛ばされ中（着地でBlowAfterへ）
/// </summary>
enum class EnemyReactState
{
    None,   // リアクションなし（通常行動可）
    Flinch, // ひるみ（行動不能・Hittingアニメ）
    Blow,   // 吹き飛ばし（行動不能・BlowBack→着地でBlowAfter）
};

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
    /// ノックバック速度を直接指定する（上方成分の固定加算を行わない）。
    /// 下方向へ叩きつける等、任意方向の吹き飛ばしに使う
    /// </summary>
    /// <param name="velocity">適用するノックバック速度</param>
    void SetKnockbackDirect(const Hagine::Vector3 &velocity);

    /// <summary>
    /// ダメージリアクションを開始する
    /// </summary>
    void StartDamageReact();

    /// <summary>
    /// 次に受けるダメージを「吹き飛ばし（Blow）」リアクションとして扱うよう予約する。
    /// 瞬間移動コンボの吹き飛ばし段のヒット時にコライダーから呼ぶ
    /// </summary>
    void RequestBlowReaction() { blowPending_ = true; }

    /// <summary>
    /// 被弾リアクション（ひるみ・吹き飛ばし）の状態を進める（毎フレーム呼ぶ）
    /// </summary>
    void UpdateReaction();

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

    /// <summary>被弾リアクション（ひるみ or 吹き飛ばし）中か。true の間はAI(BT)を停止する</summary>
    bool IsReacting() const { return reactState_ != EnemyReactState::None; }
    /// <summary>吹き飛ばし（Blow）リアクション中か</summary>
    bool IsBlow() const { return reactState_ == EnemyReactState::Blow; }
    /// <summary>ひるみ（Flinch）リアクション中か</summary>
    bool IsFlinch() const { return reactState_ == EnemyReactState::Flinch; }
    /// <summary>吹き飛ばし後、地面に着地済みか（BlowAfter再生の判定用）</summary>
    bool IsBlowLanded() const { return blowLanded_; }
    /// <summary>ひるみアニメの番号（1〜3）</summary>
    int GetFlinchAnimIndex() const { return flinchAnimIndex_; }

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
    static constexpr float kGuardEnergyCost = 4.0f; // ガード中の被弾で消費するエネルギー（プレイヤーと同様）

    // 点滅関連定数
    static constexpr float kDamageBlinkInterval = 0.03f;
    static constexpr int kBlinkModulo = 2;
    static constexpr int kEvenBlink = 0;
    static constexpr float kAlphaTransparent = 0.0f;
    static constexpr float kAlphaOpaque = 1.0f;

    Enemy *pOwner_ = nullptr; ///< 所有者の敵

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

    // ─── 被弾リアクション（ひるみ・吹き飛ばし）───
    // リアクション中はBTを停止し、Blowはノックバック速度で滑走、Flinchはその場で硬直する
    EnemyReactState reactState_ = EnemyReactState::None; ///< 現在のリアクション状態
    float reactTimer_ = 0.0f;                            ///< ひるみの残り時間
    float flinchDuration_ = 0.5f;                        ///< ひるみ継続時間（秒・コンボ間隔をまたぐ長さ）
    int flinchAnimIndex_ = 1;                            ///< ひるみアニメ番号（1〜3）
    bool blowPending_ = false;                           ///< 次のダメージをBlow扱いにする予約
    bool blowLanded_ = false;                            ///< 吹き飛ばし後に着地したか
    float blowAfterTimer_ = 0.0f;                        ///< 着地後（BlowAfter）の残り硬直時間
    float blowAfterDuration_ = 0.6f;                     ///< 着地後の硬直時間（秒）
    float blowTimer_ = 0.0f;                             ///< 吹き飛ばし開始からの経過時間（空中滞留時の強制復帰用）
    float blowMaxDuration_ = 1.2f;                       ///< 着地しないまま吹き飛ばし続ける最大時間（秒）。これを超えたら強制復帰
};

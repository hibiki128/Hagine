#pragma once
#include <type/Vector3.h>

class Enemy;

/// <summary>
/// 被弾リアクションの状態
/// </summary>
enum class EnemyReactState
{
    None,      // リアクションなし（通常行動可）
    Flinch,    // ひるみ（行動不能・Hittingアニメ）
    Blow,      // 吹き飛ばし（行動不能・BlowBack→着地でBlowAfter）
    SkillBlow, // 必殺技被弾スタン（行動不能・横速度を保ったまま減速しつつ落下）
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
    /// <param name="pOwner">所有者の敵</param>
    void Init(Enemy *pOwner);

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
    /// <param name="grantFlinchImmunity">復帰直後にひるみ無効時間を与えるなら true（叩きつけ段用）</param>
    void RequestBlowReaction(bool grantFlinchImmunity = false)
    {
        blowPending_ = true;
        blowGrantsFlinchImmunity_ = grantFlinchImmunity;
    }

    /// <summary>
    /// 次に受けるダメージを「必殺技被弾スタン（SkillBlow）」として扱うよう予約する
    /// </summary>
    /// <param name="direction">吹き飛ばす水平方向（正規化不要・ゼロなら現在の向きの後方）</param>
    void RequestSkillBlowReaction(const Hagine::Vector3 &direction);

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
    float GetHP() const { return hp_; }
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
    /// <summary>吹き飛ばし中か（必殺技スタンも含む。重力適用・BlowBackアニメの判定に使う）</summary>
    bool IsBlow() const
    {
        return reactState_ == EnemyReactState::Blow || reactState_ == EnemyReactState::SkillBlow;
    }
    /// <summary>必殺技被弾スタン（SkillBlow）中か</summary>
    bool IsSkillBlow() const { return reactState_ == EnemyReactState::SkillBlow; }
    /// <summary>ひるみ（Flinch）リアクション中か</summary>
    bool IsFlinch() const { return reactState_ == EnemyReactState::Flinch; }
    /// <summary>吹き飛ばし後、地面に着地済みか（BlowAfter再生の判定用）</summary>
    bool IsBlowLanded() const { return blowLanded_; }
    /// <summary>ひるみアニメの番号（1〜3）</summary>
    int GetFlinchAnimIndex() const { return flinchAnimIndex_; }

    /// <summary>ひるみ無効時間中か（吹き飛ばし・必殺技被弾から復帰した直後）</summary>
    bool IsFlinchImmune() const { return flinchImmuneTimer_ > 0.0f; }

    /// ===================================================
    /// ImGui 表示用のパラメータ参照
    /// ===================================================
    float &GetHPRef() { return hp_; }
    float &GetMaxHPRef() { return maxHP_; }
    float &GetMaxEnergyRef() { return maxEnergy_; }
    float &GetDamageReactDurationRef() { return damageReactDuration_; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetHP(float hp) { hp_ = hp; }
    void SetMaxHP(float maxHP) { maxHP_ = maxHP; }

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
    void SetGuarding(bool guarding) { isGuarding_ = guarding; }
    void SetEnergy(float energy);
    void SetEnergyRecoveryRate(float rate) { energyRecoveryRate_ = rate; }

    /// <summary>HP・エネルギー・被弾状態を初期化する（復活用）</summary>
    void ResetForRevive();

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 必殺技被弾スタンを開始する（吹き飛ばし速度の設定と落下状態への移行）
    /// </summary>
    void StartSkillBlow();

    /// <summary>
    /// 必殺技被弾スタンの更新（横速度の減速・着地判定）
    /// </summary>
    /// <param name="deltaTime">経過時間（秒）</param>
    void UpdateSkillBlow(float deltaTime);

    /// <summary>
    /// 大きな吹き飛ばしから通常状態へ復帰させる（残速度を消す）
    /// </summary>
    /// <param name="grantFlinchImmunity">ひるみ無効時間を与えるなら true</param>
    void RecoverFromBlow(bool grantFlinchImmunity);

    /// ===================================================
    /// private variables
    /// ===================================================

    // ダメージ・HP関連定数
    static constexpr float kNoDamage = 0.0f;
    static constexpr float kMinHP = 0.0f;
    static constexpr float kGuardDamageMultiplier = 0.15f;
    static constexpr float kGuardEnergyCost = 3.0f;       // ガード中の被弾で消費するエネルギー（プレイヤーと同様）
    static constexpr float kGuardSkillEnergyCost = 15.0f; // ガード中に必殺技を受けた際の消費エネルギー（通常より大きく削る）

    // 点滅関連定数
    static constexpr float kDamageBlinkInterval = 0.03f;
    static constexpr int kBlinkModulo = 2;
    static constexpr int kEvenBlink = 0;
    static constexpr float kAlphaTransparent = 0.0f;
    static constexpr float kAlphaOpaque = 1.0f;

    Enemy *pOwner_ = nullptr; ///< 所有者の敵

    float hp_ = 100.0f;    ///< HP
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

    // ─── 被弾リアクション（ひるみ・吹き飛ばし）中はBTを停止する ───
    EnemyReactState reactState_ = EnemyReactState::None; ///< 現在のリアクション状態
    float reactTimer_ = 0.0f;                            ///< ひるみの残り時間
    float flinchDuration_ = 0.5f;                        ///< ひるみ継続時間（秒・コンボ間隔をまたぐ長さ）
    bool damageIsShot_ = false;                          ///< 処理待ちのダメージが射撃由来か（ひるみを短くする）
    bool damageIsSkill_ = false;                         ///< 処理待ちのダメージが必殺技由来か（ガード時の消費量を切り替える）
    float shotFlinchScale_ = 0.5f;                       ///< 射撃被弾時のひるみ時間倍率（近接に対する比率）
    int flinchAnimIndex_ = 1;                            ///< ひるみアニメ番号（1〜3）
    bool blowPending_ = false;                           ///< 次のダメージをBlow扱いにする予約
    bool blowGrantsFlinchImmunity_ = false;              ///< 予約中のBlowが復帰時にひるみ無効を与えるか
    bool blowImmunityArmed_ = false;                     ///< 進行中のBlowが復帰時にひるみ無効を与えるか

    // ─── ひるみ無効時間（起き上がり直後に殴られ続けないための猶予。ダメージは通る）───
    float flinchImmuneTimer_ = 0.0f;    ///< ひるみ無効の残り時間（>0 でひるまない）
    float flinchImmuneDuration_ = 2.0f; ///< 吹き飛ばし復帰後のひるみ無効時間（秒）

    // ─── 必殺技被弾スタン（SkillBlow）大きく吹き飛ばされて落下し、その間は行動不能 ───
    bool skillBlowPending_ = false;                     ///< 次のダメージをSkillBlow扱いにする予約
    Hagine::Vector3 skillBlowDirection_ = {0, 0, 0};    ///< 吹き飛ばす水平方向（正規化済み）
    float skillBlowTimer_ = 0.0f;                       ///< スタン開始からの経過時間
    float skillBlowSpeed_ = 35.0f;                      ///< 吹き飛ばしの水平初速
    float skillBlowRiseSpeed_ = 12.0f;                  ///< 吹き飛ばしの上方初速（この後は落下する）
    float skillBlowHorizontalRetain_ = 0.12f;           ///< 1秒あたりに残る横速度の割合（小さいほど早く減速）
    float skillBlowMaxDuration_ = 5.0f;                 ///< 着地しないまま落下し続ける最大時間（秒・安全策）
    float skillBlowDamageMultiplier_ = 0.2f;            ///< スタン中の被ダメージ倍率（0.2＝80%軽減）
    static constexpr float kSkillBlowFallbackGravity = 30.0f; ///< 重力加速度が未設定のときに使う値
    bool blowLanded_ = false;                            ///< 吹き飛ばし後に着地したか
    float blowAfterTimer_ = 0.0f;                        ///< 着地後（BlowAfter）の残り硬直時間
    float blowAfterDuration_ = 0.6f;                     ///< 着地後の硬直時間（秒）
    float blowTimer_ = 0.0f;                             ///< 吹き飛ばし開始からの経過時間（空中滞留時の強制復帰用）
    float blowMaxDuration_ = 1.2f;                       ///< 着地しないまま吹き飛ばし続ける最大時間（秒）。これを超えたら強制復帰
};

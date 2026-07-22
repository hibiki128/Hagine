#pragma once
#include <type/Vector3.h>

class Player;
namespace Hagine {
class DataHandler;
}

/// <summary>
/// 被弾リアクションの状態
/// </summary>
enum class PlayerReactState
{
    None,      // リアクションなし（通常行動可）
    Flinch,    // ひるみ（行動不能・Hittingアニメ）
    Blow,      // 吹き飛ばし（行動不能・BlowBack→着地でBlowAfter）
    SkillBlow, // 必殺技被弾スタン（行動不能・横速度を保ったまま減速しつつ落下）
};

/// <summary>
/// プレイヤーのステータスパーツクラス
/// HP・エネルギー・被ダメージ・無敵時間・ガード・ノックバック・被弾リアクションを担当する
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
    /// 被弾リアクション（ひるみ・吹き飛ばし・必殺技被弾スタン）の状態を進める（毎フレーム呼ぶ）
    /// </summary>
    void UpdateReaction();

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
    /// 次に受けるダメージを「吹き飛ばし（Blow）」リアクションとして扱うよう予約する。
    /// 敵のコンボ最終段などのヒット時にコライダーから呼ぶ
    /// </summary>
    /// <param name="grantFlinchImmunity">復帰直後にひるみ無効時間を与えるなら true</param>
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

    /// <summary>被弾リアクション中かどうか。true の間はプレイヤーは行動できない</summary>
    bool IsReacting() const { return reactState_ != PlayerReactState::None; }

    /// <summary>ひるみ（ヒットスタン）中かどうか</summary>
    bool IsHitStun() const { return reactState_ == PlayerReactState::Flinch; }

    /// <summary>吹き飛ばし中か（必殺技スタンも含む。BlowBackアニメの判定に使う）</summary>
    bool IsBlow() const
    {
        return reactState_ == PlayerReactState::Blow || reactState_ == PlayerReactState::SkillBlow;
    }

    /// <summary>必殺技被弾スタン（SkillBlow）中か</summary>
    bool IsSkillBlow() const { return reactState_ == PlayerReactState::SkillBlow; }

    /// <summary>吹き飛ばし後、地面に着地済みか（BlowAfter再生の判定用）</summary>
    bool IsBlowLanded() const { return blowLanded_; }

    /// <summary>ひるみ無効時間中か（吹き飛ばし・必殺技被弾から復帰した直後）</summary>
    bool IsFlinchImmune() const { return flinchImmuneTimer_ > 0.0f; }

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
    /// 吹き飛ばし中の重力を適用する（ステート更新が止まっている間の落下用）
    /// </summary>
    /// <param name="deltaTime">経過時間（秒）</param>
    void ApplyBlowGravity(float deltaTime);

    /// <summary>
    /// 大きな吹き飛ばしから通常状態へ復帰させる（残速度を消して待機ステートへ戻す）
    /// </summary>
    /// <param name="grantFlinchImmunity">ひるみ無効時間を与えるなら true</param>
    void RecoverFromBlow(bool grantFlinchImmunity);

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

    // ─── 被弾リアクション（ひるみ・吹き飛ばし）中は行動不能になる ───
    PlayerReactState reactState_ = PlayerReactState::None; ///< 現在のリアクション状態
    float hitStunTimer_ = 0.0f;     ///< ひるみ残り時間（>0 で行動不能）
    float hitStunDuration_ = 0.5f;  ///< ひるみ継続時間（秒）コンボ間隔をまたぐ長さ。GameParamで調整可
    int flinchAnimIndex_ = 1;       ///< ひるみアニメ番号（1〜3・被弾ごとにランダム）

    bool damageIsShot_ = false;     ///< 処理待ちのダメージが射撃由来か（ひるみを短くする）
    bool damageIsSkill_ = false;    ///< 処理待ちのダメージが必殺技由来か（ガード時の消費量を切り替える）
    float shotFlinchScale_ = 0.5f;  ///< 射撃被弾時のひるみ時間倍率（近接に対する比率）

    // ─── 吹き飛ばし（Blow）敵のコンボ最終段などで大きく飛ばされ、着地まで行動不能 ───
    bool blowPending_ = false;              ///< 次のダメージをBlow扱いにする予約
    bool blowGrantsFlinchImmunity_ = false; ///< 予約中のBlowが復帰時にひるみ無効を与えるか
    bool blowImmunityArmed_ = false;        ///< 進行中のBlowが復帰時にひるみ無効を与えるか
    bool blowLanded_ = false;               ///< 吹き飛ばし後に着地したか
    float blowTimer_ = 0.0f;                ///< 吹き飛ばし開始からの経過時間（空中滞留時の強制復帰用）
    float blowMaxDuration_ = 1.2f;          ///< 着地しないまま吹き飛ばし続ける最大時間（秒）
    float blowAfterTimer_ = 0.0f;           ///< 着地後（BlowAfter）の残り硬直時間
    float blowAfterDuration_ = 0.6f;        ///< 着地後の硬直時間（秒）
    float blowGravity_ = 30.0f;             ///< 吹き飛ばし中の落下加速度（通常の落下より強くして滞空を短くする）

    // ─── ひるみ無効時間（起き上がり直後に殴られ続けないための猶予。ダメージは通る）───
    float flinchImmuneTimer_ = 0.0f;    ///< ひるみ無効の残り時間（>0 でひるまない）
    float flinchImmuneDuration_ = 2.0f; ///< 吹き飛ばし復帰後のひるみ無効時間（秒）

    // ─── 必殺技被弾スタン（SkillBlow）大きく吹き飛ばされて落下し、その間は行動不能 ───
    bool skillBlowPending_ = false;                  ///< 次のダメージをSkillBlow扱いにする予約
    Hagine::Vector3 skillBlowDirection_ = {0, 0, 0}; ///< 吹き飛ばす水平方向（正規化済み）
    float skillBlowTimer_ = 0.0f;                    ///< スタン開始からの経過時間
    float skillBlowSpeed_ = 35.0f;                   ///< 吹き飛ばしの水平初速
    float skillBlowRiseSpeed_ = 12.0f;               ///< 吹き飛ばしの上方初速（この後は落下する）
    float skillBlowHorizontalRetain_ = 0.12f;        ///< 1秒あたりに残る横速度の割合（小さいほど早く減速）
    float skillBlowMaxDuration_ = 5.0f;              ///< 着地しないまま落下し続ける最大時間（秒・安全策）
    float skillBlowDamageMultiplier_ = 0.2f;         ///< スタン中の被ダメージ倍率（0.2＝80%軽減）
};

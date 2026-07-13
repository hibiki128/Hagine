#pragma once
#include "Application/staging/death/DeathStaging.h"
#include "Application/system/tutorial/TutorialSystem.h"
#include "data/DataHandler.h"
#include "GamePad.h"
#include "object/base/BaseObject.h"
#include "parts/PlayerCombat.h"
#include "parts/PlayerMovement.h"
#include "parts/PlayerStatus.h"
#include "parts/PlayerVisual.h"
#include "PlayerData.h"
#include "state/base/PlayerBaseState.h"
#include <Application/utility/shake/Shake.h>
#include <Input.h>
#include <particle/gpu/ParticleCSEmitter.h>
#include <particle/gpu/ParticleCSFieldManager.h>

class FollowCamera;
class Enemy;

/// <summary>
/// プレイヤーのゲームオブジェクトクラス
/// ステート管理と各パーツ（移動・戦闘・ステータス・見た目）の統括を行うファサード。
/// 実際のロジックは Parts/ 以下の各パーツクラスが担当する
/// </summary>
class Player : public Hagine::BaseObject
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    Player();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~Player();

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="objectName">オブジェクト名</param>
    void Init(const std::string objectName) override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void Draw(const Hagine::ViewProjection &viewProjection) override;

    /// <summary>
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const Hagine::ViewProjection &viewProjection);
    void DrawParticleCompute(const Hagine::ViewProjection &viewProjection);

    /// <summary>
    /// 状態を変更
    /// </summary>
    /// <param name="stateName">変更する状態名</param>
    void ChangeState(const std::string &stateName);

    /// <summary>
    /// 当たってる間
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollision(Hagine::ColliderBase *other);

    /// <summary>
    /// 当たった瞬間
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollisionEnter(Hagine::ColliderBase *other);

    /// <summary>
    /// デバッグ処理
    /// </summary>
    void Debug();

    /// <summary>
    /// チャージ状態に変更
    /// Idle / Move / FlyIdle / FlyMove の各ステートの Update() 末尾から呼ぶ
    /// </summary>
    void ChangeEnergyCharge();

    /// <summary>
    /// プレイヤー設定を保存
    /// </summary>
    void Save();

    /// <summary>
    /// プレイヤー設定を読み込み
    /// </summary>
    void Load();

    /// <summary>
    /// ロックオンを解除する
    /// 解除タイミングは呼び出し側が管理する
    /// </summary>
    void ReleaseLockOn() { isLockOn_ = false; }

    /// ===================================================
    /// パーツ（新規コードはこちらを直接使ってよい）
    /// ===================================================
    PlayerMovement &Movement() { return *movement_; }
    PlayerCombat &Combat() { return *combat_; }
    PlayerStatus &Status() { return *status_; }
    PlayerVisual &Visual() { return *visual_; }

    /// ===================================================
    /// パーツへの転送（既存の呼び出し側との互換用）
    /// ===================================================

    // ─── 移動（PlayerMovement） ───
    void Move() { movement_->Move(); }
    void DirectionUpdate() { movement_->DirectionUpdate(); }
    float CalculateShortestRotation(float from, float to) { return movement_->CalculateShortestRotation(from, to); }
    Hagine::Vector3 &GetAcceleration() { return movement_->GetAcceleration(); }
    Hagine::Vector3 &GetVelocity() { return movement_->GetVelocity(); }
    Hagine::Vector3 GetMovementDirection() const { return movement_->GetMovementDirection(); }
    float GetVelocityMagnitude() const { return movement_->GetVelocityMagnitude(); }
    float &GetFallSpeed() { return movement_->GetFallSpeed(); }
    float &GetMoveSpeed() { return movement_->GetMoveSpeed(); }
    float &GetJumpSpeed() { return movement_->GetJumpSpeed(); }
    float &GetMaxSpeed() { return movement_->GetMaxSpeed(); }
    float &GetAccelRate() { return movement_->GetAccelRate(); }
    bool &GetCanJump() { return movement_->GetCanJump(); }
    bool &GetIsGrounded() { return movement_->GetIsGrounded(); }
    bool GetIsDashing() const { return movement_->GetIsDashing(); }
    float GetDashDuration() const { return movement_->GetDashDuration(); }
    bool GetDashStartedThisFrame() const { return movement_->GetDashStartedThisFrame(); }
    Direction &GetDirection() { return movement_->GetDirection(); }
    MoveDirection &GetMoveDirection() { return movement_->GetMoveDirection(); }
    void ClearDashState() { movement_->ClearDashState(); }
    void SetDashing(bool flag) { movement_->SetDashing(flag); }
    void SetDashInput(float x, float z) { movement_->SetDashInput(x, z); }

    // ─── 戦闘（PlayerCombat） ───
    std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() { return combat_->GetBullets(); }
    PlayerAttackCollider *GetAttackCollider() { return combat_->GetAttackCollider(); }
    bool GetIsSkillActive() const { return combat_->IsSkillActive(); }
    float GetChargeThreshold() const { return combat_->GetChargeThreshold(); }

    /// <summary>パンチコンボが進行中か（敵BTの脅威判定から参照）</summary>
    bool IsComboActive() const { return combat_->GetPunchCombo().IsComboActive(); }

    /// <summary>
    /// このフレームに近接攻撃が発火していれば true を返し、その段名を out に格納する。
    /// 取得すると内部フラグはクリアされる（1発火につき1回だけ true）。入力表示UI用。
    /// </summary>
    bool ConsumeMeleeAttackFired(std::string &outName) { return combat_->ConsumeMeleeAttackFired(outName); }

    // ─── ステータス（PlayerStatus） ───
    bool ConsumeEnergy(float amount) { return status_->ConsumeEnergy(amount); }
    void RecoverEnergy() { status_->RecoverEnergy(); }
    float GetHP() const { return status_->GetHP(); }
    float GetMaxHP() const { return status_->GetMaxHP(); }
    float GetEnergy() const { return static_cast<const PlayerStatus *>(status_.get())->GetEnergy(); }
    float &GetEnergy() { return status_->GetEnergy(); }
    float GetMaxEnergy() const { return status_->GetMaxEnergy(); }
    float GetChargeRate() const { return status_->GetChargeRate(); }

    /// <summary>ガード中かどうか（敵BTの被弾リスク判定などから参照）</summary>
    bool IsGuarding() const { return status_->IsGuarding(); }

    /// <summary>ガード可能か（被弾で消費するエネルギーを支払えるか）</summary>
    bool CanGuard() const { return status_->CanGuard(); }

    void SetGuarding(bool flag) { status_->SetGuarding(flag); }
    void SetEnergyRecoveryRate(float rate) { status_->SetEnergyRecoveryRate(rate); }

    /// <summary>
    /// 外部からダメージ量をセット（次のDamageUpdateで処理される）
    /// </summary>
    /// <param name="damage">与えるダメージ量</param>
    void SetDamage(float damage) { status_->SetDamage(damage); }

    /// <summary>
    /// 外部からノックバックを与える
    /// EnemyHand::OnCollisionEnter から呼ばれる
    /// </summary>
    /// <param name="direction">ノックバック方向（正規化済みでなくてもよい）</param>
    /// <param name="power">ノックバック強度</param>
    void SetKnockback(const Hagine::Vector3 &direction, float power) { status_->SetKnockback(direction, power); }

    /// ===================================================
    /// 入力表示UI用
    /// ===================================================

    /// <summary>
    /// 入力表示UI用のアクション種別。入力の有無ではなく「実際に発動したもの」だけを通知する。
    /// </summary>
    enum class ActionKind
    {
        NormalShot,   // 通常射撃
        ChargeStart,  // チャージ開始（溜め始め）
        ChargeShot,   // チャージ弾発射
        Special,      // 必殺技
        Dash,         // ダッシュ
        Rush,         // 急接近（ラッシュ）
        Guard,        // ガード
        EnergyCharge, // エネルギーチャージ
    };

    /// <summary>このフレームに実際に発動したアクションイベント列（入力表示UI用）</summary>
    const std::vector<ActionKind> &GetActionEvents() const { return actionEvents_; }

    /// <summary>アクションイベントを1件記録する（各アクションの実発動箇所から呼ぶ）</summary>
    void EmitAction(ActionKind kind) { actionEvents_.push_back(kind); }

    /// ===================================================
    /// Getter
    /// ===================================================
    Hagine::GamePad *GetGamePad() { return gamePad_.get(); }
    Hagine::Input *GetInput() { return pInput_; }
    FollowCamera *GetCamera() { return FollowCamera_; }
    Enemy *GetEnemy() { return pEnemy_; }
    Hagine::Vector3 GetForward() const;
    Hagine::Vector3 GetBackward() const;
    Hagine::Vector3 GetRight() const;
    Hagine::Vector3 GetLeft() const;
    Hagine::Vector3 GetUp() const;
    Hagine::Vector3 GetDown() const;
    Hagine::Vector3 GetPositionBehind(float distance = 3.0f) const;
    Hagine::Vector3 GetPositionFront(float distance = 3.0f) const;
    Hagine::Vector3 GetPositionRight(float distance = 3.0f) const;
    Hagine::Vector3 GetPositionLeft(float distance = 3.0f) const;
    Hagine::Vector3 GetPositionAbove(float distance = 3.0f) const;
    Hagine::Vector3 GetPositionBelow(float distance = 3.0f) const;
    float &GetDt() { return dt_; }
    bool &GetAlive() { return isAlive_; }
    bool &GetIsLockOn() { return isLockOn_; }
    bool GetIsPause() const { return isPause_; }
    Hagine::ViewProjection &GetViewProjection() { return *pVp_; }
    std::string GetCurrentStateName() const;
    std::string GetPreviewStateName() const { return previousStateName_; }
    TutorialStep GetTutorialStep() const { return tutorialStep_; }

    /// <summary>ワールドトランスフォームのポインタを取得（必殺技の追従用）</summary>
    Hagine::WorldTransform *GetTransformPtr() { return transform_.get(); }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetCamera(FollowCamera *camera) { FollowCamera_ = camera; }
    void SetVp(Hagine::ViewProjection *vp);
    void SetStart(bool flag) { started_ = flag; }
    void SetPause(bool flag) { isPause_ = flag; }
    void SetEnemy(Enemy *enemy);
    void SetIsLockOn(bool flag) { isLockOn_ = flag; }
    void SetIsDeathStaging(bool flag) { isDeathStaging_ = flag; }
    void SetActiveDebugCamera(bool flag) { activeDebugCamera_ = flag; }

    /// <summary>
    /// チュートリアルの現在ステップを受け取る
    /// TutorialScene::Update() から毎フレーム呼ぶ
    /// EnergyCharge ステップへの切替時はエネルギーを 0 にリセットする
    /// </summary>
    void SetTutorialStep(TutorialStep step);

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// ガード入力を判定して Guard ステートへ遷移する
    /// 地上(Idle/Move)・飛行(FlyIdle/FlyMove)から押しっぱ式で発動
    /// </summary>
    void UpdateGuardInput();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 回転・ベクトル定数
    static constexpr float kForwardVectorX = 0.0f;
    static constexpr float kForwardVectorY = 0.0f;
    static constexpr float kForwardVectorZ = -1.0f;
    static constexpr float kRightVectorX = 1.0f;
    static constexpr float kRightVectorY = 0.0f;
    static constexpr float kRightVectorZ = 0.0f;
    static constexpr float kUpVectorX = 0.0f;
    static constexpr float kUpVectorY = 1.0f;
    static constexpr float kUpVectorZ = 0.0f;

    // FOV関連定数
    static constexpr float kNormalFov = 45.0f;
    static constexpr float kDashingFov = 55.0f;

    // ─── パーツ ───
    std::unique_ptr<PlayerMovement> movement_; // 移動・回転・ダッシュ・接地
    std::unique_ptr<PlayerCombat> combat_;     // 射撃・コンボ・必殺技
    std::unique_ptr<PlayerStatus> status_;     // HP・エネルギー・被ダメージ・ガード
    std::unique_ptr<PlayerVisual> visual_;     // アニメーション・飛行リーン

    FollowCamera *FollowCamera_ = nullptr;
    Enemy *pEnemy_ = nullptr;

    float dt_ = 0.0f; // デルタタイム

    float currentFov_ = 45.0f;  // 現在のFOV
    float targetFov_ = 45.0f;   // 目標FOV
    float fovLerpSpeed_ = 5.0f; // FOV補間速度

    bool isLockOn_ = false;       // ロックオンフラグ
    bool started_ = false;        // ゲーム開始フラグ
    bool isPause_ = false;        // ポーズ中フラグ
    bool isDeathStaging_ = false; // 死亡演出中フラグ
    bool wasRTPressed_ = false;   // 前フレームのRT押下状態
    bool wasSkillLocked_ = false; // 前フレームの必殺技演出ロック状態（ロック開始検出用）

    // 入力表示UI用: 実発動アクションのイベント列（Update先頭でクリア）
    std::vector<ActionKind> actionEvents_;

    std::unordered_map<std::string, std::unique_ptr<PlayerBaseState>> states_; // 状態マップ
    PlayerBaseState *pCurrentState_ = nullptr;                                  // 現在の状態
    std::string previousStateName_ = "";

    std::unique_ptr<Hagine::DataHandler> data_;              // データ管理
    std::unique_ptr<Shake> shake_;                           // シェイク
    std::unique_ptr<Hagine::ParticleCSEmitter> auraEmitter_; // オーラパーティクル
    std::unique_ptr<Hagine::ParticleEmitter> hitEmitter_;    // 被弾ヒットエミッター
    std::unique_ptr<DeathStaging> deathStaging_;             // 死亡演出
    std::unique_ptr<Hagine::GamePad> gamePad_;               // ゲームパッド

    Hagine::ViewProjection *pVp_ = nullptr;               // カメラ
    Hagine::OBBCollider *pPlayerCollider_ = nullptr;      // コライダー
    Hagine::AABBCollider *pPlayerWallCollider_ = nullptr; // 壁用コライダー
    Hagine::ParticleField *pGeneratedField_ = nullptr;    // 生成するフィールド
    Hagine::Input *pInput_ = nullptr;                     // 入力

    bool activeDebugCamera_ = false; // デバッグカメラがアクティブかどうか

    // ─── チュートリアル連携 ───
    TutorialStep tutorialStep_ = TutorialStep::Move; ///< シーンから渡された現在のチュートリアルステップ
};

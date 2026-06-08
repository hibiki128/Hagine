#pragma once
#include "Application/Staging/Death/DeathStaging.h"
#include "Application/System/Tutorial/TutorialSystem.h"
#include "Bullet/PlayerBullet.h"
#include "Collider/PlayerAttackCollider.h"
#include "Data/DataHandler.h"
#include "GamePad.h"
#include "Object/Base/BaseObject.h"
#include "PlayerData.h"
#include "Skill/MakanAttackSkill.h"
#include "State/Base/PlayerBaseState.h"
#include <Animation/AnimationController.h>
#include <Application/Utility/Shake/Shake.h>
#include <Input.h>
#include <Particle/CSParticle/ParticleCSEmitter.h>
#include <Particle/CSParticle/ParticleCSFieldManager.h>
#include <application/Utility/ComboSystem/ComboSystem.h>

class ChargeShot;
class FollowCamera;
class Enemy;

/// <summary>
/// プレイヤーのゲームオブジェクトクラス
/// 状態管理、移動、攻撃、カメラ制御などを行う
/// </summary>
class Player : public Hagine::BaseObject {
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
    /// 方向情報を更新
    /// </summary>
    void DirectionUpdate();

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
    /// 最短回転角度を計算
    /// </summary>
    /// <param name="from">開始角度</param>
    /// <param name="to">目標角度</param>
    /// <returns>float: 計算された最短回転角度</returns>
    float CalculateShortestRotation(float from, float to);

    /// <summary>
    /// 移動処理
    /// カメラ方向・入力・加速度に基づいて velocity を更新する
    /// </summary>
    void Move();

    bool ConsumeEnergy(float amount); // エネルギー消費処理
    void RecoverEnergy();             // エネルギー回復処理

    /// <summary>
    /// ロックオンを解除する
    /// 解除タイミングは呼び出し側が管理する
    /// </summary>
    void ReleaseLockOn();

    /// ===================================================
    /// Getter
    /// ===================================================
    Hagine::GamePad *GetGamePad() { return gamePad_.get(); }
    FollowCamera *GetCamera() { return FollowCamera_; }
    Enemy *GetEnemy() { return enemy_; }
    Hagine::Vector3 &GetAcceleration() { return acceleration_; }
    Hagine::Vector3 &GetVelocity() { return velocity_; }
    Hagine::Vector3 GetMovementDirection() const;
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
    float GetVelocityMagnitude() const;
    float &GetFallSpeed() { return fallSpeed_; }
    float &GetMoveSpeed() { return moveSpeed_; }
    float &GetJumpSpeed() { return jumpSpeed_; }
    float &GetMaxSpeed() { return maxSpeed_; }
    float &GetAccelRate() { return accelRate_; }
    float GetChargeThreshold() const { return kYButtonChargeThreshold; }
    float &GetDt() { return dt_; }
    float GetHP() const { return HP_; }
    float GetMaxHP() const { return maxHP_; }
    float GetEnergy() const { return energy_; }
    float &GetEnergy() { return energy_; }
    float GetMaxEnergy() const { return maxEnergy_; }
    float GetChargeRate() const { return energyRecoveryRate_; }
    bool &GetCanJump() { return canJump_; }
    bool &GetAlive() { return isAlive_; }
    bool &GetIsGrounded() { return isGrounded_; }
    bool &GetIsLockOn() { return isLockOn_; }
    bool GetIsPause() const { return isPause_; }
    bool GetIsDashing() const { return isDashing_; }
    float GetDashDuration() const { return dashDuration_; }
    bool GetDashStartedThisFrame() const { return dashStartedThisFrame_; }
    Hagine::ViewProjection &GetViewProjection();
    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }
    std::string GetCurrentStateName() const;
    std::string GetPreviewStateName() const { return previousStateName_; }
    std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() { return bullets_; }
    PlayerAttackCollider *GetAttackCollider() { return attackCollider_.get(); }
    bool GetIsSkillActive() const { return makanAttack_ptr_->IsActive(); }

    /// <summary>ガード中かどうか（敵BTの被弾リスク判定などから参照）</summary>
    bool IsGuarding() const { return isGuarding_; }

    /// <summary>ガード可能か（被弾で消費するエネルギーを支払えるか）</summary>
    bool CanGuard() const { return energy_ >= guardEnergyCost_; }

    /// <summary>パンチコンボが進行中か（敵BTの脅威判定から参照）</summary>
    bool IsComboActive() const { return punchCombo_.IsComboActive(); }

    void SetIsLockOn(bool flag) { isLockOn_ = flag; }
    void SetGuarding(bool flag) { isGuarding_ = flag; }

    /// <summary>
    /// ダッシュ状態をリセットする
    /// Rush 遷移後など、ステート側からダッシュを解除する際に使う
    /// </summary>
    void ClearDashState() {
        isDashing_ = false;
        dashDuration_ = 0.0f;
    }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetCamera(FollowCamera *camera);
    void SetVp(Hagine::ViewProjection *vp);
    void SetStart(bool flag) {
        started_ = flag;
    }
    void SetPause(bool flag);
    void SetEnemy(Enemy *enemy) {
        enemy_ = enemy;
        if (attackCollider_) {
            attackCollider_->SetEnemy(enemy);
        }
    }
    void SetIsDeathStaging(bool flag) {
        isDeathStaging_ = flag;
    }
    void SetEnergyRecoveryRate(float Rate) { energyRecoveryRate_ = Rate; }
    void SetDashing(bool flag) { isDashing_ = flag; }
    void SetDashInput(float x, float z) {
        dashInputX_ = x;
        dashInputZ_ = z;
    }

    /// <summary>
    /// 外部からダメージ量をセット（次のDamageUpdateで処理される）
    /// </summary>
    /// <param name="damage">与えるダメージ量</param>
    void SetDamage(float damage) { damage_ = damage; }
    void SetActiveDebugCamera(bool flag) { activeDebugCamrera_ = flag; }

    /// <summary>
    /// チュートリアルの現在ステップを受け取る
    /// TutorialScene::Update() から毎フレーム呼ぶ
    /// EnergyCharge ステップへの切替時はエネルギーを 0 にリセットする
    /// </summary>
    void SetTutorialStep(TutorialStep step);

    /// <summary>
    /// 外部からノックバックを与える
    /// EnemyHand::OnCollisionEnter から呼ばれる
    /// </summary>
    /// <param name="direction">ノックバック方向（正規化済みでなくてもよい）</param>
    /// <param name="power">ノックバック強度</param>
    void SetKnockback(const Hagine::Vector3 &direction, float power);

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// プレイヤー設定を保存
    /// </summary>
    void Save();

    /// <summary>
    /// プレイヤー設定を読み込み
    /// </summary>
    void Load();

    /// <summary>
    /// コンボ更新処理
    /// </summary>
    void ComboUpdate();

    /// <summary>
    /// 射撃処理
    /// </summary>
    void Shot();

    /// <summary>
    /// スキルショット
    /// </summary>
    void SkillShot();

    /// <summary>
    /// 影のスケールを更新
    /// </summary>
    void UpdateShadowScale();

    /// <summary>
    /// 回転を更新（ロックオン追従 / スティック手動回転）
    /// </summary>
    void RotateUpdate();

    /// <summary>
    /// 地面との衝突判定処理
    /// </summary>
    void CollisionGround();

    /// <summary>
    /// 回転からDirection値を計算
    /// </summary>
    /// <returns>Direction: 計算されたDirection</returns>
    Direction CalculateDirectionFromRotation();

    /// <summary>
    /// 角度を正規化
    /// </summary>
    /// <param name="angle">正規化する角度</param>
    /// <returns>float: 正規化された角度</returns>
    float NormalizeAngle(float angle);

    /// <summary>
    /// Direction値を文字列で取得
    /// </summary>
    /// <param name="dir">方向の値</param>
    /// <returns>const char*: 方向の名前文字列</returns>
    const char *GetDirectionName(Direction dir);

    /// <summary>
    /// ダメージを受ける処理
    /// </summary>
    void DamageUpdate();

    /// <summary>
    /// 無敵時間の更新処理
    /// </summary>
    void InvincibleUpdate();

    /// <summary>
    /// ダッシュ継続時間と開始フラグを更新する
    /// </summary>
    void UpdateDashState();

    /// <summary>
    /// ガード入力を判定して Guard ステートへ遷移する
    /// 地上(Idle/Move)・飛行(FlyIdle/FlyMove)から押しっぱ式で発動
    /// </summary>
    void UpdateGuardInput();

    /// <summary>
    /// アニメーションの更新
    /// </summary>
    void UpdateAnimation();

    /// <summary>
    /// 飛行移動中の体の傾き（リーン）を更新する
    /// ロックオン中は顔（向き）を敵に向けたまま、進行方向へ体を傾けて見せる。
    /// 描画専用の回転オフセットとして適用するため、射撃などの向きには影響しない
    /// </summary>
    void UpdateFlyLean();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 初期化定数
    static constexpr float kShadowRotationDegrees = -90.0f;
    static constexpr float kShadowScale = 1.5f;
    static constexpr float kShadowYPosition = -0.95f;
    static constexpr float kRotationZero = 0.0f;

    // ダメージ・HP関連定数
    static constexpr float kMinHP = 0.0f;
    static constexpr float kEnemyCollisionDamage = 7.5f;
    static constexpr float kTimerReset = 0.0f;
    static constexpr float kNoDamage = 0.0f;

    // 点滅関連定数
    static constexpr float kPlayerBlinkInterval = 0.05f;
    static constexpr int kBlinkModulo = 2;
    static constexpr int kEvenBlink = 0;
    static constexpr float kPlayerAlphaTransparent = 0.3f;
    static constexpr float kAlphaOpaque = 1.0f;

    // 回転・ベクトル定数
    static constexpr float kXAxisX = 1.0f;
    static constexpr float kXAxisY = 0.0f;
    static constexpr float kXAxisZ = 0.0f;
    static constexpr float kForwardVectorX = 0.0f;
    static constexpr float kForwardVectorY = 0.0f;
    static constexpr float kForwardVectorZ = -1.0f;
    static constexpr float kRightVectorX = 1.0f;
    static constexpr float kRightVectorY = 0.0f;
    static constexpr float kRightVectorZ = 0.0f;
    static constexpr float kUpVectorX = 0.0f;
    static constexpr float kUpVectorY = 1.0f;
    static constexpr float kUpVectorZ = 0.0f;

    // 速度・移動関連定数
    static constexpr float kMaxFallVelocity = -40.0f;
    static constexpr float kGroundLevel = 0.0f;
    static constexpr float kVelocityZero = 0.0f;
    static constexpr float kRushGroundOffset = 0.1f;
    static constexpr float kLandingSpeedThreshold = 0.5f;
    static constexpr float kMinRotationDistance = 0.001f;
    static constexpr float kParallelThreshold = 0.999f;
    static constexpr float kPlayerRotationSpeed = 10.0f;
    static constexpr float kManualRotationSpeed = 0.04f;
    static constexpr float kFlyVerticalAnimThreshold = 0.5f; // 飛行中、上昇・下降アニメに切り替えるY速度の閾値

    // 入力・移動制御定数
    static constexpr float kInputZero = 0.0f;
    static constexpr float kInputValue = 1.0f;
    static constexpr float kDecelerationFactor = 0.65f;
    static constexpr float kVelocityStopThreshold = 0.01f;
    static constexpr float kYComponentZero = 0.0f;
    static constexpr float kDashSpeedMultiplier = 1.5f;

    // 弾丸関連定数
    static constexpr float kBulletScale = 0.5f;
    static constexpr float kBulletColliderRadius = 0.5f;
    static constexpr float kNormalShotEnergyCost = 5.0f;
    static constexpr float kSkillShotEnergyCost = 65.0f;

    // FOV関連定数
    static constexpr float kNormalFov = 45.0f;
    static constexpr float kDashingFov = 55.0f;

    // 影のスケール関連定数
    static constexpr float kShadowBaseScale = 1.5f;
    static constexpr float kShadowMinScale = 0.3f;
    static constexpr float kShadowScaleFactor = 0.1f;

    FollowCamera *FollowCamera_;
    Enemy *enemy_ = nullptr;

    Direction dir_;
    MoveDirection moveDir_;

    Hagine::Vector3 velocity_{};
    Hagine::Vector3 acceleration_{};

    float moveSpeed_ = 0.0f; // 移動速度
    float fallSpeed_ = 0.0f; // 落下速度
    float jumpSpeed_ = 0.0f; // ジャンプ速度
    float maxSpeed_ = 0.0f;  // 最大速度
    float accelRate_ = 0.0f; // 加速度レート
    float dt_;               // デルタタイム
    float HP_ = 100.0f;
    float maxHP_ = 100.0f;
    float energy_ = 100.0f;                      // 現在のエネルギー
    float maxEnergy_ = 100.0f;                   // 最大エネルギー
    float energyRecoveryRate_ = 0.01f;           // エネルギー回復速度(秒速)
    float energyRecoveryDelay_ = 1.0f;           // 回復開始までの遅延時間
    float timeSinceLastShot_ = 0.0f;             // 最後に撃ってからの経過時間
    float yButtonHoldTime_ = 0.0f;               // Yボタン押下時間
    const float kYButtonChargeThreshold = 0.15f; // チャージ判定閾値(秒)

    float currentFov_ = 45.0f;  // 現在のFOV
    float targetFov_ = 45.0f;   // 目標FOV
    float fovLerpSpeed_ = 5.0f; // FOV補間速度

    float B_acce_ = 0.0f;  // ブーストの加速度
    float B_speed_ = 0.0f; // ブーストの速度

    bool canJump_ = false;   // ジャンプ可能フラグ
    bool isLockOn_ = false;  // ロックオンフラグ
    bool isGrounded_ = true; // 接地フラグ
    bool isDashing_ = false; // ダッシュ中フラグ
    bool isGuarding_ = false; // ガード中フラグ
    float guardDamageMultiplier_ = 0.20f; // ガード中の被ダメージ倍率（軽減率80%）ImGuiで調整可
    float guardEnergyCost_ = 10.0f;       // ガード中に被弾した際のエネルギー消費量
    bool isSkillMenu_ = false;

    float dashInputX_ = 0.0f;           // ダッシュ開始時のスティックX入力
    float dashInputZ_ = 0.0f;           // ダッシュ開始時のスティックZ入力
    bool dashStartedThisFrame_ = false; // ダッシュ開始フラグ
    float dashDuration_ = 0.0f;         // ダッシュ継続時間
    bool wasDashing_ = false;           // 前フレームのダッシュ状態

    bool wasRTPressed_ = false; // 前フレームのRT押下状態

    bool started_ = false;        // ゲーム開始フラグ
    bool isPause_ = false;        // ポーズ中フラグ
    bool isDeathStaging_ = false; // 死亡演出中フラグ

    bool isInvincible_ = false;        // 無敵状態フラグ
    float invincibleTime_ = 0.0f;      // 無敵時間の経過時間
    float invincibleDuration_ = 0.25f; // 無敵時間の長さ(秒)
    float damage_ = 0.0f;

    ComboSystem punchCombo_;
    bool comboInitialized_ = false;            // コンボ初期化済みフラグ
    std::vector<std::string> comboAnimations_; // コンボ段ごとのプレイヤー本体アニメーションパス

    // ─── 飛行移動中の体の傾き（リーン）───
    // ロックオン飛行移動中、顔は敵向きのまま進行方向へ体を倒して見せるための描画専用の傾き。
    // すべて ImGui で調整可・セーブ対象。後ろ移動＝仰向け、左右移動＝バンク表現
    bool flyLeanEnabled_ = true;              // リーン演出の有効/無効
    float flyLeanMaxFwdPitchDeg_ = 10.0f;     // 前進時の前傾の最大角（度）
    float flyLeanMaxBackPitchDeg_ = 50.0f;    // 後退時の仰け反り（仰向け）の最大角（度）
    float flyLeanMaxSideDeg_ = 70.0f;         // 左右移動時に体を進行方向へ向けるヨー(Y回転)の最大角（度。90で真横）
    float flyLeanRefSpeed_ = 8.0f;            // 最大傾きに達する基準の水平速度
    float flyLeanResponse_ = 10.0f;           // 傾きの追従速度（大きいほど即座に追従）
    Hagine::Vector3 flyLeanPivot_ = {0.0f, 1.0f, 0.0f}; // 回転中心（モデル中心が原点にないため補正用）
    Hagine::Quaternion flyLeanRotation_ = Hagine::Quaternion::IdentityQuaternion(); // 現在の傾き（平滑化後・クォータニオン）

    Hagine::AnimationController animationController_; // アニメーション制御

    std::unordered_map<std::string, std::unique_ptr<PlayerBaseState>> states_; // 状態マップ

    std::vector<std::unique_ptr<PlayerBullet>> bullets_; // 発射した弾

    std::unique_ptr<Hagine::DataHandler> data_;              // データ管理
    std::unique_ptr<Hagine::BaseObject> shadow_;             // 影
    std::unique_ptr<ChargeShot> chargeShot_;         // チャージショット
    std::unique_ptr<Shake> shake_;                   // シェイク
    std::unique_ptr<Hagine::ParticleCSEmitter> auraEmitter_; // オーラパーティクル
    std::unique_ptr<Hagine::ParticleEmitter> hitEmitter_;
    std::unique_ptr<DeathStaging> deathStaging_;    // 死亡演出
    std::unique_ptr<MakanAttackSkill> makanAttack_; // 必殺技
    std::unique_ptr<Hagine::GamePad> gamePad_;
    std::unique_ptr<PlayerAttackCollider> attackCollider_; // 前方攻撃判定

    Hagine::ViewProjection *vp_;                    // カメラ
    Hagine::OBBCollider *playerCollider_ = nullptr; // コライダー
    Hagine::AABBCollider *playerWallCollider_ = nullptr;
    PlayerBaseState *currentState_ = nullptr;     // 現在の状態
    MakanAttackSkill *makanAttack_ptr_ = nullptr; // 必殺技
    Hagine::ParticleField *generatedField_ = nullptr;     // 生成するフィールド

    bool isDamageReact_ = false;       // リアクション中かどうか（被弾点滅）
    float damageReactTimer_ = 0.0f;    // 経過時間
    float damageReactDuration_ = 0.5f; // リアクション時間

    std::string previousStateName_ = "";

    Hagine::Input *input_ = nullptr;

    bool activeDebugCamrera_ = false;                // デバッグカメラがアクティブかどうか
    Hagine::Vector3 knockbackVelocity_ = {0.0f, 0.0f, 0.0f}; // 適用待ちノックバック
    bool hasKnockback_ = false;

    // ─── チュートリアル連携 ───
    TutorialStep tutorialStep_ = TutorialStep::Move; ///< シーンから渡された現在のチュートリアルステップ
};
#pragma once
#include "Application/Staging/Death/DeathStaging.h"
#include "Bullet/PlayerBullet.h"
#include "Data/DataHandler.h"
#include "GamePad.h"
#include "Hand/PlayerHand.h"
#include "Object/Base/BaseObject.h"
#include "PlayerData.h"
#include "Skill/MakanAttackSkill.h"
#include "State/Base/PlayerBaseState.h"
#include <Application/Utility/Shake/Shake.h>
#include <Input.h>
#include <Particle/CSParticle/ParticleCSEmitter.h>
#include <application/Utility/ComboSystem/ComboSystem.h>

class ChargeShot;
class FollowCamera;
class Enemy;

/// <summary>
/// プレイヤーのゲームオブジェクトクラス
/// 状態管理、移動、攻撃、カメラ制御などを行う
/// </summary>
class Player : public BaseObject {
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
    /// <param name="offSet">描画オフセット</param>
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    /// <summary>
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const ViewProjection &viewProjection);

    /// <summary>
    /// 状態を変更
    /// </summary>
    /// <param name="stateName">変更する状態名</param>
    void ChangeState(const std::string &stateName);

    /// <summary>
    /// 当たってる間
    /// </summary>
    /// <param name="other"></param>
    void OnCollision(ColliderBase *other);

    /// <summary>
    /// 方向情報を更新
    /// </summary>
    void DirectionUpdate();

    /// <summary>
    /// デバッグ処理
    /// </summary>
    void Debug();

    /// <summary>
    /// 突撃状態に変更
    /// </summary>
    void ChangeRush();

    /// <summary>
    /// チャージ状態に変更
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
    /// </summary>
    void Move();

    /// <summary>
    /// コントロール入力カウントをリセット
    /// </summary>
    void ResetControlCount() {
        lControlInputTime_ = 0.0f;
        lControlInputCount_ = 0;
    }

    bool ConsumeEnergy(float amount); // エネルギー消費処理
    void RecoverEnergy();             // エネルギー回復処理

    /// <summary>
    /// Getter
    /// </summary>
    GamePad *GetGamePad() { return gamePad_.get(); }
    FollowCamera *GetCamera() { return FollowCamera_; }
    Enemy *GetEnemy() { return enemy_; }
    Vector3 &GetAcceleration() { return acceleration_; }
    Vector3 &GetVelocity() { return velocity_; }
    Vector3 GetMovementDirection() const;
    Vector3 GetForward() const;
    Vector3 GetBackward() const;
    Vector3 GetRight() const;
    Vector3 GetLeft() const;
    Vector3 GetUp() const;
    Vector3 GetDown() const;
    Vector3 GetPositionBehind(float distance = 3.0f) const;
    Vector3 GetPositionFront(float distance = 3.0f) const;
    Vector3 GetPositionRight(float distance = 3.0f) const;
    Vector3 GetPositionLeft(float distance = 3.0f) const;
    Vector3 GetPositionAbove(float distance = 3.0f) const;
    Vector3 GetPositionBelow(float distance = 3.0f) const;
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
    ViewProjection &GetViewProjection();
    PlayerHand *GetRightHand() { return rightHand_ptr_; }
    PlayerHand *GetLeftHand() { return leftHand_ptr_; }
    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }
    std::string GetCurrentStateName() const;
    std::string GetPreviewStateName() const { return previousStateName; }
    std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() { return bullets_; }

    /// <summary>
    /// Setter
    /// </summary>
    void SetCamera(FollowCamera *camera);
    void SetVp(ViewProjection *vp);
    void SetStart(bool flag) {
        started_ = flag;
    }
    void SetEnemy(Enemy *enemy) {
        enemy_ = enemy;
        leftHand_ptr_->SetEnemy(enemy);
        rightHand_ptr_->SetEnemy(enemy);
    }
    void SetIsDeathStaging(bool flag) {
        isDeathStaging_ = flag;
    }
    void SetEnergyRecoveryRate(float Rate) { energyRecoveryRate_ = Rate; }

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
    /// 回転を更新
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
    static constexpr float kPlayerDamageTiltDegrees = 20.0f;

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

    // 移動コスト定数
    static constexpr float kRushEnergyCost = 30.0f;

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

    Vector3 velocity_{};
    Vector3 acceleration_{};

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

    float lControlInputTime_ = 0.0f;     // L操作入力の保持時間
    int lControlInputCount_ = 0;         // L操作入力の回数
    const float INPUT_RESET_TIME = 0.3f; // 入力リセット時間

    float currentFov_ = 45.0f;  // 現在のFOV
    float targetFov_ = 45.0f;   // 目標FOV
    float fovLerpSpeed_ = 5.0f; // FOV補間速度

    float B_acce_ = 0.0f;  // ブーストの加速度
    float B_speed_ = 0.0f; // ブーストの速度

    bool canJump_ = false;   // ジャンプ可能フラグ
    bool isLockOn_ = false;  // ロックオンフラグ
    bool isGrounded_ = true; // 接地フラグ
    bool isDashing_ = false; // ダッシュ中フラグ
    bool isSkillMenu_ = false;
    bool rushXButtonPressed_ = false; // Xボタンが押された状態
    bool rushAButtonPressed_ = false; // Aボタンが押された状態
    float rushXButtonTime_ = 0.0f;    // Xボタンが押されてからの経過時間
    float rushAButtonTime_ = 0.0f;    // Aボタンが押されてからの経過時間

    bool started_ = false;        // ゲーム開始フラグ
    bool isDeathStaging_ = false; // 死亡演出中フラグ

    bool isInvincible_ = false;        // 無敵状態フラグ
    float invincibleTime_ = 0.0f;      // 無敵時間の経過時間
    float invincibleDuration_ = 0.25f; // 無敵時間の長さ(秒)

    ComboSystem punchCombo_;
    bool comboInitialized_ = false; // コンボ初期化済みフラグ

    std::unordered_map<std::string, std::unique_ptr<PlayerBaseState>> states_; // 状態マップ

    std::vector<std::unique_ptr<PlayerBullet>> bullets_; // 発射した弾

    std::unique_ptr<DataHandler> data_;              // データ管理
    std::unique_ptr<BaseObject> shadow_;             // 影
    std::unique_ptr<ChargeShot> chargeShot_;         // チャージショット
    std::unique_ptr<PlayerHand> leftHand_;           // 左手
    std::unique_ptr<PlayerHand> rightHand_;          // 右手
    std::unique_ptr<Shake> shake_;                   // シェイク
    std::unique_ptr<ParticleCSEmitter> auraEmitter_; // オーラパーティクル
    std::unique_ptr<DeathStaging> deathStaging_;     // 死亡演出
    std::unique_ptr<MakanAttackSkill> makanAttack_;  // 必殺技
    std::unique_ptr<GamePad> gamePad_;

    ViewProjection *vp_;                          // カメラ
    OBBCollider *playerCollider_ = nullptr;       // コライダー
    PlayerHand *leftHand_ptr_;                    // 左手
    PlayerHand *rightHand_ptr_;                   // 右手
    PlayerBaseState *currentState_ = nullptr;     // 現在の状態
    MakanAttackSkill *makanAttack_ptr_ = nullptr; // 必殺技

    bool isDamageReact_ = false;       // リアクション中かどうか
    float damageReactTimer_ = 0.0f;    // 経過時間
    float damageReactDuration_ = 0.5f; // リアクション時間
    EasingData<float> tiltEase_;       // 回転角イージング
    Quaternion baseRotation_;          // 通常時の向き
    Quaternion tiltRotation_;          // のけぞり用の回転

    std::string previousStateName = "";

    Input *input_ = nullptr;
};
#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Collider/EnemyAttackCollider.h"
#include "Bullet/EnemyBullet.h"
#include "Hand/EnemyHand.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"
#include <Application/GameObject/BehaviorTree/Node/BehaviorNode.h>
#include <Application/GameObject/Player/Player.h>
#include <Easing.h>
#include <application/GameObject/Player/PlayerData.h>
#include <application/Utility/ComboSystem/ComboSystem.h>
#include <memory>

/// <summary>
/// 敵のゲームオブジェクトクラス
/// ビヘイビアツリーに基づいて行動し、プレイヤーとの相互作用を管理する
/// </summary>
class Enemy : public BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    Enemy();
    ~Enemy() override;

    void Init(const std::string objectName) override;
    void Update() override;
    void Draw(const ViewProjection &viewProjection,
              Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;
    void DrawParticle(const ViewProjection &viewProjection);
    void DrawFrustum();
    void Debug();

    void OnCollisionEnter(ColliderBase *collider);
    void OnCollision(ColliderBase *collider);
    void ConboUpdate();

    /// ===================================================
    /// Getter
    /// ===================================================

    Vector3 &GetAcceleration() { return acceleration_; }
    Vector3 GetVelocity() { return velocity_; }
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
    Vector3 GetPosition() const { return transform_->translation_; }
    Vector3 GetLocalPosition() const { return transform_->translation_; }
    float GetVelocityMagnitude() const;
    float &GetFallSpeed() { return fallSpeed_; }
    float &GetMoveSpeed() { return moveSpeed_; }
    float &GetJumpSpeed() { return jumpSpeed_; }
    float &GetMaxSpeed() { return maxSpeed_; }
    float &GetAccelRate() { return accelRate_; }
    float GetHP() const { return HP_; }
    float GetMaxHP() const { return maxHP_; }
    float &GetEnergy() { return energy_; }
    float GetMaxEnergy() const { return maxEnergy_; }
    bool &GetCanJump() { return canJump_; }
    bool &GetAlive() { return isAlive_; }
    bool &GetIsGrounded() { return isGrounded_; }
    bool IsGuarding() const { return isGuarding_; }
    bool GetIsLockOn() const { return isLockOn_; }
    Player *GetTarget() { return target_; }
    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }
    EnemyHand *GetRightHand() { return rightHand_ptr_; }
    EnemyHand *GetLeftHand() { return leftHand_ptr_; }

    /// <summary>
    /// 前方攻撃判定コライダーを返す
    /// </summary>
    EnemyAttackCollider *GetAttackCollider() { return attackCollider_.get(); }
    float GetVerticalVelocity() const { return velocity_.y; }
    float GetVerticalAcceleration() const { return acceleration_.y; }
    bool GetIsFlying() const { return isFlying_; }

    // コンボ状態Getter（BTノードから参照）
    int GetPunchComboLength() const { return punchCombo_.GetComboLength(); }
    bool IsPunchComboActive() const { return punchCombo_.IsComboActive(); }

    /// <summary>
    /// 現在の攻撃のダメージ量を返す（EnemyHandから参照）
    /// </summary>
    float GetCurrentAttackDamage() const { return currentAttackDamage_; }

    /// <summary>
    /// 現在の攻撃のノックバック強度を返す（EnemyHandから参照）
    /// </summary>
    float GetCurrentAttackKnockback() const { return currentAttackKnockback_; }

    float GetEnergyRecoveryRate() const { return energyRecoveryRate_; }

    // ConditionNode用
    Vector3 GetWorldPosition() const { return transform_->translation_; }

    /// ===================================================
    /// Setter
    /// ===================================================

    void SetDamage(float damage) { damage_ = damage; }

    /// <summary>
    /// ノックバックを設定する
    /// PlayerAttackCollider::OnCollisionEnterから呼ばれる
    /// </summary>
    /// <param name="direction">ノックバック方向（正規化済みでなくてもよい）</param>
    /// <param name="power">ノックバック強度</param>
    void SetKnockback(const Vector3 &direction, float power);

    void SetVp(ViewProjection *vp);
    void SetTarget(Player *target) {
        target_ = target;
        // 前方攻撃判定コライダーにもプレイヤーを設定する
        if (attackCollider_) {
            attackCollider_->SetPlayer(target);
        }
    }
    void SetGuarding(bool guarding) { isGuarding_ = guarding; }
    void SetIsLockOn(bool lockOn) { isLockOn_ = lockOn; }
    void SetStart(bool flag) { started_ = flag; }
    void SetPause(bool flag) { isPause_ = flag; }
    void SetDrawShadow(bool flag) { drawShadow_ = flag; }
    void SetVelocity(const Vector3 &vel) { velocity_ = vel; }
    void SetMoveSpeed(float speed) { moveSpeed_ = speed; }
    void SetStrafeDirection(int dir) { strafeDirection_ = dir; }
    void SetBehaviorTree(std::shared_ptr<BTNode> rootNode) {
        rootNode_ = rootNode;
        if (rootNode_) {
            rootNode_->SetContext(this, target_);
        }
    }
    void SetComboAttack(bool flag) { isComboAttack_ = flag; }
    void SetEnergy(float energy);
    void SetVerticalVelocity(float velocity) { velocity_.y = velocity; }
    void SetVerticalAcceleration(float accel) { acceleration_.y = accel; }
    void SetIsGrounded(bool grounded) { isGrounded_ = grounded; }
    void SetIsFlying(bool flying) { isFlying_ = flying; }
    void SetEnergyRecoveryRate(float rate) { energyRecoveryRate_ = rate; }
    void SetLocalPosition(const Vector3 &pos) { transform_->translation_ = pos; }

    bool ConsumeEnergy(float amount);
    void RecoverEnergy();

    void UpdateFrustumLockOn();
    void ReleaseLockOn() { isLockOn_ = false; }

    void MoveToTarget(const Vector3 &targetPos);
    void PerformAttack();
    void MoveStrafe();
    void MoveRetreat();
    void StopMovement();
    void Move();
    void DirectionUpdate();
    void Shot();
    void ShotWithDirection(const Vector3 &direction, bool forceHoming = false);

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    void Save();
    void Load();
    void UpdateShadowScale();
    void RotateUpdate();
    void CollisionGround();
    void DamageUpdate();
    void StartDamageReact();
    Direction CalculateDirectionFromRotation();
    const char *GetDirectionName(Direction dir);

    /// ===================================================
    /// private variants
    /// ===================================================

    // 初期化定数
    static constexpr int kTextureIndex = 0;
    static constexpr float kShadowRotationDegrees = -90.0f;
    static constexpr float kShadowScale = 1.5f;
    static constexpr float kShadowYPosition = -0.95f;

    // ダメージ・HP関連定数
    static constexpr float kNoDamage = 0.0f;
    static constexpr float kMinHP = 0.0f;
    static constexpr float kGuardDamageMultiplier = 0.15f;

    // 色関連定数
    static constexpr float kColorRed = 1.0f;
    static constexpr float kColorZero = 0.0f;
    static constexpr float kColorOpaque = 1.0f;

    // 点滅関連定数
    static constexpr float kBlinkInterval = 0.1f;
    static constexpr float kDamageBlinkInterval = 0.03f;
    static constexpr int kBlinkModulo = 2;
    static constexpr int kEvenBlink = 0;

    // 透明度定数
    static constexpr float kAlphaTransparent = 0.0f;
    static constexpr float kAlphaOpaque = 1.0f;

    // 回転・ベクトル定数
    static constexpr float kRotationZero = 0.0f;
    static constexpr float kTimerReset = 0.0f;
    static constexpr float kDamageTiltDegrees = 20.0f;
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

    // 距離・閾値定数
    static constexpr float kMinRotationDistance = 0.001f;
    static constexpr float kParallelThreshold = 0.999f;
    static constexpr float kRotationSpeed = 8.0f;
    static constexpr float kGroundLevel = 0.0f;
    static constexpr float kVelocityZero = 0.0f;

    // イージング関連定数
    static constexpr float kVelocityEaseTime = 0.15f;
    static constexpr float kStopEaseTime = 0.2f;

    // 影のスケール関連定数
    static constexpr float kShadowBaseScale = 1.5f;
    static constexpr float kShadowMinScale = 0.3f;
    static constexpr float kShadowScaleFactor = 0.1f;

    // ビヘイビアツリー距離定数
    static constexpr float kFarDistanceMin = 10.0f;
    static constexpr float kFarDistanceMax = 100.0f;
    static constexpr float kMidDistanceMin = 5.0f;
    static constexpr float kMidDistanceMax = 10.0f;
    static constexpr float kCloseDistanceMin = 0.0f;
    static constexpr float kCloseDistanceMax = 5.0f;

    // 弾丸関連定数
    static constexpr float kBulletScale = 0.5f;
    static constexpr float kBulletColliderRadius = 0.5f;
    static constexpr float kNormalShotEnergyCost = 5.0f;

    Direction dir_;          // 現在の方向
    MoveDirection moveDir_;  // 移動方向

    Vector3 velocity_{};      // 速度
    Vector3 acceleration_{};  // 加速度
    Player *target_ = nullptr; // ターゲットプレイヤー

    int strafeDirection_ = 1; // 横移動方向

    float HP_ = 100.0f;    // HP
    float maxHP_ = 100.0f; // 最大HP
    float damage_ = 0.0f;  // 受けるダメージ量

    float moveSpeed_ = 0.0f; // 移動速度
    float fallSpeed_ = 0.0f; // 落下速度
    float jumpSpeed_ = 0.0f; // ジャンプ速度
    float maxSpeed_ = 0.0f;  // 最大速度
    float accelRate_ = 0.0f; // 加速レート

    Vector3 velocityTarget_{};        // 目標速度
    EasingData<Vector3> velocityEase_; // 速度補間用イージング

    // -----------------------------------------------
    // ノックバック関連
    // -----------------------------------------------
    bool hasKnockback_ = false;            // ノックバック中フラグ
    Vector3 pendingKnockback_ = {0, 0, 0}; // ノックバック速度

    // -----------------------------------------------
    // コンボ攻撃パラメータ（ComboSystemのコールバックで更新される）
    // EnemyHandがヒット時に参照する
    // -----------------------------------------------
    float currentAttackDamage_ = 10.0f;   // 現在の攻撃ダメージ量
    float currentAttackKnockback_ = 3.0f; // 現在の攻撃ノックバック強度
    float currentAttackDuration_ = 0.25f; // 現在の攻撃有効時間

    bool canJump_ = false;       // ジャンプ可能フラグ
    bool isLockOn_ = false;      // ロックオンフラグ
    bool isGrounded_ = true;     // 接地フラグ
    bool isFlying_ = false;      // 飛行中フラグ
    bool isStop_ = false;        // 停止中フラグ
    bool started_ = false;       // 開始フラグ
    bool isPause_ = false;       // ポーズフラグ
    bool drawShadow_ = true;     // 影描画フラグ
    bool isGuarding_ = false;    // ガード中フラグ
    bool isComboAttack_ = false; // コンボ攻撃中フラグ

    std::unique_ptr<DataHandler> data_;      // データハンドラ
    std::unique_ptr<BaseObject> shadow_;     // 影オブジェクト
    std::unique_ptr<ParticleEmitter> hitEmitter_; // ヒットエミッター
    std::unique_ptr<Shake> chargeShake_;     // シェイク
    std::shared_ptr<BTNode> rootNode_ = nullptr; // ビヘイビアツリーのルートノード
    std::unique_ptr<EnemyHand> leftHand_;    // 左手
    std::unique_ptr<EnemyHand> rightHand_;   // 右手

    bool isDamageReact_ = false;       // ダメージ反応中フラグ
    float damageReactTimer_ = 0.0f;    // ダメージ反応タイマー
    float damageReactDuration_ = 0.5f; // ダメージ反応時間
    float energy_ = 100.0f;            // エネルギー
    float maxEnergy_ = 100.0f;         // 最大エネルギー
    float energyRecoveryRate_ = 0.01f; // エネルギー回復速度
    float energyRecoveryDelay_ = 1.0f; // エネルギー回復遅延
    float timeSinceLastShot_ = 0.0f;   // 最終射撃からの経過時間

    ComboSystem punchCombo_;       // パンチコンボシステム
    bool comboInitialized_ = false; // コンボ初期化済みフラグ

    // -----------------------------------------------
    // 前方攻撃判定コライダー
    // EnemyHandのコライダーの代わりに敵前方に判定を展開する
    // PlayerAttackColliderと対称の設計
    // -----------------------------------------------
    std::unique_ptr<EnemyAttackCollider> attackCollider_; // 攻撃コライダー

    EasingData<float> tiltEase_; // のけぞり回転イージング
    Quaternion baseRotation_;    // 基本回転
    Quaternion tiltRotation_;    // のけぞり回転

    OBBCollider *enemyCollider_ = nullptr;   // 敵コライダー
    AABBCollider *enemyWallCollider_ = nullptr; // 壁用コライダー

    EnemyHand *leftHand_ptr_;  // 左手ポインタ
    EnemyHand *rightHand_ptr_; // 右手ポインタ

    std::vector<std::unique_ptr<EnemyBullet>> bullets_; // 敵の弾

    // ===================================================
    // 視錐台ロックオン関連
    // ===================================================
    static constexpr float kDefaultFrustumRange = 150.0f;
    static constexpr float kDefaultFrustumHalfFovH = 40.0f * (3.14159265f / 180.0f);
    static constexpr float kDefaultFrustumHalfFovV = 30.0f * (3.14159265f / 180.0f);

    float frustumLockOnRange_ = kDefaultFrustumRange;   // 視錐台範囲
    float frustumLockOnHalfFovH_ = kDefaultFrustumHalfFovH; // 視錐台水平半角
    float frustumLockOnHalfFovV_ = kDefaultFrustumHalfFovV; // 視錐台垂直半角
    bool drawFrustumDebug_ = false; // 視錐台デバッグ表示フラグ
};
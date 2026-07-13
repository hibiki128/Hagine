#pragma once
#include "Object/Base/BaseObject.h"
#include "Parts/EnemyCombat.h"
#include "Parts/EnemyMovement.h"
#include "Parts/EnemyStatus.h"
#include "Parts/EnemyVisual.h"
#include "Particle/ParticleEmitter.h"
#include <Application/GameObject/BehaviorTree/Node/BehaviorNode.h>
#include <Application/GameObject/Player/Player.h>
#include <memory>

/// <summary>
/// 敵のゲームオブジェクトクラス
/// ビヘイビアツリー駆動で行動し、各パーツ（移動・戦闘・ステータス・見た目）の統括を行うファサード。
/// 視錐台ロックオン（視界）・BT・コライダー・トレーニング用ダミー制御を本体が担当し、
/// 実際のロジックは Parts/ 以下の各パーツクラスが担当する
/// </summary>
class Enemy : public Hagine::BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    Enemy();
    ~Enemy() override;

    void Init(const std::string objectName) override;
    void Update() override;
    void Draw(const Hagine::ViewProjection &viewProjection) override;
    void DrawParticle(const Hagine::ViewProjection &viewProjection);
    void DrawParticleCompute(const Hagine::ViewProjection &viewProjection);
    void DrawFrustum();
    void Debug();

    void OnCollisionEnter(Hagine::ColliderBase *collider);
    void OnCollision(Hagine::ColliderBase *collider);
    void ConboUpdate() { combat_->ConboUpdate(); }

    /// ===================================================
    /// パーツ（新規コードはこちらを直接使ってよい）
    /// ===================================================
    EnemyMovement &Movement() { return *movement_; }
    EnemyCombat &Combat() { return *combat_; }
    EnemyStatus &Status() { return *status_; }
    EnemyVisual &Visual() { return *visual_; }

    /// ===================================================
    /// Getter
    /// ===================================================

    // ─── 移動（EnemyMovement） ───
    Hagine::Vector3 &GetAcceleration() { return movement_->GetAcceleration(); }
    Hagine::Vector3 GetVelocity() { return movement_->GetVelocity(); }
    Hagine::Vector3 GetMovementDirection() const { return movement_->GetMovementDirection(); }
    float GetVelocityMagnitude() const { return movement_->GetVelocityMagnitude(); }
    float &GetFallSpeed() { return movement_->GetFallSpeed(); }
    float &GetMoveSpeed() { return movement_->GetMoveSpeed(); }
    float &GetJumpSpeed() { return movement_->GetJumpSpeed(); }
    float &GetMaxSpeed() { return movement_->GetMaxSpeed(); }
    float &GetAccelRate() { return movement_->GetAccelRate(); }
    bool &GetCanJump() { return movement_->GetCanJump(); }
    bool &GetIsGrounded() { return movement_->GetIsGrounded(); }
    bool GetIsFlying() const { return movement_->GetIsFlying(); }
    float GetVerticalVelocity() const { return movement_->GetVerticalVelocity(); }
    float GetVerticalAcceleration() const { return movement_->GetVerticalAcceleration(); }
    Direction &GetDirection() { return movement_->GetDirection(); }
    MoveDirection &GetMoveDirection() { return movement_->GetMoveDirection(); }

    // ─── ステータス（EnemyStatus） ───
    float GetHP() const { return status_->GetHP(); }
    float GetMaxHP() const { return status_->GetMaxHP(); }
    float &GetEnergy() { return status_->GetEnergy(); }
    float GetMaxEnergy() const { return status_->GetMaxEnergy(); }
    float GetEnergyRecoveryRate() const { return status_->GetEnergyRecoveryRate(); }
    bool IsGuarding() const { return status_->IsGuarding(); }
    /// <summary>ガード可能か（被弾で消費するエネルギーを支払えるか）</summary>
    bool CanGuard() const { return status_->CanGuard(); }

    // ─── 戦闘（EnemyCombat） ───
    /// <summary>前方攻撃判定コライダーを返す</summary>
    EnemyAttackCollider *GetAttackCollider() { return combat_->GetAttackCollider(); }
    /// <summary>現在の攻撃のダメージ量を返す（EnemyHandから参照）</summary>
    float GetCurrentAttackDamage() const { return combat_->GetCurrentAttackDamage(); }
    /// <summary>現在の攻撃のノックバック強度を返す（EnemyHandから参照）</summary>
    float GetCurrentAttackKnockback() const { return combat_->GetCurrentAttackKnockback(); }
    // コンボ状態Getter（BTノードから参照）
    int GetPunchComboLength() const { return combat_->GetPunchComboLength(); }
    bool IsPunchComboActive() const { return combat_->IsPunchComboActive(); }

    // ─── 本体（視界・位置・向き） ───
    bool &GetAlive() { return isAlive_; }
    bool GetIsLockOn() const { return isLockOn_; }
    Player *GetTarget() { return target_; }
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
    Hagine::Vector3 GetPosition() const { return transform_->translation_; }
    Hagine::Vector3 GetLocalPosition() const { return transform_->translation_; }

    // ConditionNode用
    Hagine::Vector3 GetWorldPosition() const { return transform_->translation_; }

    /// ===================================================
    /// Setter
    /// ===================================================

    // ─── 移動（EnemyMovement） ───
    void SetVelocity(const Hagine::Vector3 &vel) { movement_->SetVelocity(vel); }
    void SetMoveSpeed(float speed) { movement_->SetMoveSpeed(speed); }
    void SetStrafeDirection(int dir) { movement_->SetStrafeDirection(dir); }
    void SetVerticalVelocity(float velocity) { movement_->SetVerticalVelocity(velocity); }
    void SetVerticalAcceleration(float accel) { movement_->SetVerticalAcceleration(accel); }
    void SetIsGrounded(bool grounded) { movement_->SetIsGrounded(grounded); }
    void SetIsFlying(bool flying) { movement_->SetIsFlying(flying); }

    // ─── ステータス（EnemyStatus） ───
    void SetDamage(float damage) { status_->SetDamage(damage); }
    void SetGuarding(bool guarding) { status_->SetGuarding(guarding); }
    void SetEnergy(float energy) { status_->SetEnergy(energy); }
    void SetEnergyRecoveryRate(float rate) { status_->SetEnergyRecoveryRate(rate); }

    /// <summary>
    /// ノックバックを設定する
    /// PlayerAttackCollider::OnCollisionEnterから呼ばれる
    /// </summary>
    /// <param name="direction">ノックバック方向（正規化済みでなくてもよい）</param>
    /// <param name="power">ノックバック強度</param>
    void SetKnockback(const Hagine::Vector3 &direction, float power) { status_->SetKnockback(direction, power); }

    // ─── 戦闘（EnemyCombat） ───
    void SetComboAttack(bool flag) { combat_->SetComboAttack(flag); }

    // ─── 本体 ───
    void SetVp(Hagine::ViewProjection *vp);
    void SetTarget(Player *target) {
        target_ = target;
        // 前方攻撃判定コライダーにもプレイヤーを設定する
        if (combat_->GetAttackCollider()) {
            combat_->GetAttackCollider()->SetPlayer(target);
        }
    }
    void SetIsLockOn(bool lockOn) { isLockOn_ = lockOn; }
    void SetStart(bool flag) { started_ = flag; }
    void SetPause(bool flag) { isPause_ = flag; }
    void SetLocalPosition(const Hagine::Vector3 &pos) { transform_->translation_ = pos; }

    void SetBehaviorTree(std::shared_ptr<BTNode> rootNode) {
        rootNode_ = rootNode;
        if (rootNode_) {
            rootNode_->SetContext(this, target_);
        }
    }

    /// <summary>
    /// トレーニング用ダミーモードを設定する。
    /// 有効時はAI(ビヘイビアツリー)・自身の攻撃・弾を停止し、
    /// 被弾リアクション(点滅・ノックバック)と復活のみ行う。
    /// 呼び出し時点のワールド位置を復活位置として記録する。
    /// </summary>
    void SetDummy(bool enable);

    /// <summary>
    /// HP・状態・位置を初期化して復活させる(ダミー用)
    /// </summary>
    void Revive();

    /// ===================================================
    /// エネルギー・戦闘・移動の転送
    /// ===================================================
    bool ConsumeEnergy(float amount) { return status_->ConsumeEnergy(amount); }
    void RecoverEnergy() { status_->RecoverEnergy(); }

    void UpdateFrustumLockOn();
    void ReleaseLockOn() { isLockOn_ = false; }

    void MoveToTarget(const Hagine::Vector3 &targetPos) { movement_->MoveToTarget(targetPos); }
    void PerformAttack() { combat_->PerformAttack(); }
    void MoveStrafe() { movement_->MoveStrafe(); }
    void MoveRetreat() { movement_->MoveRetreat(); }
    void StopMovement() { movement_->StopMovement(); }
    void Move() { movement_->Move(); }
    void DirectionUpdate() { movement_->DirectionUpdate(); }
    void Shot() { combat_->Shot(); }
    void ShotWithDirection(const Hagine::Vector3 &direction, bool forceHoming = false) {
        combat_->ShotWithDirection(direction, forceHoming);
    }

    /// ===================================================
    /// 大技（チャージ攻撃・必殺技）の転送
    /// BTノード(EnemyChargeAttackNode / EnemyUltimateNode)から駆動する
    /// ===================================================
    void StartChargeAura() { combat_->StartChargeAura(); }
    void StopChargeAura() { combat_->StopChargeAura(); }
    void FireChargeBlast() { combat_->FireChargeBlast(); }
    void ActivateBeam() { combat_->ActivateBeam(); }
    void DeactivateBeam() { combat_->DeactivateBeam(); }
    bool IsBeamActive() const { return combat_->IsBeamActive(); }
    void StartBeamStaging() { combat_->StartBeamStaging(); }
    void CancelBeamStaging() { combat_->CancelBeamStaging(); }
    bool IsBeamStaging() const { return combat_->IsBeamStaging(); }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    void Save();
    void Load();

    /// ===================================================
    /// private variants
    /// ===================================================

    // 初期化定数
    static constexpr float kAlphaOpaque = 1.0f;

    // 色関連定数
    static constexpr float kColorRed = 1.0f;
    static constexpr float kColorZero = 0.0f;
    static constexpr float kColorOpaque = 1.0f;

    // 距離・閾値定数
    static constexpr float kMinRotationDistance = 0.001f;
    static constexpr float kGroundLevel = 0.0f;
    static constexpr float kVelocityZero = 0.0f;
    static constexpr float kMinHP = 0.0f;

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

    // ─── パーツ ───
    std::unique_ptr<EnemyMovement> movement_; ///< 移動・回転・接地
    std::unique_ptr<EnemyCombat> combat_;     ///< 射撃・コンボ・大技
    std::unique_ptr<EnemyStatus> status_;     ///< HP・エネルギー・被ダメージ・ガード
    std::unique_ptr<EnemyVisual> visual_;     ///< アニメーション・被弾/ガード色

    Player *target_ = nullptr; ///< ターゲットプレイヤー

    std::shared_ptr<BTNode> rootNode_ = nullptr; ///< ビヘイビアツリーのルートノード

    std::unique_ptr<Hagine::ParticleEmitter> hitEmitter_; ///< ヒットエミッター

    Hagine::OBBCollider *enemyCollider_ = nullptr;      ///< 敵コライダー
    Hagine::AABBCollider *enemyWallCollider_ = nullptr; ///< 壁用コライダー

    bool started_ = false;   ///< 開始フラグ
    bool isPause_ = false;   ///< ポーズフラグ
    bool isStop_ = false;    ///< 停止中フラグ
    bool dummyMode_ = false; ///< トレーニング用ダミーモード(AI停止・復活のみ)
    Hagine::Vector3 spawnPosition_{}; ///< ダミー復活時に戻る初期位置

    // 視錐台ロックオン関連
    static constexpr float kDefaultFrustumRange = 150.0f;
    static constexpr float kDefaultFrustumHalfFovH = 40.0f * (3.14159265f / 180.0f);
    static constexpr float kDefaultFrustumHalfFovV = 30.0f * (3.14159265f / 180.0f);

    bool isLockOn_ = false;                                  ///< ロックオンフラグ
    float frustumLockOnRange_ = kDefaultFrustumRange;       ///< 視錐台範囲
    float frustumLockOnHalfFovH_ = kDefaultFrustumHalfFovH; ///< 視錐台水平半角
    float frustumLockOnHalfFovV_ = kDefaultFrustumHalfFovV; ///< 視錐台垂直半角
    bool drawFrustumDebug_ = false;                         ///< 視錐台デバッグ表示フラグ
};

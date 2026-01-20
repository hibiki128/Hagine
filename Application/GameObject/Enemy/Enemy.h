#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"
#include <Application/GameObject/Player/Player.h>
#include <application/GameObject/Player/PlayerData.h>
#include <Application/GameObject/BehaviorTree/Node/BehaviorNode.h>

/// <summary>
/// 敵のゲームオブジェクトクラス
/// ビヘイビアツリーに基づいて行動し、プレイヤーとの相互作用を管理する
/// </summary>
class Enemy : public BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    Enemy();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~Enemy() override;

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
    /// デバッグ処理
    /// </summary>
    void Debug();

    /// <summary>
    /// 衝突判定時の処理
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollisionEnter(ColliderBase *collider);

    /// <summary>
    /// Getter
    /// </summary>
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
    float GetVelocityMagnitude() const;
    float &GetFallSpeed() { return fallSpeed_; }
    float &GetMoveSpeed() { return moveSpeed_; }
    float &GetJumpSpeed() { return jumpSpeed_; }
    float &GetMaxSpeed() { return maxSpeed_; }
    float &GetAccelRate() { return accelRate_; }
    float GetHP() const { return HP_; }
    float GetMaxHP() const { return maxHP_; }
    bool &GetCanJump() { return canJump_; }
    bool &GetAlive() { return isAlive_; }
    bool &GetIsGrounded() { return isGrounded_; }
    bool IsGuarding() const { return isGuarding_; }
    Player *GetTarget() { return target_; }
    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }

    /// <summary>
    /// Setter
    /// </summary>
    void SetDamage(float damage) { damage_ = damage; }
    void SetVp(ViewProjection *vp);
    void SetTarget(Player *target) { target_ = target; }
    void SetGuarding(bool guarding) { isGuarding_ = guarding; }
    void SetStart(bool flag) { started_ = flag; }
    void SetPause(bool flag) { isPause_ = flag; }
    void SetDrawShadow(bool flag) { drawShadow_ = flag; }
    void SetVelocity(const Vector3 &vel) { velocity_ = vel; }

    void SetBehaviorTree(std::shared_ptr<BTNode> rootNode) {
        rootNode_ = rootNode;
        // ツリーに自分とターゲットを教える
        if (rootNode_) {
            rootNode_->SetContext(this, target_);
        }
    }

    void MoveToTarget(const Vector3 &targetPos);
    void PerformAttack();

    // ConditionNode用に位置取得が必要（BaseObjectにあればOK）
    Vector3 GetWorldPosition() const { return transform_->translation_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 設定を保存
    /// </summary>
    void Save();

    /// <summary>
    /// 設定を読み込み
    /// </summary>
    void Load();

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
    /// <returns>計算されたDirection</returns>
    Direction CalculateDirectionFromRotation();

    /// <summary>
    /// Direction値を文字列で取得
    /// </summary>
    /// <param name="dir">方向の値</param>
    /// <returns>方向の名前文字列</returns>
    const char *GetDirectionName(Direction dir);

  private:
    /// ===================================================
    /// private varians
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

    // ビヘイビアツリー重み定数
    static constexpr float kCloseApproachWeight = 1.0f;
    static constexpr float kStrafeWeight = 1.5f;
    static constexpr float kRetreatWeight = 1.0f;
    static constexpr float kGuardWeight = 1.2f;

    Direction dir_;
    MoveDirection moveDir_;

    Vector3 velocity_{};
    Vector3 acceleration_{};
    Player *target_ = nullptr;

    float HP_ = 100.0f;
    float maxHP_ = 100.0f;
    float damage_ = 0.0f;
    bool isGuarding_ = false; // ガード状態

    float moveSpeed_ = 0.0f;
    float fallSpeed_ = 0.0f;
    float jumpSpeed_ = 0.0f;
    float maxSpeed_ = 0.0f;
    float accelRate_ = 0.0f;

    bool canJump_ = false;
    bool isLockOn_ = false;
    bool isGrounded_ = true;
    bool isStop_ = false;
    bool started_ = false;
    bool isPause_ = false;
    bool drawShadow_ = true;

    std::unique_ptr<DataHandler> data_;
    std::unique_ptr<BaseObject> shadow_;
    std::unique_ptr<ParticleEmitter> hitEmitter_;
    std::unique_ptr<Shake> chargeShake_;
    std::shared_ptr<BTNode> rootNode_ = nullptr;

    bool isDamageReact_ = false;       // リアクション中かどうか
    float damageReactTimer_ = 0.0f;    // 経過時間
    float damageReactDuration_ = 0.5f; // 少し短めの時間
    EasingData<float> tiltEase_;       // 回転角イージング
    Quaternion baseRotation_;          // 通常時の向き
    Quaternion tiltRotation_;          // のけぞり用の回転

    OBBCollider *enemyCollider_ = nullptr;
};
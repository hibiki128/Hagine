#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"
#include <Application/GameObject/Player/Player.h>
#include <application/GameObject/Player/PlayerData.h>

class BehaviorNode;
class BehaviorTreeEditor;

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
    ~Enemy();

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
    void OnCollisionEnter([[maybe_unused]] Collider *other) override;

    /// <summary>
    /// ビヘイビアツリーを初期化
    /// </summary>
    void InitializeBehaviorTree();

    /// <summary>
    /// ビヘイビアツリーを実行
    /// </summary>
    void ExecuteBehaviorTree(float deltaTime);

    /// <summary>
    /// ビヘイビアツリーエディターを描画
    /// </summary>
    void DrawBehaviorTreeEditor();

    /// <summary>
    /// Getter
    /// </summary>
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
    float GetHP() const { return HP_; }
    float GetMaxHP() const { return maxHP_; }
    bool &GetCanJump() { return canJump_; }
    bool &GetAlive() { return isAlive_; }
    bool &GetIsGrounded() { return isGrounded_; }
    Player *GetTarget() { return target_; }
    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }
    bool IsGuarding() const { return isGuarding_; }

    /// <summary>
    /// Setter
    /// </summary>
    void SetDamage(int damage) { damage_ = damage; }
    void SetVp(ViewProjection *vp);
    void SetTarget(Player *target) { target_ = target; }
    void SetGuarding(bool guarding) { isGuarding_ = guarding; }
    void SetStart(bool flag) { started_ = flag; }
    void SetDrawShadow(bool flag) { drawShadow_ = flag; }

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

    Direction dir_;
    MoveDirection moveDir_;

    Vector3 velocity_{};
    Vector3 acceleration_{};
    Player *target_ = nullptr;

    float HP_ = 1.0f;
    float maxHP_ = 100.0f;
    int damage_ = 0;
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
    bool drawShadow_ = true;

    std::unique_ptr<DataHandler> data_;
    std::unique_ptr<BaseObject> shadow_;
    std::unique_ptr<ParticleEmitter> emitter_;
    std::unique_ptr<Shake> chageShake_;

    // ビヘイビアツリー関連
#ifdef _DEBUG
    std::unique_ptr<BehaviorTreeEditor> behaviorTreeEditor_;
#endif
    std::unique_ptr<BehaviorNode> behaviorTreeRoot_;
};
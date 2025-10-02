#pragma once
#include"Object/Base/BaseObject.h"
#include <application/GameObject/Player/PlayerData.h>
#include"Particle/ParticleEmitter.h"
#include"Application/Utility/Shake/Shake.h"
class Enemy: public BaseObject {
  public:
    /// ==================================================================
    /// public methods
    /// ==================================================================

    Enemy();
    ~Enemy();
    void Init(const std::string objectName) override;
    void Update() override;
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;
    void DrawParticle(const ViewProjection &viewProjection);
    void Debug();
    
    void OnCollisionEnter([[maybe_unused]] Collider *other) override;

    /// <summary>
    /// Getter
    /// </summary>

    Vector3 &GetAcceleration() { return acceleration_; }
    Vector3 &GetVelocity() { return velocity_; }
    Vector3 GetMovementDirection() const;
    Vector3 GetForward() const;  // 敵の前方向
    Vector3 GetBackward() const; // 敵の後方向
    Vector3 GetRight() const;    // 敵の右方向
    Vector3 GetLeft() const;     // 敵の左方向
    Vector3 GetUp() const;       // 敵の上方向
    Vector3 GetDown() const;     // 敵の下方向

    // 敵の周囲の位置を取得
    Vector3 GetPositionBehind(float distance = 3.0f) const; // 敵の後ろの位置
    Vector3 GetPositionFront(float distance = 3.0f) const;  // 敵の前の位置
    Vector3 GetPositionRight(float distance = 3.0f) const;  // 敵の右の位置
    Vector3 GetPositionLeft(float distance = 3.0f) const;   // 敵の左の位置
    Vector3 GetPositionAbove(float distance = 3.0f) const;  // 敵の上の位置
    Vector3 GetPositionBelow(float distance = 3.0f) const;  // 敵の下の位置

    float GetVelocityMagnitude() const;
    float &GetFallSpeed() { return fallSpeed_; }
    float &GetMoveSpeed() { return moveSpeed_; }
    float &GetJumpSpeed() { return jumpSpeed_; }
    float &GetMaxSpeed() { return maxSpeed_; }
    float &GetAccelRate() { return accelRate_; }
    int GetHP() const { return HP_; }
    int GetMaxHP() const { return maxHP_; }

    bool &GetCanJump() { return canJump_; }
    bool &GetAlive() { return isAlive_; }
    bool &GetIsGrounded() { return isGrounded_; }

    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }

    void SetDamage(int damage) { damage_ = damage; }
    void SetVp(ViewProjection *vp);

  private:
    /// ==================================================================
    /// private methods
    /// ==================================================================

    void Save();
    void Load();

    void UpdateShadowScale();
    void RotateUpdate();
    void CollisionGround();

    Direction CalculateDirectionFromRotation();

    const char *GetDirectionName(Direction dir);

  private:
    /// ==================================================================
    /// private varians
    /// ==================================================================

    Direction dir_;
    MoveDirection moveDir_;

    Vector3 velocity_{};
    Vector3 acceleration_{};

    int HP_ = 100;
    int maxHP_ = 100;
    int damage_ = 0;

    float moveSpeed_ = 0.0f;
    float fallSpeed_ = 0.0f;
    float jumpSpeed_ = 0.0f;
    float maxSpeed_ = 0.0f;  
    float accelRate_ = 0.0f;

    bool canJump_ = false;
    bool isAlive_ = true;
    bool isLockOn_ = false;
    bool isGrounded_ = true; 

    std::unique_ptr<DataHandler> data_;
    std::unique_ptr<BaseObject> shadow_;
    std::unique_ptr<ParticleEmitter> emitter_;
    std::unique_ptr<Shake> chageShake_;
};

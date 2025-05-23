#pragma once
#include "Data/DataHandler.h"
#include "PlayerData.h"
#include "State/PlayerBaseState.h"
#include "application/Base/BaseObject.h"
class FollowCamera;
class Enemy;
class Player : public BaseObject {
  public:
    /// ==================================================================
    /// public methods
    /// ==================================================================

    Player();
    ~Player();
    void Init(const std::string objectName) override;
    void Update() override;
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;
    void ChangeState(const std::string &stateName);
    void DirectionUpdate();
    void Debug();

    void Move();

    /// <summary>
    /// Getter
    /// </summary>

    FollowCamera *GetCamera() { return FollowCamera_; }
    Enemy *GetEnemy() { return enemy_; }

    Vector3 &GetAcceleration() { return acceleration_; }
    Vector3 &GetVelocity() { return velocity_; }
    Vector3 GetMovementDirection() const;

    float GetVelocityMagnitude() const;
    float &GetFallSpeed() { return fallSpeed_; }
    float &GetMoveSpeed() { return moveSpeed_; }
    float &GetJumpSpeed() { return jumpSpeed_; }
    float &GetMaxSpeed() { return maxSpeed_; }
    float &GetAccelRate() { return accelRate_; }
    float &GetDt() { return dt_; }

    bool &GetCanJump() { return canJump_; }
    bool &GetAlive() { return isAlive_; }
    bool &GetIsGrounded() { return isGrounded_; }
    bool &GetIsLockOn() { return isLockOn_; }

    void SetCamera(FollowCamera *camera) { FollowCamera_ = camera; }
    void SetEnemy(Enemy *enemy) { enemy_ = enemy; }

    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }

  private:
    /// ==================================================================
    /// private methods
    /// ==================================================================

    void Save();
    void Load();

    void RotateUpdate();
    void CollisionGround();

    Direction CalculateDirectionFromRotation();

    float NormalizeAngle(float angle);

    // プレイヤーが向いている方向を文字列に変換して表示
    const char *GetDirectionName(Direction dir);

    void DefaultMovement();

  private:
    /// ==================================================================
    /// private varians
    /// ==================================================================

    FollowCamera *FollowCamera_;
    Enemy *enemy_ = nullptr;

    Direction dir_;
    MoveDirection moveDir_;

    Vector3 velocity_{};
    Vector3 acceleration_{};

    float moveSpeed_ = 0.0f;
    float fallSpeed_ = 0.0f;
    float jumpSpeed_ = 0.0f;
    float maxSpeed_ = 0.0f;  // 追加: 最大速度
    float accelRate_ = 0.0f; // 追加: 加速率
    float dt_;

    bool canJump_ = false;
    bool isAlive_ = true;
    bool isLockOn_ = false;
    bool isGrounded_ = true; // 追加: 接地判定

    std::unordered_map<std::string, std::unique_ptr<PlayerBaseState>> states_;
    PlayerBaseState *currentState_ = nullptr;

    std::unique_ptr<DataHandler> data_;
    std::unique_ptr<BaseObject> shadow_;
};
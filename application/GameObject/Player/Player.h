#pragma once
#include "Data/DataHandler.h"
#include "PlayerData.h"
#include "State/PlayerBaseState.h"
#include "application/Base/BaseObject.h"

class FollowCamera;
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

    /// <summary>
    /// Getter
    /// </summary>

    Vector3 &GetAcceleration() { return acceleration_; }
    Vector3 &GetVelocity() { return velocity_; }

    float &GetFallSpeed() { return fallSpeed_; }
    float &GetMoveSpeed() { return moveSpeed_; }
    float &GetJumpSpeed() { return jumpSpeed_; }
    float &GetMaxSpeed() { return maxSpeed_; }   // 追加: 最大速度
    float &GetAccelRate() { return accelRate_; } // 追加: 加速率

    bool &GetCanJump() { return canJump_; }
    bool &GetAlive() { return isAlive_; }
    bool &GetIsGrounded() { return isGrounded_; } // 追加: 接地判定

    void SetCamera(FollowCamera *camera) { FollowCamera_ = camera; }

    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }

  private:
    /// ==================================================================
    /// private methods
    /// ==================================================================

    void Save();
    void Load();

  private:
    /// ==================================================================
    /// private varians
    /// ==================================================================

    FollowCamera *FollowCamera_;

    Direction dir_;
    MoveDirection moveDir_;

    Vector3 velocity_{};
    Vector3 acceleration_{};

    float moveSpeed_ = 0.0f;
    float fallSpeed_ = 0.0f;
    float jumpSpeed_ = 0.0f;
    float maxSpeed_ = 0.0f;  // 追加: 最大速度
    float accelRate_ = 0.0f; // 追加: 加速率

    bool canJump_ = false;
    bool isAlive_ = true;
    bool isLockOn_ = false;
    bool isGrounded_ = true; // 追加: 接地判定

    std::unordered_map<std::string, std::unique_ptr<PlayerBaseState>> states_;
    PlayerBaseState *currentState_ = nullptr;

    std::unique_ptr<DataHandler> data_;
};
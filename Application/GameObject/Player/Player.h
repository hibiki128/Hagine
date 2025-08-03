#pragma once
#include "Bullet/PlayerBullet.h"
#include "Data/DataHandler.h"
#include "Hand/PlayerHand.h"
#include "Object/Base/BaseObject.h"
#include "PlayerData.h"
#include "State/Base/PlayerBaseState.h"
#include <application/Utility/ComboSystem/ComboSystem.h>

class ChageShot;
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
    void DrawParticle(const ViewProjection &viewProjection);
    void ChangeState(const std::string &stateName);
    void DirectionUpdate();
    void Debug();

    void ChangeRush();

    float CalculateShortestRotation(float from, float to);

    void Move();

    /// <summary>
    /// Getter
    /// </summary>

    FollowCamera *GetCamera() { return FollowCamera_; }
    Enemy *GetEnemy() { return enemy_; }

    Vector3 &GetAcceleration() { return acceleration_; }
    Vector3 &GetVelocity() { return velocity_; }
    Vector3 GetMovementDirection() const;
    Vector3 GetForward() const;  // プレイヤーの前方向
    Vector3 GetBackward() const; // プレイヤーの後方向
    Vector3 GetRight() const;    // プレイヤーの右方向
    Vector3 GetLeft() const;     // プレイヤーの左方向
    Vector3 GetUp() const;       // プレイヤーの上方向
    Vector3 GetDown() const;     // プレイヤーの下方向

    // プレイヤーの周囲の位置を取得
    Vector3 GetPositionBehind(float distance = 3.0f) const; // プレイヤーの後ろの位置
    Vector3 GetPositionFront(float distance = 3.0f) const;  // プレイヤーの前の位置
    Vector3 GetPositionRight(float distance = 3.0f) const;  // プレイヤーの右の位置
    Vector3 GetPositionLeft(float distance = 3.0f) const;   // プレイヤーの左の位置
    Vector3 GetPositionAbove(float distance = 3.0f) const;  // プレイヤーの上の位置
    Vector3 GetPositionBelow(float distance = 3.0f) const;  // プレイヤーの下の位置

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

    PlayerHand *GetRightHand() { return rightHand_ptr_; }
    PlayerHand *GetLeftHand() { return leftHand_ptr_; }

    void SetCamera(FollowCamera *camera) { FollowCamera_ = camera; }
    void SetEnemy(Enemy *enemy) { 
        enemy_ = enemy;
        leftHand_ptr_->SetEnemy(enemy);
        rightHand_ptr_->SetEnemy(enemy);
    }
    void ResetControlCount() {
        lControlInputTime_ = 0.0f;
        lControlInputCount_ = 0;
    }

    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }
    std::string GetCurrentStateName() const;

    std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() { return bullets_; }

  private:
    /// ==================================================================
    /// private methods
    /// ==================================================================

    void Save();
    void Load();
    void ComboUpdate();
    void Shot();

    void UpdateShadowScale();
    void RotateUpdate();
    void CollisionGround();

    Direction CalculateDirectionFromRotation();

    float NormalizeAngle(float angle);

    // プレイヤーが向いている方向を文字列に変換して表示
    const char *GetDirectionName(Direction dir);

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
    float maxSpeed_ = 0.0f;
    float accelRate_ = 0.0f;
    float dt_;

    float lControlInputTime_ = 0.0f;
    int lControlInputCount_ = 0;
    const float INPUT_RESET_TIME = 0.3f;

    float currentFov_ = 45.0f;
    float targetFov_ = 45.0f;
    float fovLerpSpeed_ = 5.0f;

    float B_acce_ = 0.0f;
    float B_speed_ = 0.0f;

    bool canJump_ = false;
    bool isAlive_ = true;
    bool isLockOn_ = false;
    bool isGrounded_ = true;
    bool isDashing_ = false;

    ComboSystem punchCombo_;
    bool comboInitialized_ = false;

    // ステート
    std::unordered_map<std::string, std::unique_ptr<PlayerBaseState>> states_;
    PlayerBaseState *currentState_ = nullptr;

    // データ
    std::unique_ptr<DataHandler> data_;

    // 影
    std::unique_ptr<BaseObject> shadow_;

    // 弾
    std::vector<std::unique_ptr<PlayerBullet>> bullets_;
    std::unique_ptr<ChageShot> chageShot_;

    // 両手
    std::unique_ptr<PlayerHand> leftHand_;
    std::unique_ptr<PlayerHand> rightHand_;
    PlayerHand *leftHand_ptr_;
    PlayerHand *rightHand_ptr_;
};
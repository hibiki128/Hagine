#pragma once
#include"Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "Input.h"
#include"Application/GameObject/Enemy/Enemy.h"

class MoveToTargetNode : public BehaviorNode {
  public:
    MoveToTargetNode(float speed) : moveSpeed_(speed) {}

    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "MoveToTarget"; }
    float GetSpeed() const { return moveSpeed_; }
    void SetSpeed(float speed) { moveSpeed_ = speed; }
  private:
    float moveSpeed_;
};

class JumpNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "Jump"; }
};

class FlyIdleNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "FlyIdle"; }
};

class ApplyGravityNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "ApplyGravity"; }
};

class FlyToTargetNode : public BehaviorNode {
  public:
    FlyToTargetNode(float speed) : moveSpeed_(speed) {}
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "FlyToTarget"; }
    float GetSpeed() const { return moveSpeed_; }
    void SetSpeed(float speed) { moveSpeed_ = speed; }
  private:
    float moveSpeed_;
};

class RushAttackNode : public BehaviorNode {
  public:
    RushAttackNode(float speed, float minDistance)
        : rushSpeed_(speed), minDistance_(minDistance) {}
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "RushAttack"; }
    float GetRushSpeed() const { return rushSpeed_; }
    void SetRushSpeed(float speed) { rushSpeed_ = speed; }
    float GetMinDistance() const { return minDistance_; }
    void SetMinDistance(float dist) { minDistance_ = dist; }
  private:
    float rushSpeed_;
    float minDistance_;
    Vector3 rushDirection_ = {0.0f, 0.0f, 0.0f};
    float rushElapsedTime_ = 0.0f;
};
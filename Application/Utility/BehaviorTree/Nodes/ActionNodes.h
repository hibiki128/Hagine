#pragma once
#include"Application/Utility/BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "Input.h"
#include"Application/GameObject/Enemy/Enemy.h"

class MoveToTargetNode : public BehaviorNode {
  public:
    MoveToTargetNode(float speed) : moveSpeed_(speed) {}

    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "MoveToTarget"; }

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
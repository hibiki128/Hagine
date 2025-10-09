#pragma once
#include"Application/Utility/BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "application/GameObject/Enemy/Enemy.h"

class IsGroundedNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "IsGrounded"; }
};

class IsInAirNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "IsInAir"; }
};

class HasTargetNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "HasTarget"; }
};

class DistanceToTargetNode : public BehaviorNode {
  public:
    DistanceToTargetNode(float minDist, float maxDist)
        : minDistance_(minDist), maxDistance_(maxDist) {}

    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    const char *GetNodeName() const override { return "DistanceCheck"; }

  private:
    float minDistance_;
    float maxDistance_;
};
#pragma once
#include"Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"
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
    float GetMinDistance() const { return minDistance_; }
    float GetMaxDistance() const { return maxDistance_; }
    void SetDistances(float minDist, float maxDist) {
        minDistance_ = minDist;
        maxDistance_ = maxDist;
    }
  private:
    float minDistance_;
    float maxDistance_;
};

class IsPlayerAirborneNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "IsPlayerAirborne"; }
};

class DistanceThresholdNode : public BehaviorNode {
  public:
    DistanceThresholdNode(float threshold) : threshold_(threshold) {}
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "DistanceThreshold"; }

  private:
    float threshold_;
};
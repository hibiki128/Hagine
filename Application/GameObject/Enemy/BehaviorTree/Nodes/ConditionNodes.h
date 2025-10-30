#pragma once
#include "../BehaviorNode/BehaviorNode.h"

/// <summary>
/// 距離判定ノード
/// </summary>
class DistanceCheckNode : public BehaviorNode {
  public:
    DistanceCheckNode(float minDist = 0.0f, float maxDist = 10.0f);
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
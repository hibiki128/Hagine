#pragma once
#include <memory>
#include <string>
#include <vector>

class Enemy;

enum class NodeStatus {
    Running,
    Success,
    Failure
};

class BehaviorNode {
  public:
    virtual ~BehaviorNode() = default;
    virtual NodeStatus Execute(Enemy &enemy, float deltaTime) = 0;
    virtual const char *GetNodeName() const = 0;

    int nodeId = 0;
};
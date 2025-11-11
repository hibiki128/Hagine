#include "ConditionNodes.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include"../Editor/BehaviorTreeEditor.h"

DistanceCheckNode::DistanceCheckNode(float minDist, float maxDist)
    : minDistance_(minDist), maxDistance_(maxDist) {}

NodeStatus DistanceCheckNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (!enemy.GetTarget()) {
        return NodeStatus::Failure;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    float distance = toTarget.Length();

    if (distance >= minDistance_ && distance <= maxDistance_) {
        return NodeStatus::Success;
    }

    return NodeStatus::Failure;
}
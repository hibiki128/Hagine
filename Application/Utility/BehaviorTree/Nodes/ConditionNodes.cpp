#include "ConditionNodes.h"

NodeStatus IsGroundedNode::Execute(Enemy &enemy, float deltaTime) {
    return enemy.GetIsGrounded() ? NodeStatus::Success : NodeStatus::Failure;
}

NodeStatus IsInAirNode::Execute(Enemy &enemy, float deltaTime) {
    return !enemy.GetIsGrounded() ? NodeStatus::Success : NodeStatus::Failure;
}

NodeStatus HasTargetNode::Execute(Enemy &enemy, float deltaTime) {
    return enemy.GetTarget() ? NodeStatus::Success : NodeStatus::Failure;
}

NodeStatus DistanceToTargetNode::Execute(Enemy &enemy, float deltaTime) {
    if (!enemy.GetTarget()) {
        return NodeStatus::Failure;
    }

    float distance = (enemy.GetTarget()->GetWorldPosition() -
                      enemy.GetWorldPosition())
                         .Length();

    if (distance >= minDistance_ && distance <= maxDistance_) {
        return NodeStatus::Success;
    }

    return NodeStatus::Failure;
}

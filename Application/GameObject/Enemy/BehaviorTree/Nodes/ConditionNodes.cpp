#include "ConditionNodes.h"
#include <Application/GameObject/Player/Player.h>

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

NodeStatus IsPlayerAirborneNode::Execute(Enemy &enemy, float deltaTime) {
    Player *target = enemy.GetTarget();
    if (!target) {
        return NodeStatus::Failure;
    }

    // プレイヤーが浮遊状態かどうかを判定
    std::string stateName = target->GetCurrentStateName();
    if (stateName == "FlyIdle" || stateName == "FlyMove" || stateName == "Rush" || stateName == "Air") {
        return NodeStatus::Success;
    }

    return NodeStatus::Failure;
}

NodeStatus DistanceThresholdNode::Execute(Enemy &enemy, float deltaTime) {
    Player *target = enemy.GetTarget();
    if (!target) {
        return NodeStatus::Failure;
    }

    float distance = (target->GetWorldPosition() - enemy.GetWorldPosition()).Length();

    if (distance >= threshold_) {
        return NodeStatus::Success;
    }

    return NodeStatus::Failure;
}
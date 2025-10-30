#include "ActionNodes.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include <Frame.h>
#include"../Editor/BehaviorTreeEditor.h"


ApproachNode::ApproachNode(MoveSpeedType speedType) : speedType_(speedType) {
    speed_ = (speedType == MoveSpeedType::Fast) ? 15.0f : 8.0f;
}

NodeStatus ApproachNode::Execute(Enemy &enemy, float deltaTime) {
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

    if (distance > 0.1f) {
        Vector3 direction = toTarget.Normalize();
        enemy.GetVelocity().x = direction.x * speed_;
        enemy.GetVelocity().z = direction.z * speed_;
    }

    return NodeStatus::Running;
}

// StopNode
NodeStatus StopNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    enemy.GetVelocity().x = 0.0f;
    enemy.GetVelocity().z = 0.0f;
    return NodeStatus::Success;
}

// CloseApproachNode
NodeStatus CloseApproachNode::Execute(Enemy &enemy, float deltaTime) {
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

    if (distance <= targetDistance_) {
        enemy.GetVelocity().x = 0.0f;
        enemy.GetVelocity().z = 0.0f;
        return NodeStatus::Success;
    }

    Vector3 direction = toTarget.Normalize();
    enemy.GetVelocity().x = direction.x * speed_;
    enemy.GetVelocity().z = direction.z * speed_;

    return NodeStatus::Running;
}

// StrafeNode
NodeStatus StrafeNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (!enemy.GetTarget()) {
        return NodeStatus::Failure;
    }

    strafeTime_ += deltaTime;

    if (strafeTime_ >= maxStrafeTime_) {
        strafeTime_ = 0.0f;
        strafeDirection_ *= -1;
        return NodeStatus::Success;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    Vector3 forward = toTarget.Normalize();
    Vector3 right = (Vector3(0, 1, 0).Cross(forward)).Normalize();

    enemy.GetVelocity().x = right.x * speed_ * strafeDirection_;
    enemy.GetVelocity().z = right.z * speed_ * strafeDirection_;

    return NodeStatus::Running;
}

// RetreatNode
NodeStatus RetreatNode::Execute(Enemy &enemy, float deltaTime) {
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

    if (distance >= retreatDistance_) {
        enemy.GetVelocity().x = 0.0f;
        enemy.GetVelocity().z = 0.0f;
        return NodeStatus::Success;
    }

    Vector3 direction = toTarget.Normalize();
    enemy.GetVelocity().x = -direction.x * speed_;
    enemy.GetVelocity().z = -direction.z * speed_;

    return NodeStatus::Running;
}
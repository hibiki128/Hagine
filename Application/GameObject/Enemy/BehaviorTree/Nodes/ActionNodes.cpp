#include "ActionNodes.h"

NodeStatus MoveToTargetNode::Execute(Enemy &enemy, float deltaTime) {
    if (!enemy.GetTarget()) {
        return NodeStatus::Failure;
    }

    Vector3 toTarget = enemy.GetTarget()->GetWorldPosition() - enemy.GetWorldPosition();
    float distance = toTarget.Length();

    if (distance < 2.0f) {
        return NodeStatus::Success;
    }

    toTarget = toTarget.Normalize();

    enemy.GetVelocity().x = toTarget.x * moveSpeed_;
    enemy.GetVelocity().z = toTarget.z * moveSpeed_;

    // 回転処理
    Vector3 forward = toTarget;
    Vector3 worldUp = {0.0f, 1.0f, 0.0f};
    Vector3 right;
    if (std::abs(forward.Dot(worldUp)) > 0.999f) {
        right = {1.0f, 0.0f, 0.0f};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }
    Vector3 up = (forward.Cross(right)).Normalize();

    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    enemy.GetWorldTransform()->quateRotation_ =
        Quaternion::Slerp(enemy.GetWorldTransform()->quateRotation_, targetRot, 5.0f * deltaTime);

    return NodeStatus::Running;
}

NodeStatus JumpNode::Execute(Enemy &enemy, float deltaTime) {
    if (!enemy.GetIsGrounded()) {
        return NodeStatus::Failure;
    }

    enemy.GetVelocity().y = enemy.GetJumpSpeed();
    enemy.GetIsGrounded() = false;

    return NodeStatus::Success;
}

NodeStatus FlyIdleNode::Execute(Enemy &enemy, float deltaTime) {
    const float damping = 0.75f;

    enemy.GetVelocity().x *= damping;
    enemy.GetVelocity().z *= damping;
    enemy.GetVelocity().y *= damping;

    if (std::abs(enemy.GetVelocity().x) < 0.01f)
        enemy.GetVelocity().x = 0.0f;
    if (std::abs(enemy.GetVelocity().z) < 0.01f)
        enemy.GetVelocity().z = 0.0f;
    if (std::abs(enemy.GetVelocity().y) < 0.01f)
        enemy.GetVelocity().y = 0.0f;

    return NodeStatus::Success;
}

NodeStatus ApplyGravityNode::Execute(Enemy &enemy, float deltaTime) {
    if (!enemy.GetIsGrounded()) {
        enemy.GetVelocity().y += enemy.GetFallSpeed() * deltaTime;

        if (enemy.GetVelocity().y < -40.0f) {
            enemy.GetVelocity().y = -40.0f;
        }
    }

    return NodeStatus::Success;
}

NodeStatus FlyToTargetNode::Execute(Enemy &enemy, float deltaTime) {
    if (!enemy.GetTarget()) {
        return NodeStatus::Failure;
    }

    Vector3 toTarget = enemy.GetTarget()->GetWorldPosition() - enemy.GetWorldPosition();
    float distance = toTarget.Length();

    if (distance < 2.0f) {
        enemy.GetVelocity() = {0.0f, 0.0f, 0.0f};
        return NodeStatus::Success;
    }

    toTarget = toTarget.Normalize();
    enemy.GetVelocity() = toTarget * moveSpeed_;

    // 回転処理
    Vector3 forward = toTarget;
    Vector3 worldUp = {0.0f, 1.0f, 0.0f};
    Vector3 right;
    if (std::abs(forward.Dot(worldUp)) > 0.999f) {
        right = {1.0f, 0.0f, 0.0f};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }
    Vector3 up = (forward.Cross(right)).Normalize();

    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    enemy.GetWorldTransform()->quateRotation_ =
        Quaternion::Slerp(enemy.GetWorldTransform()->quateRotation_, targetRot, 5.0f * deltaTime);

    return NodeStatus::Running;
}

NodeStatus RushAttackNode::Execute(Enemy &enemy, float deltaTime) {
    if (!enemy.GetTarget()) {
        return NodeStatus::Failure;
    }

    Player *target = dynamic_cast<Player *>(enemy.GetTarget());
    if (!target) {
        return NodeStatus::Failure;
    }

    Vector3 toTarget = target->GetWorldPosition() - enemy.GetWorldPosition();
    float distance = toTarget.Length();

    if (distance < minDistance_) {
        enemy.GetVelocity() = {0.0f, 0.0f, 0.0f};
        rushElapsedTime_ = 0.0f;
        return NodeStatus::Success;
    }

    if (rushElapsedTime_ == 0.0f) {
        rushDirection_ = toTarget.Normalize();
    }

    rushElapsedTime_ += deltaTime;
    enemy.GetVelocity() = rushDirection_ * rushSpeed_;

    // 回転処理
    Vector3 forward = rushDirection_;
    Vector3 worldUp = {0.0f, 1.0f, 0.0f};
    Vector3 right;
    if (std::abs(forward.Dot(worldUp)) > 0.999f) {
        right = {1.0f, 0.0f, 0.0f};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }
    Vector3 up = (forward.Cross(right)).Normalize();

    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    enemy.GetWorldTransform()->quateRotation_ =
        Quaternion::Slerp(enemy.GetWorldTransform()->quateRotation_, targetRot, 5.0f * deltaTime);

    return NodeStatus::Running;
}
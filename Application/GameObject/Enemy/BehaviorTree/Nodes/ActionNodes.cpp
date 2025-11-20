#include "ActionNodes.h"
#include "../Editor/BehaviorTreeEditor.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include <Frame.h>
#include <random.h>

// ========================================
// ApproachNode
// ========================================

ApproachNode::ApproachNode(MoveSpeedType speedType) : speedType_(speedType) {
    speed_ = (speedType == MoveSpeedType::Fast) ? 15.0f : 8.0f;
}

bool ApproachNode::CheckInterruptCondition(Enemy &enemy) {
    if (!enemy.GetTarget()) {
        return false;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    float distance = toTarget.Length();

    // 設定した距離以上離れている場合
    if (distance >= interruptDistance_) {
        // まだ割り込みチェックをしていない場合、確率判定
        if (!interruptCheckDone_) {
            float randomValue = Random::Range(0.0f, 1.0f);
            interruptCheckDone_ = true;
            return randomValue <= interruptChance_;
        }
        return true; // 一度割り込みが決定したら継続
    }

    return false;
}

InterruptPriority ApproachNode::GetInterruptPriority() const {
    // Fastの場合は高優先度、Slowの場合は中優先度
    return speedType_ == MoveSpeedType::Fast ? InterruptPriority::High : InterruptPriority::Medium;
}

void ApproachNode::Reset() {
    interruptCheckDone_ = false;
}

NodeStatus ApproachNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (!enemy.GetTarget()) {
        Reset();
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

// ========================================
// StopNode
// ========================================

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

// ========================================
// CloseApproachNode
// ========================================

bool CloseApproachNode::CheckInterruptCondition(Enemy &enemy) {
    if (!enemy.GetTarget()) {
        return false;
    }

    // 距離ノードを参照するかどうかを判定
    float maxDistance = 5.0f; // デフォルト距離
    if (GetUseLinkedDistance()) {
        // 上位の DistanceCheckNode を探索して取得
        if (auto *distNode = FindLinkedDistanceCheck()) {
            maxDistance = distNode->GetMaxDistance();
        }
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    float distance = toTarget.Length();

    // 距離ノードの最大距離を超えたら割り込みを要求
    if (distance > maxDistance) {
        return true;
    }

    return false;
}

void CloseApproachNode::Reset() {
    distanceSet_ = false;
}

NodeStatus CloseApproachNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (!enemy.GetTarget()) {
        Reset();
        return NodeStatus::Failure;
    }

    if (!distanceSet_) {
        currentTargetDistance_ = Random::Range(minTargetDistance_, maxTargetDistance_);
        distanceSet_ = true;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    float distance = toTarget.Length();

    if (distance <= currentTargetDistance_) {
        enemy.GetVelocity().x = 0.0f;
        enemy.GetVelocity().z = 0.0f;
        Reset();
        return NodeStatus::Success;
    }

    Vector3 direction = toTarget.Normalize();
    enemy.GetVelocity().x = direction.x * speed_;
    enemy.GetVelocity().z = direction.z * speed_;

    return NodeStatus::Running;
}

// ========================================
// StrafeNode
// ========================================

bool StrafeNode::CheckInterruptCondition(Enemy &enemy) {
    if (!enemy.GetTarget()) {
        return false;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    float distance = toTarget.Length();

    // 近~中距離範囲内(2~8m)の場合のみ実行
    return distance >= 2.0f && distance <= 8.0f;
}

void StrafeNode::Reset() {
    strafeTimer_ = 0.0f;
    timeSet_ = false;
}

NodeStatus StrafeNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (!enemy.GetTarget()) {
        Reset();
        return NodeStatus::Failure;
    }

    if (!timeSet_) {
        currentStrafeTime_ = Random::Range(minStrafeTime_, maxStrafeTime_);
        strafeDirection_ = (Random::Range(0, 1) == 0) ? 1 : -1;
        timeSet_ = true;
    }

    strafeTimer_ += deltaTime;

    if (strafeTimer_ >= currentStrafeTime_) {
        Reset();
        return NodeStatus::Success;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    Vector3 forward = toTarget.Normalize();
    Vector3 right = (Vector3(0, 1, 0).Cross(forward)).Normalize();

    enemy.GetVelocity().x = right.x * speed_ * strafeDirection_;
    enemy.GetVelocity().z = right.z * speed_ * strafeDirection_;

    return NodeStatus::Running;
}

// ========================================
// RetreatNode
// ========================================

bool RetreatNode::CheckInterruptCondition(Enemy &enemy) {
    if (!enemy.GetTarget()) {
        return false;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    float distance = toTarget.Length();

    // プレイヤーが近すぎる場合(3m以内)に後退を検討
    return distance <= 3.0f;
}

void RetreatNode::Reset() {
    distanceSet_ = false;
}

NodeStatus RetreatNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (!enemy.GetTarget()) {
        Reset();
        return NodeStatus::Failure;
    }

    if (!distanceSet_) {
        currentRetreatDistance_ = Random::Range(minRetreatDistance_, maxRetreatDistance_);
        retreatStartPos_ = enemy.GetLocalPosition();
        distanceSet_ = true;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    float movedDistance = (enemy.GetLocalPosition() - retreatStartPos_).Length();

    if (movedDistance >= currentRetreatDistance_) {
        enemy.GetVelocity().x = 0.0f;
        enemy.GetVelocity().z = 0.0f;
        Reset();
        return NodeStatus::Success;
    }

    Vector3 direction = toTarget.Normalize();
    enemy.GetVelocity().x = -direction.x * speed_;
    enemy.GetVelocity().z = -direction.z * speed_;

    return NodeStatus::Running;
}

// ========================================
// GuardNode
// ========================================

bool GuardNode::CheckInterruptCondition(Enemy &enemy) {
    if (!enemy.GetTarget()) {
        return false;
    }

    Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
    float distance = toTarget.Length();

    // 中距離(4~7m)でガードを検討
    return distance >= 4.0f && distance <= 7.0f;
}

void GuardNode::Reset() {
    if (guardStarted_) {
        // ガード状態を解除
        // enemy.SetGuarding(false); は Execute内で処理されるので不要
    }
    guardStarted_ = false;
    guardTimer_ = 0.0f;
    durationSet_ = false;
}

NodeStatus GuardNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (!guardStarted_) {
        if (!durationSet_) {
            currentGuardDuration_ = Random::Range(minGuardDuration_, maxGuardDuration_);
            durationSet_ = true;
        }

        enemy.SetGuarding(true);
        guardStarted_ = true;
        guardTimer_ = 0.0f;
    }

    enemy.GetVelocity().x = 0.0f;
    enemy.GetVelocity().z = 0.0f;

    guardTimer_ += deltaTime;

    if (guardTimer_ >= currentGuardDuration_) {
        enemy.SetGuarding(false);
        Reset();
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

bool DodgeBulletNode::CheckInterruptCondition(Enemy &enemy) {
    // TODO: 弾の接近を検知する処理を実装
    // 例: enemy.GetNearbyBullets() などでプレイヤーの弾をチェック
    // 現在は常にfalseを返す
    return false;
}

void DodgeBulletNode::Reset() {
    dodgeTimer_ = 0.0f;
    dodgeStarted_ = false;
}

NodeStatus DodgeBulletNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (!dodgeStarted_) {
        // TODO: 弾の方向から回避方向を計算
        // 現在は仮実装として横方向に回避
        dodgeDirection_ = enemy.GetRight() * (Random::Range(0, 1) == 0 ? 1.0f : -1.0f);
        dodgeStarted_ = true;
        dodgeTimer_ = 0.0f;
    }

    // 高速で回避移動
    enemy.GetVelocity().x = dodgeDirection_.x * 20.0f;
    enemy.GetVelocity().z = dodgeDirection_.z * 20.0f;

    dodgeTimer_ += deltaTime;

    if (dodgeTimer_ >= dodgeDuration_) {
        enemy.GetVelocity().x = 0.0f;
        enemy.GetVelocity().z = 0.0f;
        Reset();
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}
#include "CompositeNodes.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include <random>
#include"../Editor/BehaviorTreeEditor.h"

NodeStatus SequenceNode::Execute(Enemy &enemy, float deltaTime) {
    if (children_.empty()) {
        return NodeStatus::Failure;
    }

#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    while (currentChildIndex_ < children_.size()) {
        auto &currentChild = children_[currentChildIndex_];

#ifdef _DEBUG
        if (editor_) {
            editor_->SetExecutingNode(currentChild.get());
            editor_->AddExecutionHistory(currentChild.get());
        }
#endif

        NodeStatus status = currentChild->Execute(enemy, deltaTime);

        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        } else if (status == NodeStatus::Failure) {
            currentChildIndex_ = 0;
            return NodeStatus::Failure;
        }

        currentChildIndex_++;
    }

    currentChildIndex_ = 0;
    return NodeStatus::Success;
}

NodeStatus SelectorNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    for (auto &child : children_) {
#ifdef _DEBUG
        if (editor_) {
            editor_->SetExecutingNode(child.get());
            editor_->AddExecutionHistory(child.get());
        }
#endif

        NodeStatus status = child->Execute(enemy, deltaTime);

        if (status != NodeStatus::Failure) {
            return status;
        }
    }

    return NodeStatus::Failure;
}

int WeightedSelectorNode::SelectChildByWeight(float distance) {
    // 距離に応じて重みを調整
    std::vector<float> adjustedWeights = weights_;

    // 距離が近いほど後退の重みを上げる
    if (distance < 5.0f && adjustedWeights.size() >= 3) {
        adjustedWeights[2] *= 2.0f;
    }

    float totalWeight = 0.0f;
    for (float w : adjustedWeights) {
        totalWeight += w;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, totalWeight);

    float randomValue = static_cast<float>(dis(gen));
    float cumulative = 0.0f;

    for (size_t i = 0; i < adjustedWeights.size(); i++) {
        cumulative += adjustedWeights[i];
        if (randomValue <= cumulative) {
            return static_cast<int>(i);
        }
    }

    return 0;
}

NodeStatus WeightedSelectorNode::Execute(Enemy &enemy, float deltaTime) {
    if (children_.empty()) {
        return NodeStatus::Failure;
    }

#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    float distance = 0.0f;
    if (enemy.GetTarget()) {
        Vector3 toTarget = enemy.GetTarget()->GetLocalPosition() - enemy.GetLocalPosition();
        distance = toTarget.Length();
    }

    // 前回選択したインデックスを保持
    if (currentSelectedIndex_ < 0 || currentSelectedIndex_ >= static_cast<int>(children_.size())) {
        currentSelectedIndex_ = SelectChildByWeight(distance);
    }

    auto &selectedChild = children_[currentSelectedIndex_];

#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(selectedChild.get());
        editor_->AddExecutionHistory(selectedChild.get());
    }
#endif

    NodeStatus status = selectedChild->Execute(enemy, deltaTime);

    // 完了したら次回は新しく選択
    if (status == NodeStatus::Success || status == NodeStatus::Failure) {
        currentSelectedIndex_ = -1;
    }

    return status;
}
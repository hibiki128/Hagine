#include "InterruptNode.h"
#include "../Editor/BehaviorTreeEditor.h"
#include "Application/GameObject/Enemy/Enemy.h"

NodeStatus InterruptSelectorNode::Execute(Enemy &enemy, float deltaTime) {
#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(this);
        editor_->AddExecutionHistory(this);
    }
#endif

    if (children_.empty()) {
        return NodeStatus::Failure;
    }

    // 最も優先度の高い割り込み条件をチェック
    int highestPriorityIndex = FindHighestPriorityInterrupt(enemy);

    // 割り込みが発生した場合
    if (highestPriorityIndex != -1 && highestPriorityIndex != currentChildIndex_) {
        // 現在実行中のノードがある場合
        if (currentChildIndex_ >= 0 && currentChildIndex_ < static_cast<int>(children_.size())) {
            auto *currentNode = children_[currentChildIndex_].get();

            // 割り込み可能かチェック
            if (auto *interruptable = dynamic_cast<InterruptableNode *>(currentNode)) {
                if (!interruptable->CanBeInterrupted()) {
                    // 割り込み不可の場合は現在のノードを継続
                    highestPriorityIndex = currentChildIndex_;
                } else {
                    // 割り込み可能な場合はリセット
                    interruptable->Reset();
                }
            }
        }

        // 新しいノードに切り替え
        currentChildIndex_ = highestPriorityIndex;

        // 優先度を更新
        if (auto *interruptable = dynamic_cast<InterruptableNode *>(children_[currentChildIndex_].get())) {
            currentPriority_ = interruptable->GetInterruptPriority();
        }
    }

    // 実行するノードがない場合
    if (currentChildIndex_ < 0 || currentChildIndex_ >= static_cast<int>(children_.size())) {
        // 最初の成功するノードを探す
        for (size_t i = 0; i < children_.size(); i++) {
            auto &child = children_[i];

#ifdef _DEBUG
            if (editor_) {
                editor_->SetExecutingNode(child.get());
                editor_->AddExecutionHistory(child.get());
            }
#endif

            NodeStatus status = child->Execute(enemy, deltaTime);

            if (status != NodeStatus::Failure) {
                currentChildIndex_ = static_cast<int>(i);
                return status;
            }
        }

        currentChildIndex_ = -1;
        return NodeStatus::Failure;
    }

    // 現在のノードを実行
    auto &currentChild = children_[currentChildIndex_];

#ifdef _DEBUG
    if (editor_) {
        editor_->SetExecutingNode(currentChild.get());
        editor_->AddExecutionHistory(currentChild.get());
    }
#endif

    NodeStatus status = currentChild->Execute(enemy, deltaTime);

    // ノードが完了した場合
    if (status == NodeStatus::Success || status == NodeStatus::Failure) {
        currentChildIndex_ = -1;
        currentPriority_ = InterruptPriority::None;
    }

    return status;
}

int InterruptSelectorNode::FindHighestPriorityInterrupt(Enemy &enemy) {
    int highestIndex = -1;
    InterruptPriority highestPriority = currentPriority_;

    for (size_t i = 0; i < children_.size(); i++) {
        auto *interruptable = dynamic_cast<InterruptableNode *>(children_[i].get());

        if (interruptable) {
            // 割り込み条件をチェック
            if (interruptable->CheckInterruptCondition(enemy)) {
                InterruptPriority priority = interruptable->GetInterruptPriority();

                // より高い優先度の場合
                if (priority > highestPriority) {
                    highestPriority = priority;
                    highestIndex = static_cast<int>(i);
                }
            }
        }
    }

    return highestIndex;
}

void InterruptSelectorNode::ResetCurrentChild() {
    if (currentChildIndex_ >= 0 && currentChildIndex_ < static_cast<int>(children_.size())) {
        if (auto *interruptable = dynamic_cast<InterruptableNode *>(children_[currentChildIndex_].get())) {
            interruptable->Reset();
        }
    }
    currentChildIndex_ = -1;
    currentPriority_ = InterruptPriority::None;
}
#include "SequenceNode.h"
#include"Application/GameObject/Enemy/BehaviorTree/Editor/BehaviorTreeEditor.h"

NodeStatus SequenceNode::Execute(Enemy &enemy, float deltaTime) {
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
        if (status != NodeStatus::Success) {
#ifdef _DEBUG
            if (editor_) {
                editor_->ClearExecutingNode();
            }
#endif
            return status;
        }
    }

#ifdef _DEBUG
    if (editor_) {
        editor_->ClearExecutingNode();
    }
#endif
    return NodeStatus::Success;
}

void SequenceNode::AddChild(std::unique_ptr<BehaviorNode> child) {
    children_.push_back(std::move(child));
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
#ifdef _DEBUG
            if (editor_) {
                editor_->ClearExecutingNode();
            }
#endif
            return status;
        }
    }

#ifdef _DEBUG
    if (editor_) {
        editor_->ClearExecutingNode();
    }
#endif
    return NodeStatus::Failure;
}

void SelectorNode::AddChild(std::unique_ptr<BehaviorNode> child) {
    children_.push_back(std::move(child));
}
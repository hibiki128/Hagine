#pragma once
#include"Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"

class SequenceNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override {
        for (auto &child : children_) {
            NodeStatus status = child->Execute(enemy, deltaTime);
            if (status != NodeStatus::Success) {
                return status;
            }
        }
        return NodeStatus::Success;
    }

    const char *GetNodeName() const override { return "Sequence"; }

    void AddChild(std::unique_ptr<BehaviorNode> child) {
        children_.push_back(std::move(child));
    }

    std::vector<std::unique_ptr<BehaviorNode>> &GetChildren() { return children_; }

  private:
    std::vector<std::unique_ptr<BehaviorNode>> children_;
};

class SelectorNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override {
        for (auto &child : children_) {
            NodeStatus status = child->Execute(enemy, deltaTime);
            if (status != NodeStatus::Failure) {
                return status;
            }
        }
        return NodeStatus::Failure;
    }

    const char *GetNodeName() const override { return "Selector"; }

    void AddChild(std::unique_ptr<BehaviorNode> child) {
        children_.push_back(std::move(child));
    }

    std::vector<std::unique_ptr<BehaviorNode>> &GetChildren() { return children_; }

  private:
    std::vector<std::unique_ptr<BehaviorNode>> children_;
};
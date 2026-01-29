#pragma once
#include "BehaviorBaseNode.h"
#include <memory>
#include <vector>

/// <summary>
/// シーケンスノード(すべての子を順番に実行)
/// </summary>
class SequenceNode : public BehaviorBaseNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Sequence"; }

    void AddChild(std::unique_ptr<BehaviorBaseNode> child) {
        children_.push_back(std::move(child));
    }

    std::vector<std::unique_ptr<BehaviorBaseNode>> &GetChildren() { return children_; }

  private:
    std::vector<std::unique_ptr<BehaviorBaseNode>> children_;
    size_t currentChildIndex_ = 0;
};

/// <summary>
/// セレクターノード(成功するまで子を実行)
/// </summary>
class SelectorNode : public BehaviorBaseNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Selector"; }

    void AddChild(std::unique_ptr<BehaviorBaseNode> child) {
        children_.push_back(std::move(child));
    }

    std::vector<std::unique_ptr<BehaviorBaseNode>> &GetChildren() { return children_; }

  private:
    std::vector<std::unique_ptr<BehaviorBaseNode>> children_;
};

/// <summary>
/// 重み付きセレクターノード(重みに基づいて子を選択)
/// </summary>
class WeightedSelectorNode : public BehaviorBaseNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "WeightedSelector"; }

    void AddChild(std::unique_ptr<BehaviorBaseNode> child, float weight) {
        children_.push_back(std::move(child));
        weights_.push_back(weight);
    }

    std::vector<std::unique_ptr<BehaviorBaseNode>> &GetChildren() { return children_; }
    std::vector<float> &GetWeights() { return weights_; }

  private:
    std::vector<std::unique_ptr<BehaviorBaseNode>> children_;
    std::vector<float> weights_;
    int currentSelectedIndex_ = -1; // 追加
    int SelectChildByWeight(float distance);
};
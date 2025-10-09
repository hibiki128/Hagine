#pragma once
#include "Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "imgui.h"
#include <unordered_map>
#include <vector>

class BehaviorTreeEditor {
  public:
    void DrawEditor(BehaviorNode *root);

  private:
    void DrawNode(BehaviorNode *node, ImVec2 pos, int depth);
    void DrawChildren(std::vector<std::unique_ptr<BehaviorNode>> &children,
                      ImVec2 parentPos, int depth);

    int CalculateTreeWidth(BehaviorNode *node);
    void DrawNodeProperties(BehaviorNode *node);

    int nodeIdCounter_ = 0;
    BehaviorNode *selectedNode_ = nullptr;

    const float NODE_WIDTH = 120.0f;
    const float NODE_HEIGHT = 40.0f;
    const float HORIZONTAL_SPACING = 30.0f;
    const float VERTICAL_SPACING = 80.0f;

    // ノードの重み付け情報
    std::unordered_map<BehaviorNode *, float> nodeWeights_;
};
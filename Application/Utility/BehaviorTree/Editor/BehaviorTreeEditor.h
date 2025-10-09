#pragma once

#include "Application/Utility/BehaviorTree/Nodes/SequenceNode.h"
#include "imgui.h"
#include <Application/Utility/BehaviorTree/BehaviorNode/BehaviorNode.h>
#include <vector>

class BehaviorTreeEditor {
  public:
    void DrawEditor(BehaviorNode *root);
  private:
    void DrawNode(BehaviorNode *node, ImVec2 pos, int depth);
    void DrawChildren(std::vector<std::unique_ptr<BehaviorNode>> &children,
                      ImVec2 parentPos, int depth);

    int CalculateTreeWidth(BehaviorNode *node);

    int nodeIdCounter_ = 0;
    const float NODE_WIDTH = 120.0f;
    const float NODE_HEIGHT = 40.0f;
    const float HORIZONTAL_SPACING = 30.0f;
    const float VERTICAL_SPACING = 80.0f;
};
#include "BehaviorTreeEditor.h"

void BehaviorTreeEditor::DrawEditor(BehaviorNode *root) {
    if (!root)
        return;

    ImGui::Begin("Behavior Tree Editor");

    nodeIdCounter_ = 0;
    DrawNode(root, ImVec2(400, 50), 0);

    ImGui::End();
}

void BehaviorTreeEditor::DrawNode(BehaviorNode *node, ImVec2 pos, int depth) {
    if (!node)
        return;

    node->nodeId = nodeIdCounter_++;

    // ノードの描画
    ImGui::SetCursorPos(pos);

    ImVec4 color = ImVec4(0.2f, 0.6f, 0.8f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, color);

    std::string label = std::string(node->GetNodeName()) + "##" + std::to_string(node->nodeId);
    ImGui::Button(label.c_str(), ImVec2(NODE_WIDTH, NODE_HEIGHT));

    ImGui::PopStyleColor();

    // 子ノードの描画
    if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
        DrawChildren(sequence->GetChildren(), pos, depth);
    } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
        DrawChildren(selector->GetChildren(), pos, depth);
    }
}

void BehaviorTreeEditor::DrawChildren(std::vector<std::unique_ptr<BehaviorNode>> &children,
                                      ImVec2 parentPos, int depth) {
    if (children.empty())
        return;

    // 親ノードの中心座標を計算
    ImVec2 parentCenter(parentPos.x + NODE_WIDTH * 0.5f, parentPos.y + NODE_HEIGHT);

    // 子ノード全体の幅を計算
    float totalWidth = 0.0f;
    for (auto &child : children) {
        int childTreeWidth = CalculateTreeWidth(child.get());
        totalWidth += childTreeWidth * (NODE_WIDTH + HORIZONTAL_SPACING);
    }

    // 最後のスペーシングを削除
    if (!children.empty()) {
        totalWidth -= HORIZONTAL_SPACING;
    }

    // 子ノードの開始X座標
    float startX = parentCenter.x - totalWidth * 0.5f;
    float currentX = startX;
    float childY = parentPos.y + VERTICAL_SPACING;

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 scrollOffset = ImGui::GetCursorStartPos();

    for (size_t i = 0; i < children.size(); ++i) {
        int childTreeWidth = CalculateTreeWidth(children[i].get());
        float childWidth = childTreeWidth * (NODE_WIDTH + HORIZONTAL_SPACING) - HORIZONTAL_SPACING;

        // 子ノードの中心X座標
        float childCenterX = currentX + childWidth * 0.5f;
        ImVec2 childPos(childCenterX - NODE_WIDTH * 0.5f, childY);

        // 線を描画（親の中心から子の中心へ）
        ImVec2 p1(windowPos.x + parentCenter.x, windowPos.y + parentCenter.y);
        ImVec2 p2(windowPos.x + childCenterX, windowPos.y + childY);

        drawList->AddLine(p1, p2, IM_COL32(255, 255, 255, 200), 2.0f);

        // 子ノードを描画
        DrawNode(children[i].get(), childPos, depth + 1);

        currentX += childWidth + HORIZONTAL_SPACING;
    }
}

int BehaviorTreeEditor::CalculateTreeWidth(BehaviorNode *node) {
    if (!node)
        return 0;

    // コンポジットノードの場合、子ノードの幅の合計を返す
    if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
        auto &children = sequence->GetChildren();
        if (children.empty())
            return 1;

        int totalWidth = 0;
        for (auto &child : children) {
            totalWidth += CalculateTreeWidth(child.get());
        }
        return totalWidth;
    } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
        auto &children = selector->GetChildren();
        if (children.empty())
            return 1;

        int totalWidth = 0;
        for (auto &child : children) {
            totalWidth += CalculateTreeWidth(child.get());
        }
        return totalWidth;
    }

    // リーフノードは幅1
    return 1;
}
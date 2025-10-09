#include "BehaviorTreeEditor.h"
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/SequenceNode.h>
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/ActionNodes.h>
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/ConditionNodes.h>

void BehaviorTreeEditor::DrawEditor(BehaviorNode *root) {
    if (!root)
        return;

    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("Behavior Tree Editor");

    // レイアウト分割（ツリー表示 + プロパティパネル）
    float panelWidth = ImGui::GetWindowWidth() * 0.75f;

    // ツリー表示エリア
    ImGui::BeginChild("TreeView", ImVec2(panelWidth - 10, -1), true, ImGuiWindowFlags_NoMove);

    nodeIdCounter_ = 0;
    DrawNode(root, ImVec2(400, 50), 0);

    ImGui::EndChild();

    ImGui::SameLine();

    // プロパティパネル
    ImGui::BeginChild("Properties", ImVec2(0, -1), true);
    ImGui::Text("Node Properties");
    ImGui::Separator();

    if (selectedNode_) {
        DrawNodeProperties(selectedNode_);
    } else {
        ImGui::Text("Select a node to edit properties");
    }

    ImGui::EndChild();

    ImGui::End();
}

void BehaviorTreeEditor::DrawNode(BehaviorNode *node, ImVec2 pos, int depth) {
    if (!node)
        return;

    node->nodeId = nodeIdCounter_++;

    ImGui::SetCursorPos(pos);

    // ノードのインタラクティブな描画
    bool isSelected = (selectedNode_ == node);
    ImVec4 color = isSelected ? ImVec4(0.8f, 0.2f, 0.2f, 1.0f) : ImVec4(0.2f, 0.6f, 0.8f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, color);

    std::string label = std::string(node->GetNodeName()) + "##" + std::to_string(node->nodeId);
    if (ImGui::Button(label.c_str(), ImVec2(NODE_WIDTH, NODE_HEIGHT))) {
        selectedNode_ = node;
    }

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

    ImVec2 parentCenter(parentPos.x + NODE_WIDTH * 0.5f, parentPos.y + NODE_HEIGHT);

    float totalWidth = 0.0f;
    for (auto &child : children) {
        int childTreeWidth = CalculateTreeWidth(child.get());
        totalWidth += childTreeWidth * (NODE_WIDTH + HORIZONTAL_SPACING);
    }

    if (!children.empty()) {
        totalWidth -= HORIZONTAL_SPACING;
    }

    float startX = parentCenter.x - totalWidth * 0.5f;
    float currentX = startX;
    float childY = parentPos.y + VERTICAL_SPACING;

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();

    for (size_t i = 0; i < children.size(); ++i) {
        int childTreeWidth = CalculateTreeWidth(children[i].get());
        float childWidth = childTreeWidth * (NODE_WIDTH + HORIZONTAL_SPACING) - HORIZONTAL_SPACING;

        float childCenterX = currentX + childWidth * 0.5f;
        ImVec2 childPos(childCenterX - NODE_WIDTH * 0.5f, childY);

        ImVec2 p1(windowPos.x + parentCenter.x, windowPos.y + parentCenter.y);
        ImVec2 p2(windowPos.x + childCenterX, windowPos.y + childY);

        drawList->AddLine(p1, p2, IM_COL32(255, 255, 255, 200), 2.0f);

        DrawNode(children[i].get(), childPos, depth + 1);

        currentX += childWidth + HORIZONTAL_SPACING;
    }
}

int BehaviorTreeEditor::CalculateTreeWidth(BehaviorNode *node) {
    if (!node)
        return 0;

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

    return 1;
}

void BehaviorTreeEditor::DrawNodeProperties(BehaviorNode *node) {
    ImGui::Text("Node Type: %s", node->GetNodeName());
    ImGui::Separator();

    // 重み付け（優先度）設定
    if (nodeWeights_.find(node) == nodeWeights_.end()) {
        nodeWeights_[node] = 1.0f;
    }

    float weight = nodeWeights_[node];
    ImGui::DragFloat("Priority Weight", &weight, 0.1f, 0.0f, 10.0f);
    nodeWeights_[node] = weight;

    ImGui::Separator();
    ImGui::Text("Node Specific Settings:");

    // ノードタイプごとの設定
    if (auto *moveNode = dynamic_cast<MoveToTargetNode *>(node)) {
        float speed = moveNode->GetSpeed();
        ImGui::DragFloat("Move Speed##move", &speed, 0.5f, 0.0f, 500.0f);
        moveNode->SetSpeed(speed);
    } else if (auto *flyNode = dynamic_cast<FlyToTargetNode *>(node)) {
        float speed = flyNode->GetSpeed();
        ImGui::DragFloat("Fly Speed##fly", &speed, 0.5f, 0.0f, 500.0f);
        flyNode->SetSpeed(speed);
    } else if (auto *rushNode = dynamic_cast<RushAttackNode *>(node)) {
        float speed = rushNode->GetRushSpeed();
        float minDist = rushNode->GetMinDistance();
        ImGui::DragFloat("Rush Speed##rush", &speed, 0.5f, 0.0f, 500.0f);
        ImGui::DragFloat("Min Distance##rush", &minDist, 0.1f, 0.0f, 50.0f);
        rushNode->SetRushSpeed(speed);
        rushNode->SetMinDistance(minDist);
    } else if (auto *distNode = dynamic_cast<DistanceToTargetNode *>(node)) {
        float minDist = distNode->GetMinDistance();
        float maxDist = distNode->GetMaxDistance();
        ImGui::DragFloat("Min Distance##dist", &minDist, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Max Distance##dist", &maxDist, 0.1f, 0.0f, 100.0f);
        distNode->SetDistances(minDist, maxDist);
    }
}
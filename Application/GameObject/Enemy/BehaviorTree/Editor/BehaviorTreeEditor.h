#pragma once
#include "Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"
#ifdef _DEBUG
#include "imgui.h"
#endif // DEBUG
#include <string>
#include <unordered_map>
#include <vector>

class BehaviorTreeEditor {
#ifdef _DEBUG
  public:
    void DrawEditor(BehaviorNode *root);
    void SaveSettings(const std::string &treeName);
    void LoadSettings(const std::string &treeName, BehaviorNode *root);

    // 実行状態の追跡
    void SetExecutingNode(BehaviorNode *node) { executingNode_ = node; }
    void ClearExecutingNode() { executingNode_ = nullptr; }
    void AddExecutionHistory(BehaviorNode *node);
    void ClearExecutionHistory() { executionHistory_.clear(); }

  private:
    void DrawNode(BehaviorNode *node, ImVec2 pos, int depth, ImVec2 canvasOrigin);
    void DrawChildren(std::vector<std::unique_ptr<BehaviorNode>> &children,
                      ImVec2 parentPos, int depth, ImVec2 canvasOrigin);

    int CalculateTreeWidth(BehaviorNode *node);
    void DrawNodeProperties(BehaviorNode *node);
    void DrawToolbar(const std::string &treeName);

    int nodeIdCounter_ = 0;
    BehaviorNode *selectedNode_ = nullptr;
    BehaviorNode *executingNode_ = nullptr;        // 現在実行中のノード
    std::vector<BehaviorNode *> executionHistory_; // 実行履歴（最大10個）
    const int MAX_HISTORY = 10;

    const float NODE_WIDTH = 120.0f;
    const float NODE_HEIGHT = 40.0f;
    const float HORIZONTAL_SPACING = 30.0f;
    const float VERTICAL_SPACING = 80.0f;

    // ズーム・パン機能
    float zoom_ = 1.0f;
    ImVec2 panOffset_ = ImVec2(0.0f, 0.0f);
    bool isPanning_ = false;
    ImVec2 panStartPos_ = ImVec2(0.0f, 0.0f);

    // ノードの重み付け情報
    std::unordered_map<BehaviorNode *, float> nodeWeights_;

    // セーブ/ロード用のツリー名
    char treeNameBuffer_[256] = "DefaultTree";
#endif // _DEBUG
};
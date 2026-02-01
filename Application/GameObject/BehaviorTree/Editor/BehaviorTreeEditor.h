#pragma once
#include "Application/GameObject/BehaviorTree/Node/BehaviorNode.h"
#include "Engine/Utility/Data/DataHandler.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include <externals/nlohmann/json.hpp>
#include <map>
#include <string>
#include <vector>

enum class EditorNodeType {
    Sequence,
    Selector,
    SelectorRandom,
    DecoratorWeight,
    ActionRun,
    ConditionPlayerClose,
    ConditionHealthLow,
    ActionApproach,
    ActionDash,
    ActionStrafe,
    ActionRetreat,
    ActionAttack,
    ActionIdle
};

// 重み付けノードの各出力の情報
struct WeightedOutput {
    ax::NodeEditor::PinId PinID;
    float Weight = 1.0f;
};

struct EditorNode {
    ax::NodeEditor::NodeId ID;
    std::string Title;
    EditorNodeType Type;
    ImVec2 Position;
    ax::NodeEditor::PinId InputPinID;
    ax::NodeEditor::PinId OutputPinID;
    ax::NodeEditor::PinId SuccessPinID; // 条件ノード用: 成功時の出力
    ax::NodeEditor::PinId FailurePinID; // 条件ノード用: 失敗時の出力

    // 重み付けノード用: 複数の出力ピンとそれぞれの重み
    std::vector<WeightedOutput> WeightedOutputs;

    // パラメータ
    float Parameter = 0.0f;
    float Parameter2 = 0.0f;
    float Parameter3 = 0.0f;

    EditorNode(int id, const std::string &title, EditorNodeType type);

    // 条件ノードかどうかを判定
    bool IsConditionNode() const {
        return Type == EditorNodeType::ConditionPlayerClose ||
               Type == EditorNodeType::ConditionHealthLow;
    }

    // アクションノードかどうかを判定
    bool IsActionNode() const {
        return Type == EditorNodeType::ActionRun ||
               Type == EditorNodeType::ActionApproach ||
               Type == EditorNodeType::ActionDash ||
               Type == EditorNodeType::ActionStrafe ||
               Type == EditorNodeType::ActionRetreat ||
               Type == EditorNodeType::ActionAttack ||
               Type == EditorNodeType::ActionIdle;
    }

    // 重み付けノードかどうかを判定
    bool IsWeightNode() const {
        return Type == EditorNodeType::DecoratorWeight;
    }
};

struct EditorLink {
    ax::NodeEditor::LinkId ID;
    ax::NodeEditor::PinId StartPinID;
    ax::NodeEditor::PinId EndPinID;
    EditorLink(int id, ax::NodeEditor::PinId start, ax::NodeEditor::PinId end);
};

class BehaviorTreeEditor {
  public:
    BehaviorTreeEditor();
    ~BehaviorTreeEditor();

    void OnImGuiRender();
    void SetDebugTargets(Enemy *enemy, Player *player) {
        m_DebugEnemy = enemy;
        m_DebugPlayer = player;
    }
    std::shared_ptr<BTNode> GetRuntimeRoot() { return m_RuntimeRoot; }

  private:
    void CreateNode(const std::string &title, EditorNodeType type);
    void DeleteSelectedItems();
    void HandleCreateAction();
    void BuildAndRunTree();
    std::shared_ptr<BTNode> BuildNodeRecursive(int editorNodeId);
    std::vector<int> FindChildrenNodeIds(int outputPinId);
    std::vector<std::pair<int, float>> FindWeightedChildrenNodeIds(const EditorNode &node);
    int FindRootNodeId();
    bool IsInputPin(ax::NodeEditor::PinId pinId);
    bool IsOutputPin(ax::NodeEditor::PinId pinId);
    bool IsSuccessPin(ax::NodeEditor::PinId pinId);
    bool IsFailurePin(ax::NodeEditor::PinId pinId);
    bool IsWeightedOutputPin(ax::NodeEditor::PinId pinId, int &outNodeId, int &outOutputIndex);
    void SaveTree();
    void LoadTree(const std::string &filePath);
    void ParsePathToFolderAndFile(const std::string &fullPath, std::string &outFolder, std::string &outFile);
    const char *GetNodeDescription(EditorNodeType type);

    ax::NodeEditor::EditorContext *m_Context = nullptr;
    std::vector<EditorNode> m_Nodes;
    std::vector<EditorLink> m_Links;

    int m_NextNodeId = 1;
    int m_NextLinkId = 1;
    int m_NextPinId = 10000; // 動的に作成されるピンのID管理用
    ImVec2 m_CreatePos = ImVec2(0, 0);

    bool m_IsRunning = false;
    Enemy *m_DebugEnemy = nullptr;
    Player *m_DebugPlayer = nullptr;

    std::shared_ptr<BTNode> m_RuntimeRoot = nullptr;
    std::map<int, std::shared_ptr<BTNode>> m_nodeInstanceMap;
    std::map<int, float> m_statusTimers;
    std::string m_LastResultText = "待機中";
    ImVec4 m_LastResultColor = ImVec4(1, 1, 1, 1);

    char m_InputFileNameBuf[128] = "NewBehavior";
    std::string m_SelectedFileName = "";
    bool m_ShowLoadWindow = false;
};
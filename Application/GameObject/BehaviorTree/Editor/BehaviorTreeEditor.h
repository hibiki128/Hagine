#pragma once
#include "Application/GameObject/BehaviorTree/Node/BehaviorNode.h"
#include "Engine/Utility/Data/DataHandler.h"
#include <memory>
#include <string>

// ============================================================
// ノードタイプ定義 (Debug / Release 共通)
// ============================================================
enum class EditorNodeType {
    Sequence,
    Selector,
    SelectorRandom,
    DecoratorWeight,
    ActionRun,
    ConditionPlayerClose,
    ConditionHealthLow,
    ConditionIsGrounded,
    ConditionIsAirborne,
    ConditionPlayerState,
    ActionApproach,
    ActionDash,
    ActionStrafe,
    ActionRetreat,
    ActionAttack,
    ActionIdle,
    ActionJump,
    ActionJumpToFly,
    ActionFlyAscend,
    ActionFlyDescend,
    ActionFlyToGround
};

// ============================================================
// BehaviorTreeLoader
// Debug / Release 共通で使えるJSON→ランタイムツリー変換クラス
// ============================================================
class BehaviorTreeLoader {
  public:
    /// <summary>
    /// 指定JSONファイルからランタイムBTを構築して返す
    /// </summary>
    /// <param name="folder">リソースフォルダ名 (例: "BehaviorTree")</param>
    /// <param name="file">ファイル名 (.json なし, 例: "EnemyBehavior")</param>
    /// <returns>ルートノード (失敗時は nullptr)</returns>
    static std::shared_ptr<BTNode> LoadAndBuild(const std::string &folder, const std::string &file);

  private:
    // --- JSONから復元したノード情報 (内部用) ---
    struct NodeData {
        int id = 0;
        EditorNodeType type = EditorNodeType::ActionIdle;
        float param = 0.0f;
        float param2 = 0.0f;
        float param3 = 0.0f;
        std::string stateName = "Idle";
        // 重み付けノード用
        struct WeightPin {
            int pinId = 0;
            float weight = 1.0f;
        };
        std::vector<WeightPin> weightedOutputs;
    };

    struct LinkData {
        int startPin = 0;
        int endPin = 0;
    };

    static std::shared_ptr<BTNode> BuildNodeRecursive(
        int nodeId,
        const std::vector<NodeData> &nodes,
        const std::vector<LinkData> &links);

    static std::vector<int> FindChildrenNodeIds(
        int outputPinId,
        const std::vector<LinkData> &links);

    static std::vector<std::pair<int, float>> FindWeightedChildrenNodeIds(
        const NodeData &node,
        const std::vector<LinkData> &links);

    static int FindRootNodeId(
        const std::vector<NodeData> &nodes,
        const std::vector<LinkData> &links);
};

// ============================================================
// エディタ (Debugビルド専用)
// ============================================================
#ifdef _DEBUG
#include "imgui.h"
#include "imgui_node_editor.h"
#include <externals/nlohmann/json.hpp>
#include <map>
#include <vector>

class Enemy;
class Player;

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
    ax::NodeEditor::PinId SuccessPinID;
    ax::NodeEditor::PinId FailurePinID;

    std::vector<WeightedOutput> WeightedOutputs;

    float Parameter = 0.0f;
    float Parameter2 = 0.0f;
    float Parameter3 = 0.0f;

    std::string StateNameParameter = "Idle";

    EditorNode(int id, const std::string &title, EditorNodeType type);

    bool IsConditionNode() const {
        return Type == EditorNodeType::ConditionPlayerClose ||
               Type == EditorNodeType::ConditionHealthLow ||
               Type == EditorNodeType::ConditionIsGrounded ||
               Type == EditorNodeType::ConditionIsAirborne ||
               Type == EditorNodeType::ConditionPlayerState;
    }

    bool IsActionNode() const {
        return Type == EditorNodeType::ActionRun ||
               Type == EditorNodeType::ActionApproach ||
               Type == EditorNodeType::ActionDash ||
               Type == EditorNodeType::ActionStrafe ||
               Type == EditorNodeType::ActionRetreat ||
               Type == EditorNodeType::ActionAttack ||
               Type == EditorNodeType::ActionIdle ||
               Type == EditorNodeType::ActionJump ||
               Type == EditorNodeType::ActionJumpToFly ||
               Type == EditorNodeType::ActionFlyAscend ||
               Type == EditorNodeType::ActionFlyDescend ||
               Type == EditorNodeType::ActionFlyToGround;
    }

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
    int m_NextPinId = 10000;
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

#endif // _DEBUG
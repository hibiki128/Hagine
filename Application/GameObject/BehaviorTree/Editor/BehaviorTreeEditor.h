#pragma once
#include "Application/GameObject/BehaviorTree/Node/BehaviorNode.h"
#include "Engine/Utility/Data/DataHandler.h"
#include <memory>
#include <string>

// ============================================================
// ノードタイプ定義 (Debug / Release 共通)
// ============================================================
enum class EditorNodeType {
    // ===== 既存ノード (Before と同じ順序・int値を保持) =====
    Sequence,             // 0
    Selector,             // 1
    SelectorRandom,       // 2
    DecoratorWeight,      // 3
    ActionRun,            // 4
    ConditionPlayerClose, // 5
    ConditionHealthLow,   // 6
    ActionApproach,       // 7
    ActionDash,           // 8
    ActionStrafe,         // 9
    ActionRetreat,        // 10
    ActionAttack,         // 11
    ActionIdle,           // 12
    // ===== 新規追加ノード (末尾に追加してint値のずれを防ぐ) =====
    ActionJump,           // 13
    ActionJumpToFly,      // 14
    ActionFlyAscend,      // 15
    ActionFlyDescend,     // 16
    ActionFlyToGround,    // 17
    ConditionIsGrounded,  // 18
    ConditionIsAirborne,  // 19
    ConditionPlayerState, // 20
    // ===== 射撃・ロックオン =====
    ActionShoot,       // 21
    ActionLockOn,      // 22
    ConditionIsLockOn, // 23
    // ===== 近接コンボ =====
    ActionComboStep, // 24  コンボ1段
    ActionComboFull, // 25  コンボ全段
    // ===== 連射 =====
    ActionBurstShoot, // 26  N連発
    // ===== Non-Reactive シーケンス =====
    SequenceOnce, // 27  アクションを最後まで実行するシーケンス
    // ===== エネルギー =====
    ConditionEnergyLow, // 28  エネルギーが低いかチェック
    ActionEnergyCharge, // 29  エネルギーチャージ
    // ===== 飛行中水平移動 =====
    ActionFlyApproach // 30  飛行中にプレイヤーへ水平接近
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
        float param4 = 0.0f; // 拡散角度など追加パラメータ用
        float param5 = 0.0f; // homingMode(0=拡散, 1=ホーミング)など
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
    float Parameter4 = 0.0f; // 拡散角度など
    float Parameter5 = 0.0f; // homingMode(0=拡散, 1=ホーミング)など

    std::string StateNameParameter = "Idle";

    EditorNode(int id, const std::string &title, EditorNodeType type);

    bool IsConditionNode() const {
        return Type == EditorNodeType::ConditionPlayerClose ||
               Type == EditorNodeType::ConditionHealthLow ||
               Type == EditorNodeType::ConditionEnergyLow ||
               Type == EditorNodeType::ConditionIsGrounded ||
               Type == EditorNodeType::ConditionIsAirborne ||
               Type == EditorNodeType::ConditionPlayerState ||
               Type == EditorNodeType::ConditionIsLockOn;
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
               Type == EditorNodeType::ActionFlyToGround ||
               Type == EditorNodeType::ActionShoot ||
               Type == EditorNodeType::ActionLockOn ||
               Type == EditorNodeType::ActionComboStep ||
               Type == EditorNodeType::ActionComboFull ||
               Type == EditorNodeType::ActionBurstShoot ||
               Type == EditorNodeType::ActionEnergyCharge ||
               Type == EditorNodeType::ActionFlyApproach;
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
    // -------------------------------------------------------
    // [修正] ピンIDをノードIDと衝突しない値域から開始する
    //   ピンIDは kPinOffset(100000) + id*10 + N で生成されるため、
    //   追加ピン(WeightedOutput 拡張分)も 200000 以降から割り当てれば
    //   衝突しない。
    // -------------------------------------------------------
    int m_NextPinId = 200000;
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

    bool m_LayoutDirty_ = false;                     // レイアウト変更フラグ
    float m_SaveCooldown_ = 0.0f;                    // 保存クールダウン
    static constexpr float kSaveCooldownTime = 2.0f; // 2秒操作がなければ保存
};

#endif // _DEBUG
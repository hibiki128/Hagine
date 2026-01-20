#include "BehaviorTreeEditor.h"

// ★修正: includeパス
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "Input.h"
#include <ShowFolder/ShowFolder.h> // ★修正

#include <algorithm>
#include <filesystem>
#include <imgui_internal.h>
#include <iostream>

namespace ed = ax::NodeEditor;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ---------------------------------------------------------
// EditorNode
// ---------------------------------------------------------
EditorNode::EditorNode(int id, const std::string &title, EditorNodeType type)
    : ID(id), Title(title), Type(type) {
    InputPinID = id * 10 + 1;
    OutputPinID = id * 10 + 2;

    // デフォルト値設定
    if (type == EditorNodeType::ConditionPlayerClose) {
        Parameter = 0.0f;   // Min Dist
        Parameter2 = 10.0f; // Max Dist
    } else if (type == EditorNodeType::ConditionHealthLow) {
        Parameter = 0.3f;
    }
    // 移動系ノードのデフォルト値
    else if (type == EditorNodeType::ActionApproach) {
        Parameter = 1.0f;  // Min Time
        Parameter2 = 3.0f; // Max Time
        Parameter3 = 0.1f; // Speed (通常)
    } else if (type == EditorNodeType::ActionDash) {
        Parameter = 0.5f;  // Min Time
        Parameter2 = 1.5f; // Max Time
        Parameter3 = 0.3f; // Speed (高速)
    } else if (type == EditorNodeType::ActionStrafe) {
        Parameter = 1.0f;
        Parameter2 = 2.0f;
        Parameter3 = 0.08f;
    } else if (type == EditorNodeType::ActionRetreat) {
        Parameter = 1.0f;
        Parameter2 = 2.0f;
        Parameter3 = 0.15f;
    }
}

EditorLink::EditorLink(int id, ed::PinId start, ed::PinId end)
    : ID(id), StartPinID(start), EndPinID(end) {
}

// ---------------------------------------------------------
// Editor
// ---------------------------------------------------------
BehaviorTreeEditor::BehaviorTreeEditor() {
    ed::Config config;
    config.SettingsFile = "BehaviorTreeLayout.json";
    m_Context = ed::CreateEditor(&config);
}

BehaviorTreeEditor::~BehaviorTreeEditor() {
    if (m_Context)
        ed::DestroyEditor(m_Context);
}

bool BehaviorTreeEditor::IsInputPin(ed::PinId pinId) { return (pinId.Get() % 10) == 1; }
bool BehaviorTreeEditor::IsOutputPin(ed::PinId pinId) { return (pinId.Get() % 10) == 2; }

void BehaviorTreeEditor::ParsePathToFolderAndFile(const std::string &fullPath, std::string &outFolder, std::string &outFile) {
    fs::path path(fullPath);
    outFile = path.stem().string();
    fs::path parent = path.parent_path();
    if (parent.has_filename())
        outFolder = parent.filename().string();
    else
        outFolder = "BehaviorTree";
}

// ---------------------------------------------------------
// OnImGuiRender
// ---------------------------------------------------------
void BehaviorTreeEditor::OnImGuiRender() {
    ed::SetCurrentEditor(m_Context);

    // --- 上部コントロール ---
    ImGui::Text("File Name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("##FileName", m_InputFileNameBuf, IM_ARRAYSIZE(m_InputFileNameBuf));
    ImGui::SameLine();
    ImGui::Text(".json");

    ImGui::SameLine();
    if (ImGui::Button("Save"))
        SaveTree();
    ImGui::SameLine();
    if (ImGui::Button("Load"))
        m_ShowLoadWindow = true;

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    if (ImGui::Button("Build & Run")) {
        BuildAndRunTree();
        m_IsRunning = true;
        m_LastResultText = "実行中...";
        m_LastResultColor = ImVec4(1, 1, 1, 1);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        m_IsRunning = false;
        m_RuntimeRoot = nullptr;
        // ★修正: SetVelocityを使用
        if (m_DebugEnemy)
            m_DebugEnemy->SetVelocity({0, 0, 0});
        m_LastResultText = "停止中";
        m_LastResultColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    }

    ImGui::SameLine();
    ImGui::TextColored(m_LastResultColor, "Status: %s", m_LastResultText.c_str());

    // --- ロードウィンドウ ---
    if (m_ShowLoadWindow) {
        ImGui::Begin("Load Behavior Tree", &m_ShowLoadWindow);
        // ★修正: startPathの変更
        static std::string startPath = "BehaviorTree";
        ShowJsonFile(m_SelectedFileName, startPath);

        if (!m_SelectedFileName.empty()) {
            if (ImGui::Button("Load Selected File")) {
                LoadTree(m_SelectedFileName);
                m_ShowLoadWindow = false;
            }
        }
        ImGui::End();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // --- ステータス更新 ---
    if (m_IsRunning && m_RuntimeRoot) {
        NodeStatus rootResult = m_RuntimeRoot->Tick();
        if (rootResult == NodeStatus::Success) {
            m_LastResultText = "[成功]";
            m_LastResultColor = ImVec4(0, 1, 0, 1);
        } else if (rootResult == NodeStatus::Failure) {
            m_LastResultText = "[失敗]";
            m_LastResultColor = ImVec4(1, 0, 0, 1);
        }
        for (auto const &[nodeId, runtimeNode] : m_nodeInstanceMap) {
            if (runtimeNode->GetStatus() != NodeStatus::Idle)
                m_statusTimers[nodeId] = 0.5f;
        }
    }
    float dt = ImGui::GetIO().DeltaTime;
    for (auto it = m_statusTimers.begin(); it != m_statusTimers.end();) {
        it->second -= dt;
        if (it->second <= 0.0f)
            it = m_statusTimers.erase(it);
        else
            ++it;
    }

    // --- ノードエディタ ---
    ed::Begin("My Behavior Tree", ImVec2(0, 0));

    for (auto &node : m_Nodes) {
        int nodeId = (int)node.ID.Get();
        NodeStatus status = NodeStatus::Idle;
        bool showHighlight = false;
        if (m_IsRunning && m_nodeInstanceMap.count(nodeId))
            status = m_nodeInstanceMap[nodeId]->GetStatus();
        if (status != NodeStatus::Idle || m_statusTimers.count(nodeId)) {
            showHighlight = true;
            if (status == NodeStatus::Idle && m_nodeInstanceMap.count(nodeId))
                status = m_nodeInstanceMap[nodeId]->GetStatus();
        }

        if (showHighlight) {
            if (status == NodeStatus::Running) {
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.8f, 0.6f, 0.1f, 1.0f));
                ed::PushStyleColor(ed::StyleColor_NodeBorder, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            } else if (status == NodeStatus::Success)
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            else if (status == NodeStatus::Failure)
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        }

        ed::BeginNode(node.ID);
        ImGui::Text("%s", node.Title.c_str());

        ed::BeginPin(node.InputPinID, ed::PinKind::Input);
        ImGui::Text("-> In");
        ed::EndPin();

        // ★修正: パラメータUI
        ImGui::PushItemWidth(80);

        if (node.Type == EditorNodeType::ConditionPlayerClose) {
            ImGui::Text("Min Dist");
            ImGui::SameLine();
            ImGui::DragFloat("##min", &node.Parameter, 0.1f, 0.0f, 100.0f, "%.1fm");
            ImGui::Text("Max Dist");
            ImGui::SameLine();
            ImGui::DragFloat("##max", &node.Parameter2, 0.1f, 0.0f, 100.0f, "%.1fm");
        } else if (node.Type == EditorNodeType::ConditionHealthLow) {
            ImGui::Text("HP Ratio");
            ImGui::SameLine();
            ImGui::DragFloat("##hp", &node.Parameter, 0.01f, 0.0f, 1.0f, "%.2f");
        }
        // 移動系アクションのパラメータ (Time Min/Max, Speed)
        else if (node.Type == EditorNodeType::ActionApproach ||
                 node.Type == EditorNodeType::ActionDash ||
                 node.Type == EditorNodeType::ActionStrafe ||
                 node.Type == EditorNodeType::ActionRetreat) {

            ImGui::Text("Time Min");
            ImGui::SameLine();
            ImGui::DragFloat("##tmin", &node.Parameter, 0.1f, 0.0f, 10.0f, "%.1fs");
            ImGui::Text("Time Max");
            ImGui::SameLine();
            ImGui::DragFloat("##tmax", &node.Parameter2, 0.1f, 0.0f, 10.0f, "%.1fs");
            ImGui::Text("Speed");
            ImGui::SameLine();
            ImGui::DragFloat("##spd", &node.Parameter3, 0.01f, 0.0f, 5.0f, "%.2f");
        }

        ImGui::PopItemWidth();

        // Outピン表示判定
        bool isLeaf = (node.Type == EditorNodeType::ActionRun ||
                       node.Type == EditorNodeType::ConditionPlayerClose ||
                       node.Type == EditorNodeType::ConditionHealthLow ||
                       node.Type == EditorNodeType::ActionApproach ||
                       node.Type == EditorNodeType::ActionDash ||
                       node.Type == EditorNodeType::ActionStrafe ||
                       node.Type == EditorNodeType::ActionRetreat ||
                       node.Type == EditorNodeType::ActionAttack);

        if (!isLeaf) {
            ImGui::SameLine();
            ed::BeginPin(node.OutputPinID, ed::PinKind::Output);
            ImGui::Text("Out ->");
            ed::EndPin();
        }
        ed::EndNode();

        if (showHighlight)
            ed::PopStyleColor(status == NodeStatus::Running ? 2 : 1);
    }

    for (auto &link : m_Links)
        ed::Link(link.ID, link.StartPinID, link.EndPinID);

    static ed::LinkId contextLinkId;
    if (ed::ShowLinkContextMenu(&contextLinkId))
        ImGui::OpenPopup("LinkContextMenu");
    if (ImGui::BeginPopup("LinkContextMenu")) {
        if (ImGui::MenuItem("Delete Link"))
            ed::DeleteLink(contextLinkId);
        ImGui::EndPopup();
    }

    HandleCreateAction();
    DeleteSelectedItems();

    ed::Suspend();
    if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("CreateNodeMenu");
        Vector2 mousePos = Input::GetInstance()->GetMousePos();
        m_CreatePos = ed::ScreenToCanvas(ImVec2(mousePos.x, mousePos.y));
    }
    if (ImGui::BeginPopup("CreateNodeMenu")) {
        if (ImGui::MenuItem("Sequence"))
            CreateNode("Sequence", EditorNodeType::Sequence);
        if (ImGui::MenuItem("Selector"))
            CreateNode("Selector", EditorNodeType::Selector);
        ImGui::Separator();
        if (ImGui::MenuItem("Check: Range"))
            CreateNode("Check Dist", EditorNodeType::ConditionPlayerClose);
        if (ImGui::MenuItem("Check: Health Low"))
            CreateNode("Check HP", EditorNodeType::ConditionHealthLow);
        ImGui::Separator();
        if (ImGui::MenuItem("Act: Approach"))
            CreateNode("Approach", EditorNodeType::ActionApproach);
        if (ImGui::MenuItem("Act: Dash"))
            CreateNode("Dash", EditorNodeType::ActionDash);
        if (ImGui::MenuItem("Act: Strafe"))
            CreateNode("Strafe", EditorNodeType::ActionStrafe);
        if (ImGui::MenuItem("Act: Retreat"))
            CreateNode("Retreat", EditorNodeType::ActionRetreat);

        if (ImGui::MenuItem("Act: Attack"))
            CreateNode("Attack", EditorNodeType::ActionAttack);
        ImGui::EndPopup();
    }
    ed::Resume();
    ed::End();
    ed::SetCurrentEditor(nullptr);
}

// ---------------------------------------------------------
// Helper functions
// ---------------------------------------------------------
void BehaviorTreeEditor::HandleCreateAction() {
    if (ed::BeginCreate()) {
        ed::PinId startPinId, endPinId;
        if (ed::QueryNewLink(&startPinId, &endPinId)) {
            if (IsInputPin(startPinId) == IsInputPin(endPinId))
                ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
            else if (ed::AcceptNewItem())
                m_Links.emplace_back(m_NextLinkId++, startPinId, endPinId);
        }
    }
    ed::EndCreate();
}

void BehaviorTreeEditor::DeleteSelectedItems() {
    if (ed::BeginDelete()) {
        ed::NodeId nodeId;
        while (ed::QueryDeletedNode(&nodeId)) {
            if (ed::AcceptDeletedItem()) {
                auto it = std::remove_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto &n) { return n.ID == nodeId; });
                m_Nodes.erase(it, m_Nodes.end());
            }
        }
        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            if (ed::AcceptDeletedItem()) {
                auto it = std::remove_if(m_Links.begin(), m_Links.end(), [linkId](auto &l) { return l.ID == linkId; });
                m_Links.erase(it, m_Links.end());
            }
        }
    }
    ed::EndDelete();
}

void BehaviorTreeEditor::CreateNode(const std::string &title, EditorNodeType type) {
    int id = m_NextNodeId++;
    EditorNode node(id, title, type);
    m_Nodes.push_back(node);
    ed::SetNodePosition(node.ID, m_CreatePos);
}

void BehaviorTreeEditor::BuildAndRunTree() {
    m_nodeInstanceMap.clear();
    m_RuntimeRoot = nullptr;
    int rootId = FindRootNodeId();
    if (rootId == -1)
        return;
    m_RuntimeRoot = BuildNodeRecursive(rootId);
    if (m_RuntimeRoot)
        m_RuntimeRoot->SetContext(m_DebugEnemy, m_DebugPlayer);
}

std::shared_ptr<BTNode> BehaviorTreeEditor::BuildNodeRecursive(int editorNodeId) {
    auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(), [editorNodeId](const EditorNode &n) { return (int)n.ID.Get() == editorNodeId; });
    if (it == m_Nodes.end())
        return nullptr;
    const EditorNode &eNode = *it;

    std::shared_ptr<BTNode> runtimeNode = nullptr;
    switch (eNode.Type) {
    case EditorNodeType::Sequence:
        runtimeNode = std::make_shared<SequenceNode>();
        break;
    case EditorNodeType::Selector:
        runtimeNode = std::make_shared<SelectorNode>();
        break;
    case EditorNodeType::ActionRun:
        runtimeNode = std::make_shared<RunActionNode>();
        break;

    case EditorNodeType::ConditionPlayerClose:
        runtimeNode = std::make_shared<IsPlayerCloseNode>(eNode.Parameter, eNode.Parameter2);
        break;
    case EditorNodeType::ConditionHealthLow:
        runtimeNode = std::make_shared<IsHealthLowNode>(eNode.Parameter);
        break;

    // 移動系 (MinTime, MaxTime, Speed を渡す)
    case EditorNodeType::ActionApproach:
        runtimeNode = std::make_shared<EnemyApproachNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3);
        break;
    case EditorNodeType::ActionDash:
        runtimeNode = std::make_shared<EnemyDashNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3);
        break;
    case EditorNodeType::ActionStrafe:
        runtimeNode = std::make_shared<EnemyStrafeNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3);
        break;
    case EditorNodeType::ActionRetreat:
        runtimeNode = std::make_shared<EnemyRetreatNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3);
        break;

    case EditorNodeType::ActionAttack:
        runtimeNode = std::make_shared<EnemyAttackNode>();
        break;
    }

    if (!runtimeNode)
        return nullptr;
    m_nodeInstanceMap[editorNodeId] = runtimeNode;

    bool isLeaf = (eNode.Type == EditorNodeType::ActionRun || eNode.Type == EditorNodeType::ConditionPlayerClose || eNode.Type == EditorNodeType::ConditionHealthLow || eNode.Type == EditorNodeType::ActionApproach || eNode.Type == EditorNodeType::ActionAttack);
    if (!isLeaf) {
        int outputPinId = (int)eNode.OutputPinID.Get();
        std::vector<int> childIds = FindChildrenNodeIds(outputPinId);
        for (int childId : childIds) {
            auto childNode = BuildNodeRecursive(childId);
            if (childNode)
                runtimeNode->AddChild(childNode);
        }
    }
    return runtimeNode;
}

std::vector<int> BehaviorTreeEditor::FindChildrenNodeIds(int outputPinId) {
    std::vector<int> children;
    for (const auto &link : m_Links) {
        if ((int)link.StartPinID.Get() == outputPinId) {
            int endPin = (int)link.EndPinID.Get();
            children.push_back((endPin - 1) / 10);
        }
    }
    return children;
}

int BehaviorTreeEditor::FindRootNodeId() {
    for (const auto &node : m_Nodes) {
        bool hasInput = false;
        int inputPin = (int)node.InputPinID.Get();
        for (const auto &link : m_Links) {
            if ((int)link.EndPinID.Get() == inputPin) {
                hasInput = true;
                break;
            }
        }
        if (!hasInput)
            return (int)node.ID.Get();
    }
    return -1;
}

void BehaviorTreeEditor::SaveTree() {
    std::string fileName = m_InputFileNameBuf;
    if (fileName.empty())
        fileName = "NewBehavior";
    DataHandler handler("BehaviorTree", fileName);

    json nodesJson = json::array();
    for (const auto &node : m_Nodes) {
        json n;
        n["id"] = (int)node.ID.Get();
        n["title"] = node.Title;
        n["type"] = (int)node.Type;
        ImVec2 pos = ed::GetNodePosition(node.ID);
        n["x"] = pos.x;
        n["y"] = pos.y;
        n["param"] = node.Parameter;
        n["param2"] = node.Parameter2;
        n["param3"] = node.Parameter3;
        nodesJson.push_back(n);
    }
    handler.Save("nodes", nodesJson);

    json linksJson = json::array();
    for (const auto &link : m_Links) {
        json l;
        l["id"] = (int)link.ID.Get();
        l["start"] = (int)link.StartPinID.Get();
        l["end"] = (int)link.EndPinID.Get();
        linksJson.push_back(l);
    }
    handler.Save("links", linksJson);
    std::cout << "Saved BT: " << fileName << ".json" << std::endl;
}

void BehaviorTreeEditor::LoadTree(const std::string &filePath) {
    std::string folderName, fileName;
    ParsePathToFolderAndFile(filePath, folderName, fileName);
    if (fileName.size() < sizeof(m_InputFileNameBuf))
        strcpy_s(m_InputFileNameBuf, fileName.c_str());

    DataHandler handler(folderName, fileName);
    m_Nodes.clear();
    m_Links.clear();

    json nodesJson = handler.Load("nodes", json::array());
    int maxNodeId = 0;
    for (const auto &n : nodesJson) {
        int id = n["id"].get<int>();
        std::string title = n["title"].get<std::string>();
        EditorNodeType type = (EditorNodeType)n["type"].get<int>();
        float x = n["x"].get<float>();
        float y = n["y"].get<float>();
        float param = n.value("param", 0.0f);
        float param2 = n.value("param2", 0.0f);
        float param3 = n.value("param3", 0.0f);

        EditorNode node(id, title, type);
        node.Parameter = param;
        node.Parameter2 = param2;
        m_Nodes.push_back(node);
        ed::SetNodePosition(node.ID, ImVec2(x, y));
        if (id > maxNodeId)
            maxNodeId = id;
    }
    m_NextNodeId = maxNodeId + 1;

    json linksJson = handler.Load("links", json::array());
    int maxLinkId = 0;
    for (const auto &l : linksJson) {
        int id = l["id"].get<int>();
        int start = l["start"].get<int>();
        int end = l["end"].get<int>();
        m_Links.emplace_back(id, ed::PinId(start), ed::PinId(end));
        if (id > maxLinkId)
            maxLinkId = id;
    }
    m_NextLinkId = maxLinkId + 1;
    std::cout << "Loaded BT: " << fileName << std::endl;
}
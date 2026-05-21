#define NOMINMAX
#include "BehaviorTreeEditor.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================
// ピンID衝突防止オフセット
// ============================================================
static constexpr int kPinOffset = 100000;

// ============================================================
// BehaviorTreeLoader  (Debug / Release 共通実装)
// ============================================================

namespace {
// ピンIDからInputPinかどうかを判定 (kPinOffset適用済み)
inline bool IsInputPinStatic(int pinId) { return ((pinId - kPinOffset) % 10) == 1; }
} // anonymous namespace

int BehaviorTreeLoader::FindRootNodeId(
    const std::vector<NodeData> &nodes,
    const std::vector<LinkData> &links) {
    // 全てのノードを走査し、どこからも入力されていないノードをルートとみなす
    for (const auto &node : nodes) {
        int inputPin = node.id * 10 + 1 + kPinOffset;
        bool hasInput = false;
        for (const auto &link : links) {
            if (link.endPin == inputPin) {
                hasInput = true;
                break;
            }
        }
        if (!hasInput)
            return node.id;
    }
    return -1;
}

std::vector<int> BehaviorTreeLoader::FindChildrenNodeIds(
    int outputPinId,
    const std::vector<LinkData> &links) {
    std::vector<int> children;
    // 指定された出力ピンから伸びているリンクを探し、接続先ノードのIDを収集
    for (const auto &link : links) {
        if (link.startPin == outputPinId) {
            children.push_back((link.endPin - kPinOffset - 1) / 10);
        }
    }
    return children;
}

std::vector<std::pair<int, float>> BehaviorTreeLoader::FindWeightedChildrenNodeIds(
    const NodeData &node,
    const std::vector<LinkData> &links) {
    std::vector<std::pair<int, float>> result;
    // 重み付き出力ピンごとに接続されている子ノードを探す
    for (const auto &wp : node.weightedOutputs) {
        for (const auto &link : links) {
            if (link.startPin == wp.pinId) {
                int childId = (link.endPin - kPinOffset - 1) / 10;
                result.emplace_back(childId, wp.weight);
            }
        }
    }
    return result;
}

std::shared_ptr<BTNode> BehaviorTreeLoader::BuildNodeRecursive(
    int nodeId,
    const std::vector<NodeData> &nodes,
    const std::vector<LinkData> &links) {
    auto it = std::find_if(nodes.begin(), nodes.end(),
                           [nodeId](const NodeData &n) { return n.id == nodeId; });
    if (it == nodes.end())
        return nullptr;

    const NodeData &nd = *it;
    std::shared_ptr<BTNode> runtimeNode;

    switch (nd.type) {
    case EditorNodeType::Sequence:
        runtimeNode = std::make_shared<SequenceNode>();
        break;
    case EditorNodeType::SequenceOnce:
        runtimeNode = std::make_shared<SequenceOnceNode>();
        break;
    case EditorNodeType::Selector:
        runtimeNode = std::make_shared<SelectorNode>();
        break;
    case EditorNodeType::SelectorRandom:
        runtimeNode = std::make_shared<RandomSelectorNode>();
        break;
    case EditorNodeType::DecoratorWeight:
        runtimeNode = std::make_shared<RandomSelectorNode>();
        break;
    case EditorNodeType::ActionRun:
        runtimeNode = std::make_shared<RunActionNode>();
        break;
    case EditorNodeType::ConditionPlayerClose:
        runtimeNode = std::make_shared<IsPlayerCloseNode>(nd.param, nd.param2);
        break;
    case EditorNodeType::ConditionHealthLow:
        runtimeNode = std::make_shared<IsHealthLowNode>(nd.param);
        break;
    case EditorNodeType::ConditionEnergyLow:
        runtimeNode = std::make_shared<IsEnergyLowNode>(nd.param);
        break;
    case EditorNodeType::ConditionIsGrounded:
        runtimeNode = std::make_shared<IsGroundedNode>();
        break;
    case EditorNodeType::ConditionIsAirborne:
        runtimeNode = std::make_shared<IsAirborneNode>();
        break;
    case EditorNodeType::ConditionPlayerState:
        runtimeNode = std::make_shared<IsPlayerStateNode>(nd.stateName);
        break;
    case EditorNodeType::ActionShoot:
        runtimeNode = std::make_shared<EnemyShootNode>(nd.param);
        break;
    case EditorNodeType::ActionLockOn:
        runtimeNode = std::make_shared<EnemyLockOnNode>(nd.param >= 1.0f);
        break;
    case EditorNodeType::ConditionIsLockOn:
        runtimeNode = std::make_shared<IsEnemyLockOnNode>();
        break;
    case EditorNodeType::ActionComboStep:
        runtimeNode = std::make_shared<EnemyComboStepNode>(
            nd.param > 0.0f ? nd.param : 0.5f,
            nd.param2);
        break;
    case EditorNodeType::ActionComboFull:
        runtimeNode = std::make_shared<EnemyComboFullNode>(
            nd.param > 0.0f ? nd.param : 0.5f,
            static_cast<int>(nd.param2),
            nd.param3);
        break;
    case EditorNodeType::ActionBurstShoot:
        runtimeNode = std::make_shared<EnemyBurstShootNode>(
            nd.param > 0.0f ? nd.param : 0.2f,
            nd.param2 > 0.0f ? static_cast<int>(nd.param2) : 3,
            nd.param3 > 0.0f ? nd.param3 : 0.5f,
            nd.param4,
            nd.param5 >= 1.0f);
        break;
    case EditorNodeType::ActionEnergyCharge:
        runtimeNode = std::make_shared<EnemyEnergyChargeNode>(
            nd.param > 0.0f ? nd.param : 1.0f,
            nd.param2 > 0.0f ? nd.param2 : 1.0f);
        break;
    case EditorNodeType::ActionApproach:
        runtimeNode = std::make_shared<EnemyApproachNode>(nd.param, nd.param2, nd.param3);
        break;
    case EditorNodeType::ActionDash:
        runtimeNode = std::make_shared<EnemyDashNode>(nd.param, nd.param2, nd.param3);
        break;
    case EditorNodeType::ActionStrafe:
        runtimeNode = std::make_shared<EnemyStrafeNode>(nd.param, nd.param2, nd.param3);
        break;
    case EditorNodeType::ActionRetreat:
        runtimeNode = std::make_shared<EnemyRetreatNode>(nd.param, nd.param2, nd.param3);
        break;
    case EditorNodeType::ActionAttack:
        runtimeNode = std::make_shared<EnemyAttackNode>();
        break;
    case EditorNodeType::ActionIdle:
        runtimeNode = std::make_shared<EnemyIdleNode>(nd.param);
        break;
    case EditorNodeType::ActionJump:
        runtimeNode = std::make_shared<EnemyJumpNode>(nd.param);
        break;
    case EditorNodeType::ActionJumpToFly:
        runtimeNode = std::make_shared<EnemyJumpToFlyNode>(nd.param);
        break;
    case EditorNodeType::ActionFlyAscend:
        runtimeNode = std::make_shared<EnemyFlyAscendNode>(nd.param, nd.param2, nd.param3);
        break;
    case EditorNodeType::ActionFlyDescend:
        runtimeNode = std::make_shared<EnemyFlyDescendNode>(nd.param, nd.param2, nd.param3);
        break;
    case EditorNodeType::ActionFlyToGround:
        runtimeNode = std::make_shared<EnemyFlyToGroundNode>();
        break;
    case EditorNodeType::ActionFlyApproach:
        runtimeNode = std::make_shared<EnemyFlyApproachNode>(nd.param, nd.param2, nd.param3);
        break;
    default:
        return nullptr;
    }

    if (!runtimeNode)
        return nullptr;

    bool isLeaf = (nd.type == EditorNodeType::ActionRun ||
                   nd.type == EditorNodeType::ConditionPlayerClose ||
                   nd.type == EditorNodeType::ConditionHealthLow ||
                   nd.type == EditorNodeType::ConditionIsGrounded ||
                   nd.type == EditorNodeType::ConditionIsAirborne ||
                   nd.type == EditorNodeType::ConditionPlayerState ||
                   nd.type == EditorNodeType::ConditionIsLockOn ||
                   nd.type == EditorNodeType::ActionApproach ||
                   nd.type == EditorNodeType::ActionDash ||
                   nd.type == EditorNodeType::ActionStrafe ||
                   nd.type == EditorNodeType::ActionRetreat ||
                   nd.type == EditorNodeType::ActionAttack ||
                   nd.type == EditorNodeType::ActionIdle ||
                   nd.type == EditorNodeType::ActionJump ||
                   nd.type == EditorNodeType::ActionJumpToFly ||
                   nd.type == EditorNodeType::ActionFlyAscend ||
                   nd.type == EditorNodeType::ActionFlyDescend ||
                   nd.type == EditorNodeType::ActionFlyToGround ||
                   nd.type == EditorNodeType::ActionFlyApproach ||
                   nd.type == EditorNodeType::ActionShoot ||
                   nd.type == EditorNodeType::ActionLockOn ||
                   nd.type == EditorNodeType::ActionComboStep ||
                   nd.type == EditorNodeType::ActionComboFull ||
                   nd.type == EditorNodeType::ActionBurstShoot ||
                   nd.type == EditorNodeType::ConditionEnergyLow ||
                   nd.type == EditorNodeType::ActionEnergyCharge);

    if (!isLeaf) {
        if (nd.type == EditorNodeType::DecoratorWeight) {
            auto weightedChildren = FindWeightedChildrenNodeIds(nd, links);
            for (auto &[childId, weight] : weightedChildren) {
                auto childNode = BuildNodeRecursive(childId, nodes, links);
                if (childNode) {
                    auto decorator = std::make_shared<WeightDecoratorNode>(weight);
                    decorator->AddChild(childNode);
                    runtimeNode->AddChild(decorator);
                }
            }
        } else {
            int outputPin = nd.id * 10 + 2 + kPinOffset;
            for (int childId : FindChildrenNodeIds(outputPin, links)) {
                auto childNode = BuildNodeRecursive(childId, nodes, links);
                if (childNode)
                    runtimeNode->AddChild(childNode);
            }
        }
    } else if (nd.type == EditorNodeType::ConditionPlayerClose ||
               nd.type == EditorNodeType::ConditionHealthLow ||
               nd.type == EditorNodeType::ConditionEnergyLow ||
               nd.type == EditorNodeType::ConditionIsGrounded ||
               nd.type == EditorNodeType::ConditionIsAirborne ||
               nd.type == EditorNodeType::ConditionPlayerState) {
        int successPin = nd.id * 10 + 3 + kPinOffset;
        int failurePin = nd.id * 10 + 4 + kPinOffset;
        auto successChildIds = FindChildrenNodeIds(successPin, links);
        auto failureChildIds = FindChildrenNodeIds(failurePin, links);

        if (!successChildIds.empty() || !failureChildIds.empty()) {
            auto selectorWrapper = std::make_shared<SelectorNode>();

            if (!successChildIds.empty()) {
                auto successSeq = std::make_shared<SequenceNode>();
                std::shared_ptr<BTNode> condCopy;
                switch (nd.type) {
                case EditorNodeType::ConditionPlayerClose:
                    condCopy = std::make_shared<IsPlayerCloseNode>(nd.param, nd.param2);
                    break;
                case EditorNodeType::ConditionHealthLow:
                    condCopy = std::make_shared<IsHealthLowNode>(nd.param);
                    break;
                case EditorNodeType::ConditionIsGrounded:
                    condCopy = std::make_shared<IsGroundedNode>();
                    break;
                case EditorNodeType::ConditionIsAirborne:
                    condCopy = std::make_shared<IsAirborneNode>();
                    break;
                case EditorNodeType::ConditionPlayerState:
                    condCopy = std::make_shared<IsPlayerStateNode>(nd.stateName);
                    break;
                case EditorNodeType::ConditionIsLockOn:
                    condCopy = std::make_shared<IsEnemyLockOnNode>();
                    break;
                case EditorNodeType::ConditionEnergyLow:
                    condCopy = std::make_shared<IsEnergyLowNode>(nd.param);
                    break;
                default:
                    break;
                }
                if (condCopy) {
                    successSeq->AddChild(condCopy);
                    for (int cid : successChildIds) {
                        auto c = BuildNodeRecursive(cid, nodes, links);
                        if (c)
                            successSeq->AddChild(c);
                    }
                    selectorWrapper->AddChild(successSeq);
                }
            }

            for (int cid : failureChildIds) {
                auto c = BuildNodeRecursive(cid, nodes, links);
                if (c)
                    selectorWrapper->AddChild(c);
            }

            return selectorWrapper;
        }
    }

    return runtimeNode;
}

std::shared_ptr<BTNode> BehaviorTreeLoader::LoadAndBuild(
    const std::string &folder,
    const std::string &file) {
    DataHandler handler(folder, file);
    if (!handler.Exists()) {
        std::cerr << "[BehaviorTreeLoader] ファイルが見つかりません: "
                  << folder << "/" << file << ".json" << std::endl;
        return nullptr;
    }

    json nodesJson = handler.Load("nodes", json::array());
    json linksJson = handler.Load("links", json::array());

    std::vector<NodeData> nodes;
    for (const auto &n : nodesJson) {
        NodeData nd;
        nd.id = n["id"].get<int>();
        nd.type = static_cast<EditorNodeType>(n["type"].get<int>());
        nd.param = n.value("param", 0.0f);
        nd.param2 = n.value("param2", 0.0f);
        nd.param3 = n.value("param3", 0.0f);
        nd.param4 = n.value("param4", 0.0f);
        nd.param5 = n.value("param5", 0.0f);

        if (nd.type == EditorNodeType::ConditionPlayerState && n.contains("stateName"))
            nd.stateName = n["stateName"].get<std::string>();

        if (nd.type == EditorNodeType::DecoratorWeight && n.contains("weightedOutputs")) {
            for (const auto &w : n["weightedOutputs"]) {
                NodeData::WeightPin wp;
                wp.pinId = w["pinId"].get<int>();
                wp.weight = w["weight"].get<float>();
                nd.weightedOutputs.push_back(wp);
            }
        }
        nodes.push_back(nd);
    }

    std::vector<LinkData> links;
    for (const auto &l : linksJson) {
        LinkData ld;
        ld.startPin = l["start"].get<int>();
        ld.endPin = l["end"].get<int>();
        links.push_back(ld);
    }

    int rootId = FindRootNodeId(nodes, links);
    if (rootId == -1) {
        std::cerr << "[BehaviorTreeLoader] ルートノードが見つかりません" << std::endl;
        return nullptr;
    }

    auto root = BuildNodeRecursive(rootId, nodes, links);
    if (!root) {
        std::cerr << "[BehaviorTreeLoader] ツリーのビルドに失敗しました" << std::endl;
    }
    return root;
}

#ifdef _DEBUG
#include "Input.h"
#include <ShowFolder/ShowFolder.h>
#include <imgui_internal.h>

namespace ed = ax::NodeEditor;

EditorNode::EditorNode(int id, const std::string &title, EditorNodeType type)
    : ID(id), Title(title), Type(type) {
    InputPinID = id * 10 + 1 + kPinOffset;
    OutputPinID = id * 10 + 2 + kPinOffset;
    SuccessPinID = id * 10 + 3 + kPinOffset;
    FailurePinID = id * 10 + 4 + kPinOffset;

    if (type == EditorNodeType::ConditionPlayerClose) {
        Parameter = 0.0f;
        Parameter2 = 10.0f;
    } else if (type == EditorNodeType::DecoratorWeight) {
        WeightedOutputs.resize(2);
        WeightedOutputs[0].PinID = id * 10 + 5 + kPinOffset;
        WeightedOutputs[0].Weight = 1.0f;
        WeightedOutputs[1].PinID = id * 10 + 6 + kPinOffset;
        WeightedOutputs[1].Weight = 1.0f;
    } else if (type == EditorNodeType::ConditionHealthLow) {
        Parameter = 0.3f;
    } else if (type == EditorNodeType::ConditionEnergyLow) {
        Parameter = 0.3f;
    } else if (type == EditorNodeType::ActionEnergyCharge) {
        Parameter = 1.0f;
        Parameter2 = 1.0f;
    } else if (type == EditorNodeType::ConditionPlayerState) {
        StateNameParameter = "Idle";
    } else if (type == EditorNodeType::ActionApproach) {
        Parameter = 1.0f;
        Parameter2 = 3.0f;
        Parameter3 = 0.1f;
    } else if (type == EditorNodeType::ActionDash) {
        Parameter = 0.5f;
        Parameter2 = 1.5f;
        Parameter3 = 0.3f;
    } else if (type == EditorNodeType::ActionStrafe) {
        Parameter = 1.0f;
        Parameter2 = 2.0f;
        Parameter3 = 0.08f;
    } else if (type == EditorNodeType::ActionRetreat) {
        Parameter = 1.0f;
        Parameter2 = 2.0f;
        Parameter3 = 0.15f;
    } else if (type == EditorNodeType::ActionIdle) {
        Parameter = 1.0f;
    } else if (type == EditorNodeType::ActionJump ||
               type == EditorNodeType::ActionJumpToFly) {
        Parameter = 15.0f;
    } else if (type == EditorNodeType::ActionFlyAscend) {
        Parameter = 1.0f;
        Parameter2 = 3.0f;
        Parameter3 = 15.0f;
    } else if (type == EditorNodeType::ActionFlyDescend) {
        Parameter = 1.0f;
        Parameter2 = 3.0f;
        Parameter3 = 15.0f;
    } else if (type == EditorNodeType::ActionFlyApproach) {
        Parameter = 1.0f;
        Parameter2 = 3.0f;
        Parameter3 = 10.0f;
    } else if (type == EditorNodeType::ActionShoot) {
        Parameter = 1.0f;
    } else if (type == EditorNodeType::ActionLockOn) {
        Parameter = 1.0f;
    } else if (type == EditorNodeType::ActionComboStep) {
        Parameter = 0.5f;
        Parameter2 = 0.0f;
    } else if (type == EditorNodeType::ActionComboFull) {
        Parameter = 0.5f;
        Parameter2 = 0.0f;
        Parameter3 = 0.0f;
    } else if (type == EditorNodeType::ActionBurstShoot) {
        Parameter = 0.2f;
        Parameter2 = 3.0f;
        Parameter3 = 0.5f;
        Parameter4 = 0.0f;
        Parameter5 = 0.0f;
    }
}

EditorLink::EditorLink(int id, ed::PinId start, ed::PinId end)
    : ID(id), StartPinID(start), EndPinID(end) {}

BehaviorTreeEditor::BehaviorTreeEditor() {
    ed::Config config;
    config.SettingsFile = nullptr;
    m_Context = ed::CreateEditor(&config);
}

BehaviorTreeEditor::~BehaviorTreeEditor() {
    if (m_Context)
        ed::DestroyEditor(m_Context);
}

bool BehaviorTreeEditor::IsInputPin(ed::PinId p) { return ((int)p.Get() - kPinOffset) % 10 == 1; }
bool BehaviorTreeEditor::IsOutputPin(ed::PinId p) { return ((int)p.Get() - kPinOffset) % 10 == 2; }
bool BehaviorTreeEditor::IsSuccessPin(ed::PinId p) { return ((int)p.Get() - kPinOffset) % 10 == 3; }
bool BehaviorTreeEditor::IsFailurePin(ed::PinId p) { return ((int)p.Get() - kPinOffset) % 10 == 4; }

bool BehaviorTreeEditor::IsWeightedOutputPin(ed::PinId pinId, int &outNodeId, int &outOutputIndex) {
    for (auto &node : m_Nodes) {
        if (node.IsWeightNode()) {
            for (int i = 0; i < (int)node.WeightedOutputs.size(); ++i) {
                if (node.WeightedOutputs[i].PinID == pinId) {
                    outNodeId = (int)node.ID.Get();
                    outOutputIndex = i;
                    return true;
                }
            }
        }
    }
    return false;
}

void BehaviorTreeEditor::ParsePathToFolderAndFile(
    const std::string &fullPath, std::string &outFolder, std::string &outFile) {
    fs::path path(fullPath);
    outFile = path.stem().string();
    fs::path parent = path.parent_path();
    outFolder = parent.has_filename() ? parent.filename().string() : "BehaviorTree";
}

const char *BehaviorTreeEditor::GetNodeDescription(EditorNodeType type) {
    switch (type) {
    case EditorNodeType::Sequence: return "子ノードを順番に実行し、全て成功で成功を返す";
    case EditorNodeType::SequenceOnce: return "Non-Reactive Sequence: Action runs to completion, ignores condition changes";
    case EditorNodeType::Selector: return "子ノードを順番に試し、1つでも成功したら成功を返す";
    case EditorNodeType::SelectorRandom: return "子ノードの中からランダムに1つを選択して実行する";
    case EditorNodeType::DecoratorWeight: return "複数の行動に重み付けしてランダム選択する";
    case EditorNodeType::ActionRun: return "一定時間実行する基本アクション";
    case EditorNodeType::ConditionPlayerClose: return "プレイヤーが指定距離範囲内にいるかチェック";
    case EditorNodeType::ConditionHealthLow: return "HPが指定割合以下かチェック";
    case EditorNodeType::ConditionEnergyLow: return "エネルギーが指定割合以下かチェック (param: 閾値比率 0.0〜1.0)";
    case EditorNodeType::ConditionIsGrounded: return "敵が地上にいるかチェック";
    case EditorNodeType::ConditionIsAirborne: return "敵が空中にいるかチェック";
    case EditorNodeType::ConditionPlayerState: return "プレイヤーが指定されたステートかチェック";
    case EditorNodeType::ActionApproach: return "ターゲットに向かって通常速度で接近する";
    case EditorNodeType::ActionDash: return "ターゲットに向かって高速で接近する";
    case EditorNodeType::ActionStrafe: return "ターゲットの周囲を左右に移動する";
    case EditorNodeType::ActionRetreat: return "ターゲットから後退する";
    case EditorNodeType::ActionAttack: return "攻撃を実行する";
    case EditorNodeType::ActionIdle: return "その場で待機する(何もしない)";
    case EditorNodeType::ActionJump: return "地上からジャンプする";
    case EditorNodeType::ActionJumpToFly: return "地上からジャンプし、飛行状態へ遷移";
    case EditorNodeType::ActionFlyAscend: return "飛行中に上昇する";
    case EditorNodeType::ActionFlyDescend: return "飛行中に下降する";
    case EditorNodeType::ActionFlyToGround: return "飛行状態から地上へ着地する";
    case EditorNodeType::ActionFlyApproach: return "飛行中に水平方向へプレイヤーへ接近する";
    case EditorNodeType::ActionShoot: return "弾を1発発射し、指定秒数クールダウンする";
    case EditorNodeType::ActionLockOn: return "ロックオン状態を切り替える";
    case EditorNodeType::ConditionIsLockOn: return "ロックオン中かどうかをチェックする";
    case EditorNodeType::ActionComboStep: return "コンボを1段だけ実行する";
    case EditorNodeType::ActionComboFull: return "コンボを全段実行する";
    case EditorNodeType::ActionBurstShoot: return "弾をN連発する";
    case EditorNodeType::ActionEnergyCharge: return "エネルギーをチャージする";
    default: return "説明なし";
    }
}

void BehaviorTreeEditor::BuildAndRunTree() {
    m_nodeInstanceMap.clear();
    m_RuntimeRoot = nullptr;
    int rootId = FindRootNodeId();
    if (rootId == -1) return;
    m_RuntimeRoot = BuildNodeRecursive(rootId);
    if (m_RuntimeRoot) {
        m_RuntimeRoot->SetContext(m_DebugEnemy, m_DebugPlayer);
        if (m_DebugEnemy) m_DebugEnemy->SetBehaviorTree(m_RuntimeRoot);
    }
}

std::shared_ptr<BTNode> BehaviorTreeEditor::BuildNodeRecursive(int editorNodeId) {
    auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(),
                           [editorNodeId](const EditorNode &n) { return (int)n.ID.Get() == editorNodeId; });
    if (it == m_Nodes.end()) return nullptr;
    const EditorNode &eNode = *it;
    std::shared_ptr<BTNode> runtimeNode;

    switch (eNode.Type) {
    case EditorNodeType::Sequence: runtimeNode = std::make_shared<SequenceNode>(); break;
    case EditorNodeType::SequenceOnce: runtimeNode = std::make_shared<SequenceOnceNode>(); break;
    case EditorNodeType::Selector: runtimeNode = std::make_shared<SelectorNode>(); break;
    case EditorNodeType::ActionRun: runtimeNode = std::make_shared<RunActionNode>(); break;
    case EditorNodeType::ConditionPlayerClose: runtimeNode = std::make_shared<IsPlayerCloseNode>(eNode.Parameter, eNode.Parameter2); break;
    case EditorNodeType::ConditionHealthLow: runtimeNode = std::make_shared<IsHealthLowNode>(eNode.Parameter); break;
    case EditorNodeType::ConditionIsGrounded: runtimeNode = std::make_shared<IsGroundedNode>(); break;
    case EditorNodeType::ConditionIsAirborne: runtimeNode = std::make_shared<IsAirborneNode>(); break;
    case EditorNodeType::ConditionPlayerState: runtimeNode = std::make_shared<IsPlayerStateNode>(eNode.StateNameParameter); break;
    case EditorNodeType::ActionShoot: runtimeNode = std::make_shared<EnemyShootNode>(eNode.Parameter); break;
    case EditorNodeType::ActionLockOn: runtimeNode = std::make_shared<EnemyLockOnNode>(eNode.Parameter >= 1.0f); break;
    case EditorNodeType::ConditionIsLockOn: runtimeNode = std::make_shared<IsEnemyLockOnNode>(); break;
    case EditorNodeType::ActionApproach: runtimeNode = std::make_shared<EnemyApproachNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3); break;
    case EditorNodeType::ActionDash: runtimeNode = std::make_shared<EnemyDashNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3); break;
    case EditorNodeType::ActionStrafe: runtimeNode = std::make_shared<EnemyStrafeNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3); break;
    case EditorNodeType::ActionRetreat: runtimeNode = std::make_shared<EnemyRetreatNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3); break;
    case EditorNodeType::ActionAttack: runtimeNode = std::make_shared<EnemyAttackNode>(); break;
    case EditorNodeType::ActionIdle: runtimeNode = std::make_shared<EnemyIdleNode>(eNode.Parameter); break;
    case EditorNodeType::ActionJump: runtimeNode = std::make_shared<EnemyJumpNode>(eNode.Parameter); break;
    case EditorNodeType::ActionJumpToFly: runtimeNode = std::make_shared<EnemyJumpToFlyNode>(eNode.Parameter); break;
    case EditorNodeType::ActionFlyAscend: runtimeNode = std::make_shared<EnemyFlyAscendNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3); break;
    case EditorNodeType::ActionFlyDescend: runtimeNode = std::make_shared<EnemyFlyDescendNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3); break;
    case EditorNodeType::ActionFlyToGround: runtimeNode = std::make_shared<EnemyFlyToGroundNode>(); break;
    case EditorNodeType::ActionFlyApproach: runtimeNode = std::make_shared<EnemyFlyApproachNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3); break;
    case EditorNodeType::ActionComboStep: runtimeNode = std::make_shared<EnemyComboStepNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 0.5f, eNode.Parameter2); break;
    case EditorNodeType::ActionComboFull: runtimeNode = std::make_shared<EnemyComboFullNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 0.5f, static_cast<int>(eNode.Parameter2), eNode.Parameter3); break;
    case EditorNodeType::ActionBurstShoot: runtimeNode = std::make_shared<EnemyBurstShootNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 0.2f, eNode.Parameter2 > 0.0f ? static_cast<int>(eNode.Parameter2) : 3, eNode.Parameter3 > 0.0f ? eNode.Parameter3 : 0.5f, eNode.Parameter4, eNode.Parameter5 >= 1.0f); break;
    case EditorNodeType::ConditionEnergyLow: runtimeNode = std::make_shared<IsEnergyLowNode>(eNode.Parameter); break;
    case EditorNodeType::ActionEnergyCharge: runtimeNode = std::make_shared<EnemyEnergyChargeNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 1.0f, eNode.Parameter2 > 0.0f ? eNode.Parameter2 : 1.0f); break;
    case EditorNodeType::SelectorRandom: runtimeNode = std::make_shared<RandomSelectorNode>(); break;
    case EditorNodeType::DecoratorWeight: runtimeNode = std::make_shared<RandomSelectorNode>(); break;
    }

    if (!runtimeNode) return nullptr;
    m_nodeInstanceMap[editorNodeId] = runtimeNode;

    bool isLeaf = eNode.IsActionNode() || eNode.IsConditionNode();
    if (!isLeaf) {
        if (eNode.IsWeightNode()) {
            auto weightedChildren = FindWeightedChildrenNodeIds(eNode);
            for (auto &[childId, weight] : weightedChildren) {
                auto childNode = BuildNodeRecursive(childId);
                if (childNode) {
                    auto decorator = std::make_shared<WeightDecoratorNode>(weight);
                    decorator->AddChild(childNode);
                    runtimeNode->AddChild(decorator);
                }
            }
        } else {
            int outputPinId = (int)eNode.OutputPinID.Get();
            for (int childId : FindChildrenNodeIds(outputPinId)) {
                auto childNode = BuildNodeRecursive(childId);
                if (childNode) runtimeNode->AddChild(childNode);
            }
        }
    } else if (eNode.IsConditionNode()) {
        int successPinId = (int)eNode.SuccessPinID.Get();
        int failurePinId = (int)eNode.FailurePinID.Get();
        auto successChildIds = FindChildrenNodeIds(successPinId);
        auto failureChildIds = FindChildrenNodeIds(failurePinId);
        if (!successChildIds.empty() || !failureChildIds.empty()) {
            auto selectorWrapper = std::make_shared<SelectorNode>();
            if (!successChildIds.empty()) {
                auto successSequence = std::make_shared<SequenceNode>();
                std::shared_ptr<BTNode> conditionCopy;
                if (eNode.Type == EditorNodeType::ConditionPlayerClose) conditionCopy = std::make_shared<IsPlayerCloseNode>(eNode.Parameter, eNode.Parameter2);
                else if (eNode.Type == EditorNodeType::ConditionHealthLow) conditionCopy = std::make_shared<IsHealthLowNode>(eNode.Parameter);
                else if (eNode.Type == EditorNodeType::ConditionIsGrounded) conditionCopy = std::make_shared<IsGroundedNode>();
                else if (eNode.Type == EditorNodeType::ConditionIsAirborne) conditionCopy = std::make_shared<IsAirborneNode>();
                else if (eNode.Type == EditorNodeType::ConditionPlayerState) conditionCopy = std::make_shared<IsPlayerStateNode>(eNode.StateNameParameter);
                else if (eNode.Type == EditorNodeType::ConditionIsLockOn) conditionCopy = std::make_shared<IsEnemyLockOnNode>();
                else if (eNode.Type == EditorNodeType::ConditionEnergyLow) conditionCopy = std::make_shared<IsEnergyLowNode>(eNode.Parameter);
                if (conditionCopy) {
                    successSequence->AddChild(conditionCopy);
                    for (int childId : successChildIds) {
                        auto c = BuildNodeRecursive(childId);
                        if (c) successSequence->AddChild(c);
                    }
                    selectorWrapper->AddChild(successSequence);
                }
            }
            for (int childId : failureChildIds) {
                auto c = BuildNodeRecursive(childId);
                if (c) selectorWrapper->AddChild(c);
            }
            return selectorWrapper;
        }
    }
    return runtimeNode;
}

std::vector<int> BehaviorTreeEditor::FindChildrenNodeIds(int outputPinId) {
    std::vector<int> children;
    for (const auto &link : m_Links) {
        if ((int)link.StartPinID.Get() == outputPinId) {
            int endPin = (int)link.EndPinID.Get();
            children.push_back((endPin - kPinOffset - 1) / 10);
        }
    }
    return children;
}

std::vector<std::pair<int, float>> BehaviorTreeEditor::FindWeightedChildrenNodeIds(const EditorNode &node) {
    std::vector<std::pair<int, float>> result;
    for (int i = 0; i < (int)node.WeightedOutputs.size(); ++i) {
        int outputPinId = (int)node.WeightedOutputs[i].PinID.Get();
        float weight = node.WeightedOutputs[i].Weight;
        for (const auto &link : m_Links) {
            if ((int)link.StartPinID.Get() == outputPinId) {
                int childNodeId = ((int)link.EndPinID.Get() - kPinOffset - 1) / 10;
                result.emplace_back(childNodeId, weight);
            }
        }
    }
    return result;
}

int BehaviorTreeEditor::FindRootNodeId() {
    for (const auto &node : m_Nodes) {
        int inputPin = (int)node.InputPinID.Get();
        bool hasInput = false;
        for (const auto &link : m_Links) {
            if ((int)link.EndPinID.Get() == inputPin) {
                hasInput = true;
                break;
            }
        }
        if (!hasInput) return (int)node.ID.Get();
    }
    return -1;
}

void BehaviorTreeEditor::SaveTree() {
    std::string fileName = m_InputFileNameBuf;
    if (fileName.empty()) fileName = "NewBehavior";
    DataHandler handler("BehaviorTree", fileName);
    json nodesJson = json::array();
    for (const auto &node : m_Nodes) {
        json n;
        n["id"] = (int)node.ID.Get();
        n["title"] = node.Title;
        n["type"] = (int)node.Type;
        ImVec2 pos = ed::GetNodePosition(node.ID);
        n["x"] = pos.x; n["y"] = pos.y;
        n["param"] = node.Parameter; n["param2"] = node.Parameter2; n["param3"] = node.Parameter3;
        n["param4"] = node.Parameter4; n["param5"] = node.Parameter5;
        if (node.Type == EditorNodeType::ConditionPlayerState) n["stateName"] = node.StateNameParameter;
        if (node.IsWeightNode()) {
            json weightsJson = json::array();
            for (const auto &output : node.WeightedOutputs) {
                json w; w["pinId"] = (int)output.PinID.Get(); w["weight"] = output.Weight;
                weightsJson.push_back(w);
            }
            n["weightedOutputs"] = weightsJson;
        }
        nodesJson.push_back(n);
    }
    handler.Save("nodes", nodesJson);
    json linksJson = json::array();
    for (const auto &link : m_Links) {
        json l; l["id"] = (int)link.ID.Get(); l["start"] = (int)link.StartPinID.Get(); l["end"] = (int)link.EndPinID.Get();
        linksJson.push_back(l);
    }
    handler.Save("links", linksJson);
}

void BehaviorTreeEditor::LoadTree(const std::string &filePath) {
    std::string folderName, fileName;
    ParsePathToFolderAndFile(filePath, folderName, fileName);
    if (fileName.size() < sizeof(m_InputFileNameBuf)) strcpy_s(m_InputFileNameBuf, fileName.c_str());
    DataHandler handler(folderName, fileName);
    m_Nodes.clear(); m_Links.clear();
    json nodesJson = handler.Load("nodes", json::array());
    int maxNodeId = 0, maxPinId = 0;
    for (const auto &n : nodesJson) {
        int id = n["id"].get<int>();
        std::string title = n["title"].get<std::string>();
        EditorNodeType type = (EditorNodeType)n["type"].get<int>();
        float x = n["x"].get<float>(); float y = n["y"].get<float>();
        EditorNode node(id, title, type);
        node.Parameter = n.value("param", 0.0f); node.Parameter2 = n.value("param2", 0.0f); node.Parameter3 = n.value("param3", 0.0f);
        node.Parameter4 = n.value("param4", 0.0f); node.Parameter5 = n.value("param5", 0.0f);
        if (type == EditorNodeType::ConditionPlayerState && n.contains("stateName")) node.StateNameParameter = n["stateName"].get<std::string>();
        if (node.IsWeightNode() && n.contains("weightedOutputs")) {
            node.WeightedOutputs.clear();
            for (const auto &w : n["weightedOutputs"]) {
                WeightedOutput output; output.PinID = w["pinId"].get<int>(); output.Weight = w["weight"].get<float>();
                node.WeightedOutputs.push_back(output);
                if ((int)output.PinID.Get() > maxPinId) maxPinId = (int)output.PinID.Get();
            }
        }
        m_Nodes.push_back(node);
        ed::SetNodePosition(node.ID, ImVec2(x, y));
        if (id > maxNodeId) maxNodeId = id;
    }
    m_NextNodeId = maxNodeId + 1;
    m_NextPinId = (maxPinId > 0) ? maxPinId + 1 : 200000;
    json linksJson = handler.Load("links", json::array());
    int maxLinkId = 0;
    for (const auto &l : linksJson) {
        int id = l["id"].get<int>(); int start = l["start"].get<int>(); int end = l["end"].get<int>();
        m_Links.emplace_back(id, ed::PinId(start), ed::PinId(end));
        if (id > maxLinkId) maxLinkId = id;
    }
    m_NextLinkId = maxLinkId + 1;
}

void BehaviorTreeEditor::OnImGuiRender() {
    ed::SetCurrentEditor(m_Context);
    ImGui::Text("ファイル名:"); ImGui::SameLine(); ImGui::SetNextItemWidth(150);
    ImGui::InputText("##FileName", m_InputFileNameBuf, IM_ARRAYSIZE(m_InputFileNameBuf));
    ImGui::SameLine(); ImGui::Text(".json");
    ImGui::SameLine(); if (ImGui::Button("保存")) SaveTree();
    ImGui::SameLine(); if (ImGui::Button("読込")) m_ShowLoadWindow = true;
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine(); if (ImGui::Button("ビルド＆実行")) { BuildAndRunTree(); m_IsRunning = true; }
    ImGui::SameLine(); if (ImGui::Button("停止")) { m_IsRunning = false; m_RuntimeRoot = nullptr; }
    if (m_ShowLoadWindow) {
        ImGui::Begin("ビヘイビアツリーを読込", &m_ShowLoadWindow);
        static std::string startPath = "BehaviorTree"; ShowJsonFile(m_SelectedFileName, startPath);
        if (!m_SelectedFileName.empty()) { if (ImGui::Button("選択したファイルを読込")) { LoadTree(m_SelectedFileName); m_ShowLoadWindow = false; } }
        ImGui::End();
    }
    ed::Begin("ビヘイビアツリーエディタ", ImVec2(0, 0));
    for (auto &node : m_Nodes) {
        ed::BeginNode(node.ID);
        ImGui::Text("%s", node.Title.c_str());
        ed::BeginPin(node.InputPinID, ed::PinKind::Input); ImGui::Text("-> 入力"); ed::EndPin();
        if (node.IsConditionNode()) {
            ed::BeginPin(node.SuccessPinID, ed::PinKind::Output); ImGui::TextColored(ImVec4(0, 1, 0, 1), "成功 ->"); ed::EndPin();
            ed::BeginPin(node.FailurePinID, ed::PinKind::Output); ImGui::TextColored(ImVec4(1, 0, 0, 1), "失敗 ->"); ed::EndPin();
        } else if (!node.IsActionNode()) {
            ed::BeginPin(node.OutputPinID, ed::PinKind::Output); ImGui::Text("出力 ->"); ed::EndPin();
        }
        ed::EndNode();
    }
    for (auto &link : m_Links) ed::Link(link.ID, link.StartPinID, link.EndPinID);
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu()) ImGui::OpenPopup("Create New Node");
    if (ImGui::BeginPopup("Create New Node")) {
        auto addBtn = [&](const char *label, EditorNodeType type) { if (ImGui::MenuItem(label)) CreateNode(label, type); };
        addBtn("シーケンス", EditorNodeType::Sequence); addBtn("セレクター", EditorNodeType::Selector);
        addBtn("距離チェック", EditorNodeType::ConditionPlayerClose); addBtn("攻撃", EditorNodeType::ActionAttack);
        ImGui::EndPopup();
    }
    ed::Resume();
    HandleCreateAction(); DeleteSelectedItems();
    ed::End(); ed::SetCurrentEditor(nullptr);
}

void BehaviorTreeEditor::CreateNode(const std::string &title, EditorNodeType type) {
    int id = m_NextNodeId++; EditorNode node(id, title, type);
    m_Nodes.push_back(node); ed::SetNodePosition(node.ID, m_CreatePos);
}

void BehaviorTreeEditor::HandleCreateAction() {
    if (ed::BeginCreate()) {
        ed::PinId start, end;
        if (ed::QueryNewLink(&start, &end)) {
            if (ed::AcceptNewItem()) m_Links.emplace_back(m_NextLinkId++, start, end);
        }
    }
    ed::EndCreate();
}

void BehaviorTreeEditor::DeleteSelectedItems() {
    if (ed::BeginDelete()) {
        ed::NodeId nodeId;
        while (ed::QueryDeletedNode(&nodeId)) {
            if (ed::AcceptDeletedItem()) {
                int id = (int)nodeId.Get();
                m_Nodes.erase(std::remove_if(m_Nodes.begin(), m_Nodes.end(), [id](const EditorNode &n) { return (int)n.ID.Get() == id; }), m_Nodes.end());
            }
        }
        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            if (ed::AcceptDeletedItem()) {
                int id = (int)linkId.Get();
                m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(), [id](const EditorLink &l) { return (int)l.ID.Get() == id; }), m_Links.end());
            }
        }
    }
    ed::EndDelete();
}
#endif

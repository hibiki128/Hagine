#define NOMINMAX
#include "BehaviorTreeEditor.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "Engine/Utility/Debug/ImGui/ImGuiNotification.h"
#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <iostream>

using namespace Hagine;
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
    case EditorNodeType::ActionChargeAttack:
        runtimeNode = std::make_shared<EnemyChargeAttackNode>(
            nd.param > 0.0f ? nd.param : 1.5f,
            nd.param2 > 0.0f ? static_cast<int>(nd.param2) : 8,
            nd.param3 > 0.0f ? nd.param3 : 50.0f);
        break;
    case EditorNodeType::ActionUltimate:
        runtimeNode = std::make_shared<EnemyUltimateNode>(
            nd.param > 0.0f ? nd.param : 70.0f,
            nd.param2 > 0.0f ? static_cast<int>(nd.param2) : 8,
            nd.param3 > 0.0f ? nd.param3 : 0.4f);
        break;
    case EditorNodeType::ConditionPlayerHPLow:
        runtimeNode = std::make_shared<IsPlayerHPLowNode>(nd.param);
        break;
    case EditorNodeType::ConditionEnergyHigh:
        runtimeNode = std::make_shared<IsEnergyHighNode>(nd.param);
        break;
    case EditorNodeType::ActionGuard:
        runtimeNode = std::make_shared<EnemyGuardNode>(nd.param > 0.0f ? nd.param : 0.8f);
        break;
    case EditorNodeType::ConditionPlayerAttacking:
        runtimeNode = std::make_shared<IsPlayerAttackingNode>();
        break;
    case EditorNodeType::ActionBeamUltimate:
        runtimeNode = std::make_shared<EnemyBeamUltimateNode>(nd.param > 0.0f ? nd.param : 1.5f);
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
                   nd.type == EditorNodeType::ActionComboStep ||
                   nd.type == EditorNodeType::ActionComboFull ||
                   nd.type == EditorNodeType::ActionBurstShoot ||
                   nd.type == EditorNodeType::ConditionEnergyLow ||
                   nd.type == EditorNodeType::ActionEnergyCharge ||
                   nd.type == EditorNodeType::ActionChargeAttack ||
                   nd.type == EditorNodeType::ActionUltimate ||
                   nd.type == EditorNodeType::ConditionPlayerHPLow ||
                   nd.type == EditorNodeType::ConditionEnergyHigh ||
                   nd.type == EditorNodeType::ActionGuard ||
                   nd.type == EditorNodeType::ConditionPlayerAttacking ||
                   nd.type == EditorNodeType::ActionBeamUltimate);

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
               nd.type == EditorNodeType::ConditionPlayerState ||
               nd.type == EditorNodeType::ConditionPlayerHPLow ||
               nd.type == EditorNodeType::ConditionEnergyHigh ||
               nd.type == EditorNodeType::ConditionPlayerAttacking) {
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
                case EditorNodeType::ConditionPlayerHPLow:
                    condCopy = std::make_shared<IsPlayerHPLowNode>(nd.param);
                    break;
                case EditorNodeType::ConditionEnergyHigh:
                    condCopy = std::make_shared<IsEnergyHighNode>(nd.param);
                    break;
                case EditorNodeType::ConditionPlayerAttacking:
                    condCopy = std::make_shared<IsPlayerAttackingNode>();
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
    } else if (type == EditorNodeType::ActionChargeAttack) {
        Parameter = 1.5f;  // 溜め時間(秒)
        Parameter2 = 8.0f; // 発射弾数
        Parameter3 = 0.0f; // (未使用)
    } else if (type == EditorNodeType::ActionUltimate) {
        Parameter = 0.0f;   // (未使用)
        Parameter2 = 8.0f;  // 発射弾数
        Parameter3 = 0.35f; // コンボ1段時間(秒)
    } else if (type == EditorNodeType::ConditionPlayerHPLow) {
        Parameter = 0.5f; // HP閾値比率
    } else if (type == EditorNodeType::ConditionEnergyHigh) {
        Parameter = 0.7f; // エネルギー閾値比率
    } else if (type == EditorNodeType::ActionGuard) {
        Parameter = 0.8f; // ガード継続時間(秒)
    } else if (type == EditorNodeType::ActionBeamUltimate) {
        Parameter = 1.5f; // 溜め時間(秒)
    }
}

EditorLink::EditorLink(int id, ed::PinId start, ed::PinId end)
    : ID(id), StartPinID(start), EndPinID(end) {}

BehaviorTreeEditor::BehaviorTreeEditor() {
    ed::Config config;
    config.SettingsFile = nullptr;
    context_ = ed::CreateEditor(&config);
}

BehaviorTreeEditor::~BehaviorTreeEditor() {
    if (context_)
        ed::DestroyEditor(context_);
}

bool BehaviorTreeEditor::IsInputPin(ed::PinId p) { return ((int)p.Get() - kPinOffset) % 10 == 1; }
bool BehaviorTreeEditor::IsOutputPin(ed::PinId p) { return ((int)p.Get() - kPinOffset) % 10 == 2; }
bool BehaviorTreeEditor::IsSuccessPin(ed::PinId p) { return ((int)p.Get() - kPinOffset) % 10 == 3; }
bool BehaviorTreeEditor::IsFailurePin(ed::PinId p) { return ((int)p.Get() - kPinOffset) % 10 == 4; }

bool BehaviorTreeEditor::IsWeightedOutputPin(ed::PinId pinId, int &outNodeId, int &outOutputIndex) {
    for (auto &node : nodes_) {
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
    case EditorNodeType::Sequence:
        return "子ノードを順番に実行し、全て成功で成功を返す";
    case EditorNodeType::SequenceOnce:
        return "Non-Reactive Sequence: Action runs to completion, ignores condition changes";
    case EditorNodeType::Selector:
        return "子ノードを順番に試し、1つでも成功したら成功を返す";
    case EditorNodeType::SelectorRandom:
        return "子ノードの中からランダムに1つを選択して実行する";
    case EditorNodeType::DecoratorWeight:
        return "複数の行動に重み付けしてランダム選択する";
    case EditorNodeType::ActionRun:
        return "一定時間実行する基本アクション";
    case EditorNodeType::ConditionPlayerClose:
        return "プレイヤーが指定距離範囲内にいるかチェック";
    case EditorNodeType::ConditionHealthLow:
        return "HPが指定割合以下かチェック";
    case EditorNodeType::ConditionEnergyLow:
        return "エネルギーが指定割合以下かチェック (param: 閾値比率 0.0〜1.0)";
    case EditorNodeType::ConditionIsGrounded:
        return "敵が地上にいるかチェック";
    case EditorNodeType::ConditionIsAirborne:
        return "敵が空中にいるかチェック";
    case EditorNodeType::ConditionPlayerState:
        return "プレイヤーが指定されたステートかチェック";
    case EditorNodeType::ActionApproach:
        return "ターゲットに向かって通常速度で接近する";
    case EditorNodeType::ActionDash:
        return "ターゲットに向かって高速で接近する";
    case EditorNodeType::ActionStrafe:
        return "ターゲットの周囲を左右に移動する";
    case EditorNodeType::ActionRetreat:
        return "ターゲットから後退する";
    case EditorNodeType::ActionAttack:
        return "攻撃を実行する";
    case EditorNodeType::ActionIdle:
        return "その場で待機する(何もしない)";
    case EditorNodeType::ActionJump:
        return "地上からジャンプする";
    case EditorNodeType::ActionJumpToFly:
        return "地上からジャンプし、飛行状態へ遷移";
    case EditorNodeType::ActionFlyAscend:
        return "飛行中に上昇する";
    case EditorNodeType::ActionFlyDescend:
        return "飛行中に下降する";
    case EditorNodeType::ActionFlyToGround:
        return "飛行状態から地上へ着地する";
    case EditorNodeType::ActionFlyApproach:
        return "飛行中に水平方向へプレイヤーへ接近する";
    case EditorNodeType::ActionShoot:
        return "弾を1発発射し、指定秒数クールダウンする";
    case EditorNodeType::ConditionIsLockOn:
        return "ロックオン中かどうかをチェックする";
    case EditorNodeType::ActionComboStep:
        return "コンボを1段だけ実行する";
    case EditorNodeType::ActionComboFull:
        return "コンボを全段実行する";
    case EditorNodeType::ActionBurstShoot:
        return "弾をN連発する";
    case EditorNodeType::ActionEnergyCharge:
        return "エネルギーをチャージする";
    case EditorNodeType::ActionChargeAttack:
        return "溜め攻撃: param=溜め時間(秒) param2=発射弾数";
    case EditorNodeType::ActionUltimate:
        return "必殺技: フルコンボ+一斉射撃 param2=発射弾数 param3=コンボ1段時間(秒)";
    case EditorNodeType::ConditionPlayerHPLow:
        return "プレイヤーHPが閾値以下かチェック (param: 比率 0.0〜1.0)";
    case EditorNodeType::ConditionEnergyHigh:
        return "自身のエネルギーが閾値以上かチェック (param: 比率 0.0〜1.0)";
    case EditorNodeType::ActionGuard:
        return "一定時間ガードして被ダメージを軽減する (param: 継続時間 秒)";
    case EditorNodeType::ConditionPlayerAttacking:
        return "プレイヤーが攻撃的(Rush/スキル/コンボ中)かチェックする";
    case EditorNodeType::ActionBeamUltimate:
        return "ビーム必殺技: 溜め後にlightningBoltビームを発射 param=溜め時間(秒)";
    default:
        return "説明なし";
    }
}

void BehaviorTreeEditor::BuildAndRunTree() {
    nodeInstanceMap_.clear();
    runtimeRoot_ = nullptr;
    int rootId = FindRootNodeId();
    if (rootId == -1)
        return;
    runtimeRoot_ = BuildNodeRecursive(rootId);
    if (runtimeRoot_) {
        runtimeRoot_->SetContext(debugEnemy_, debugPlayer_);
        if (debugEnemy_)
            debugEnemy_->SetBehaviorTree(runtimeRoot_);
    }
}

std::shared_ptr<BTNode> BehaviorTreeEditor::BuildNodeRecursive(int editorNodeId) {
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
                           [editorNodeId](const EditorNode &n) { return (int)n.ID.Get() == editorNodeId; });
    if (it == nodes_.end())
        return nullptr;
    const EditorNode &eNode = *it;
    std::shared_ptr<BTNode> runtimeNode;

    switch (eNode.Type) {
    case EditorNodeType::Sequence:
        runtimeNode = std::make_shared<SequenceNode>();
        break;
    case EditorNodeType::SequenceOnce:
        runtimeNode = std::make_shared<SequenceOnceNode>();
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
    case EditorNodeType::ConditionIsGrounded:
        runtimeNode = std::make_shared<IsGroundedNode>();
        break;
    case EditorNodeType::ConditionIsAirborne:
        runtimeNode = std::make_shared<IsAirborneNode>();
        break;
    case EditorNodeType::ConditionPlayerState:
        runtimeNode = std::make_shared<IsPlayerStateNode>(eNode.StateNameParameter);
        break;
    case EditorNodeType::ActionShoot:
        runtimeNode = std::make_shared<EnemyShootNode>(eNode.Parameter);
        break;
    case EditorNodeType::ConditionIsLockOn:
        runtimeNode = std::make_shared<IsEnemyLockOnNode>();
        break;
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
    case EditorNodeType::ActionIdle:
        runtimeNode = std::make_shared<EnemyIdleNode>(eNode.Parameter);
        break;
    case EditorNodeType::ActionJump:
        runtimeNode = std::make_shared<EnemyJumpNode>(eNode.Parameter);
        break;
    case EditorNodeType::ActionJumpToFly:
        runtimeNode = std::make_shared<EnemyJumpToFlyNode>(eNode.Parameter);
        break;
    case EditorNodeType::ActionFlyAscend:
        runtimeNode = std::make_shared<EnemyFlyAscendNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3);
        break;
    case EditorNodeType::ActionFlyDescend:
        runtimeNode = std::make_shared<EnemyFlyDescendNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3);
        break;
    case EditorNodeType::ActionFlyToGround:
        runtimeNode = std::make_shared<EnemyFlyToGroundNode>();
        break;
    case EditorNodeType::ActionFlyApproach:
        runtimeNode = std::make_shared<EnemyFlyApproachNode>(eNode.Parameter, eNode.Parameter2, eNode.Parameter3);
        break;
    case EditorNodeType::ActionComboStep:
        runtimeNode = std::make_shared<EnemyComboStepNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 0.5f, eNode.Parameter2);
        break;
    case EditorNodeType::ActionComboFull:
        runtimeNode = std::make_shared<EnemyComboFullNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 0.5f, static_cast<int>(eNode.Parameter2), eNode.Parameter3);
        break;
    case EditorNodeType::ActionBurstShoot:
        runtimeNode = std::make_shared<EnemyBurstShootNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 0.2f, eNode.Parameter2 > 0.0f ? static_cast<int>(eNode.Parameter2) : 3, eNode.Parameter3 > 0.0f ? eNode.Parameter3 : 0.5f, eNode.Parameter4, eNode.Parameter5 >= 1.0f);
        break;
    case EditorNodeType::ConditionEnergyLow:
        runtimeNode = std::make_shared<IsEnergyLowNode>(eNode.Parameter);
        break;
    case EditorNodeType::ActionEnergyCharge:
        runtimeNode = std::make_shared<EnemyEnergyChargeNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 1.0f, eNode.Parameter2 > 0.0f ? eNode.Parameter2 : 1.0f);
        break;
    case EditorNodeType::ActionChargeAttack:
        runtimeNode = std::make_shared<EnemyChargeAttackNode>(
            eNode.Parameter > 0.0f ? eNode.Parameter : 1.5f,
            eNode.Parameter2 > 0.0f ? static_cast<int>(eNode.Parameter2) : 8,
            eNode.Parameter3 > 0.0f ? eNode.Parameter3 : 50.0f);
        break;
    case EditorNodeType::ActionUltimate:
        runtimeNode = std::make_shared<EnemyUltimateNode>(
            eNode.Parameter > 0.0f ? eNode.Parameter : 70.0f,
            eNode.Parameter2 > 0.0f ? static_cast<int>(eNode.Parameter2) : 8,
            eNode.Parameter3 > 0.0f ? eNode.Parameter3 : 0.4f);
        break;
    case EditorNodeType::ConditionPlayerHPLow:
        runtimeNode = std::make_shared<IsPlayerHPLowNode>(eNode.Parameter);
        break;
    case EditorNodeType::ConditionEnergyHigh:
        runtimeNode = std::make_shared<IsEnergyHighNode>(eNode.Parameter);
        break;
    case EditorNodeType::ActionGuard:
        runtimeNode = std::make_shared<EnemyGuardNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 0.8f);
        break;
    case EditorNodeType::ConditionPlayerAttacking:
        runtimeNode = std::make_shared<IsPlayerAttackingNode>();
        break;
    case EditorNodeType::ActionBeamUltimate:
        runtimeNode = std::make_shared<EnemyBeamUltimateNode>(eNode.Parameter > 0.0f ? eNode.Parameter : 1.5f);
        break;
    case EditorNodeType::SelectorRandom:
        runtimeNode = std::make_shared<RandomSelectorNode>();
        break;
    case EditorNodeType::DecoratorWeight:
        runtimeNode = std::make_shared<RandomSelectorNode>();
        break;
    }

    if (!runtimeNode)
        return nullptr;
    nodeInstanceMap_[editorNodeId] = runtimeNode;

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
                if (childNode)
                    runtimeNode->AddChild(childNode);
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
                if (eNode.Type == EditorNodeType::ConditionPlayerClose)
                    conditionCopy = std::make_shared<IsPlayerCloseNode>(eNode.Parameter, eNode.Parameter2);
                else if (eNode.Type == EditorNodeType::ConditionHealthLow)
                    conditionCopy = std::make_shared<IsHealthLowNode>(eNode.Parameter);
                else if (eNode.Type == EditorNodeType::ConditionIsGrounded)
                    conditionCopy = std::make_shared<IsGroundedNode>();
                else if (eNode.Type == EditorNodeType::ConditionIsAirborne)
                    conditionCopy = std::make_shared<IsAirborneNode>();
                else if (eNode.Type == EditorNodeType::ConditionPlayerState)
                    conditionCopy = std::make_shared<IsPlayerStateNode>(eNode.StateNameParameter);
                else if (eNode.Type == EditorNodeType::ConditionIsLockOn)
                    conditionCopy = std::make_shared<IsEnemyLockOnNode>();
                else if (eNode.Type == EditorNodeType::ConditionEnergyLow)
                    conditionCopy = std::make_shared<IsEnergyLowNode>(eNode.Parameter);
                else if (eNode.Type == EditorNodeType::ConditionPlayerHPLow)
                    conditionCopy = std::make_shared<IsPlayerHPLowNode>(eNode.Parameter);
                else if (eNode.Type == EditorNodeType::ConditionEnergyHigh)
                    conditionCopy = std::make_shared<IsEnergyHighNode>(eNode.Parameter);
                else if (eNode.Type == EditorNodeType::ConditionPlayerAttacking)
                    conditionCopy = std::make_shared<IsPlayerAttackingNode>();
                if (conditionCopy) {
                    successSequence->AddChild(conditionCopy);
                    for (int childId : successChildIds) {
                        auto c = BuildNodeRecursive(childId);
                        if (c)
                            successSequence->AddChild(c);
                    }
                    selectorWrapper->AddChild(successSequence);
                }
            }
            for (int childId : failureChildIds) {
                auto c = BuildNodeRecursive(childId);
                if (c)
                    selectorWrapper->AddChild(c);
            }
            return selectorWrapper;
        }
    }
    return runtimeNode;
}

std::vector<int> BehaviorTreeEditor::FindChildrenNodeIds(int outputPinId) {
    std::vector<int> children;
    for (const auto &link : links_) {
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
        for (const auto &link : links_) {
            if ((int)link.StartPinID.Get() == outputPinId) {
                int childNodeId = ((int)link.EndPinID.Get() - kPinOffset - 1) / 10;
                result.emplace_back(childNodeId, weight);
            }
        }
    }
    return result;
}

int BehaviorTreeEditor::FindRootNodeId() {
    for (const auto &node : nodes_) {
        int inputPin = (int)node.InputPinID.Get();
        bool hasInput = false;
        for (const auto &link : links_) {
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
    std::string fileName = inputFileNameBuf_;
    if (fileName.empty())
        fileName = "NewBehavior";
    DataHandler handler("BehaviorTree", fileName);
    json nodesJson = json::array();
    for (const auto &node : nodes_) {
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
        n["param4"] = node.Parameter4;
        n["param5"] = node.Parameter5;
        if (node.Type == EditorNodeType::ConditionPlayerState)
            n["stateName"] = node.StateNameParameter;
        if (node.IsWeightNode()) {
            json weightsJson = json::array();
            for (const auto &output : node.WeightedOutputs) {
                json w;
                w["pinId"] = (int)output.PinID.Get();
                w["weight"] = output.Weight;
                weightsJson.push_back(w);
            }
            n["weightedOutputs"] = weightsJson;
        }
        nodesJson.push_back(n);
    }
    handler.Save("nodes", nodesJson);
    json linksJson = json::array();
    for (const auto &link : links_) {
        json l;
        l["id"] = (int)link.ID.Get();
        l["start"] = (int)link.StartPinID.Get();
        l["end"] = (int)link.EndPinID.Get();
        linksJson.push_back(l);
    }
    handler.Save("links", linksJson);
    ImGuiNotification::Post("ビヘイビアツリーを保存しました: " + fileName);
}

void BehaviorTreeEditor::LoadTree(const std::string &filePath) {
    std::string folderName, fileName;
    ParsePathToFolderAndFile(filePath, folderName, fileName);
    if (fileName.size() < sizeof(inputFileNameBuf_))
        strcpy_s(inputFileNameBuf_, fileName.c_str());
    DataHandler handler(folderName, fileName);
    nodes_.clear();
    links_.clear();
    json nodesJson = handler.Load("nodes", json::array());
    int maxNodeId = 0, maxPinId = 0;
    for (const auto &n : nodesJson) {
        int id = n["id"].get<int>();
        std::string title = n["title"].get<std::string>();
        EditorNodeType type = (EditorNodeType)n["type"].get<int>();
        float x = n["x"].get<float>();
        float y = n["y"].get<float>();
        EditorNode node(id, title, type);
        node.Parameter = n.value("param", 0.0f);
        node.Parameter2 = n.value("param2", 0.0f);
        node.Parameter3 = n.value("param3", 0.0f);
        node.Parameter4 = n.value("param4", 0.0f);
        node.Parameter5 = n.value("param5", 0.0f);
        if (type == EditorNodeType::ConditionPlayerState && n.contains("stateName"))
            node.StateNameParameter = n["stateName"].get<std::string>();
        if (node.IsWeightNode() && n.contains("weightedOutputs")) {
            node.WeightedOutputs.clear();
            for (const auto &w : n["weightedOutputs"]) {
                WeightedOutput output;
                output.PinID = w["pinId"].get<int>();
                output.Weight = w["weight"].get<float>();
                node.WeightedOutputs.push_back(output);
                if ((int)output.PinID.Get() > maxPinId)
                    maxPinId = (int)output.PinID.Get();
            }
        }
        nodes_.push_back(node);
        ed::SetNodePosition(node.ID, ImVec2(x, y));
        if (id > maxNodeId)
            maxNodeId = id;
    }
    nextNodeId_ = maxNodeId + 1;
    nextPinId_ = (maxPinId > 0) ? maxPinId + 1 : 200000;
    json linksJson = handler.Load("links", json::array());
    int maxLinkId = 0;
    for (const auto &l : linksJson) {
        int id = l["id"].get<int>();
        int start = l["start"].get<int>();
        int end = l["end"].get<int>();
        links_.emplace_back(id, ed::PinId(start), ed::PinId(end));
        if (id > maxLinkId)
            maxLinkId = id;
    }
    nextLinkId_ = maxLinkId + 1;
    ImGuiNotification::Post("ビヘイビアツリーを読み込みました: " + fileName);
}

void BehaviorTreeEditor::OnImGuiRender() {
    const float dt = ImGui::GetIO().DeltaTime;
    const float now = static_cast<float>(ImGui::GetTime());

    // ────────────────────────────────────────────────────────────
    // ステータスタイマー更新
    //   正値 = 成功グロー残り時間 (秒)
    //   負値 = 失敗グロー残り時間 (秒, 絶対値)
    // ────────────────────────────────────────────────────────────
    for (auto &[id, timer] : statusTimers_) {
        if (timer > 0.0f)
            timer = std::max(0.0f, timer - dt * 1.5f);
        else if (timer < 0.0f)
            timer = std::min(0.0f, timer + dt * 1.5f);
    }
    if (isRunning_) {
        for (const auto &node : nodes_) {
            const int nid = static_cast<int>(node.ID.Get());
            auto it = nodeInstanceMap_.find(nid);
            if (it != nodeInstanceMap_.end() && it->second) {
                const NodeStatus s = it->second->GetStatus();
                if (s == NodeStatus::Success)
                    statusTimers_[nid] = 1.2f;
                else if (s == NodeStatus::Failure)
                    statusTimers_[nid] = -1.2f;
            }
        }
    }

    // ────────────────────────────────────────────────────────────
    // ツールバー
    // ────────────────────────────────────────────────────────────
    ed::SetCurrentEditor(context_);

    ImGui::Text("ファイル名:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("##FileName", inputFileNameBuf_, IM_ARRAYSIZE(inputFileNameBuf_));
    ImGui::SameLine();
    ImGui::Text(".json");
    ImGui::SameLine();
    if (ImGui::Button("  保存  "))
        SaveTree();
    ImGui::SameLine();
    if (ImGui::Button("  読込  "))
        showLoadWindow_ = true;
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    if (isRunning_) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.55f, 0.22f, 1.0f));
        if (ImGui::Button(" [実行中...] ")) { /* nothing */
        }
        ImGui::PopStyleColor(2);
    } else {
        if (ImGui::Button(" ▶ ビルド＆実行 ")) {
            BuildAndRunTree();
            isRunning_ = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(" ■ 停止 ")) {
        isRunning_ = false;
        runtimeRoot_ = nullptr;
        isSingleTesting_ = false;
        singleTestNode_ = nullptr;
        singleTestNodeId_ = -1;
    }

    // ── 単体ノードテスト ──────────────────────────────
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    {
        // 選択中ノードIDを取得
        std::vector<ed::NodeId> selectedNodes;
        selectedNodes.resize(ed::GetSelectedObjectCount());
        int selCount = ed::GetSelectedNodes(selectedNodes.data(), (int)selectedNodes.size());
        int selNodeId = (selCount > 0) ? (int)selectedNodes[0].Get() : -1;

        // 選択ノード名を表示
        const char *selName = "---";
        for (const auto &n : nodes_) {
            if ((int)n.ID.Get() == selNodeId) {
                selName = n.Title.c_str();
                break;
            }
        }
        ImGui::TextDisabled("選択: %s", selName);
        ImGui::SameLine();

        if (isSingleTesting_) {
            // テスト実行中 → 停止ボタン
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.22f, 0.22f, 1.0f));
            if (ImGui::Button(" ■ 単体テスト停止 ")) {
                isSingleTesting_ = false;
                singleTestNode_ = nullptr;
                singleTestNodeId_ = -1;
            }
            ImGui::PopStyleColor(2);
        } else {
            bool canTest = (selNodeId != -1) && (debugEnemy_ != nullptr);
            if (!canTest)
                ImGui::BeginDisabled();
            if (ImGui::Button(" ▷ 単体テスト ")) {
                // 選択ノードだけをビルドして単体実行
                isSingleTesting_ = false;
                singleTestNode_ = nullptr;
                singleTestNodeId_ = selNodeId;
                auto singleNode = BuildNodeRecursive(selNodeId);
                if (singleNode) {
                    singleNode->SetContext(debugEnemy_, debugPlayer_);
                    singleNode->Reset();
                    singleTestNode_ = singleNode;
                    isSingleTesting_ = true;
                    // 全体ツリー実行は停止
                    isRunning_ = false;
                    runtimeRoot_ = nullptr;
                }
            }
            if (!canTest)
                ImGui::EndDisabled();
        }

        // 単体テスト実行中は毎フレームTickしてステータスを表示
        if (isSingleTesting_ && singleTestNode_) {
            NodeStatus s = singleTestNode_->Tick();
            ImGui::SameLine();
            if (s == NodeStatus::Running)
                ImGui::TextColored(ImVec4(1.0f, 0.88f, 0.15f, 1.0f), "[実行中]");
            else if (s == NodeStatus::Success) {
                ImGui::TextColored(ImVec4(0.25f, 1.0f, 0.35f, 1.0f), "[成功]");
                // 成功/失敗で自動停止
                isSingleTesting_ = false;
            } else if (s == NodeStatus::Failure) {
                ImGui::TextColored(ImVec4(1.0f, 0.28f, 0.28f, 1.0f), "[失敗]");
                isSingleTesting_ = false;
            }

            // 単体テスト中もステータスタイマーを更新してノードの色に反映
            if (singleTestNodeId_ != -1) {
                if (s == NodeStatus::Success)
                    statusTimers_[singleTestNodeId_] = 1.2f;
                else if (s == NodeStatus::Failure)
                    statusTimers_[singleTestNodeId_] = -1.2f;
            }
        }
    }

    // 実行中ノード名をインラインに表示
    if (isRunning_) {
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        std::string runningName = "---";
        for (const auto &node : nodes_) {
            auto it = nodeInstanceMap_.find(static_cast<int>(node.ID.Get()));
            if (it != nodeInstanceMap_.end() && it->second &&
                it->second->GetStatus() == NodeStatus::Running) {
                runningName = node.Title;
                break;
            }
        }
        ImGui::TextColored(ImVec4(1.0f, 0.88f, 0.15f, 1.0f), "実行中: %s", runningName.c_str());
    }

    // 凡例
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.88f, 0.15f, 1.0f), "● 実行");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.25f, 1.0f, 0.35f, 1.0f), "● 成功");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.28f, 0.28f, 1.0f), "● 失敗");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.0f, 1.0f), "■ Seq");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.70f, 0.25f, 1.0f), "■ Sel");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.80f, 0.40f, 1.0f, 1.0f), "■ 条件");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.0f), "■ Action");

    // ロードウィンドウ
    if (showLoadWindow_) {
        ImGui::Begin("ビヘイビアツリーを読込", &showLoadWindow_);
        static std::string startPath = "BehaviorTree";
        ShowJsonFile(selectedFileName_, startPath);
        if (!selectedFileName_.empty()) {
            if (ImGui::Button("選択したファイルを読込")) {
                LoadTree(selectedFileName_);
                showLoadWindow_ = false;
            }
        }
        ImGui::End();
    }

    // ────────────────────────────────────────────────────────────
    // エディタ本体
    // ────────────────────────────────────────────────────────────
    ed::Begin("ビヘイビアツリーエディタ", ImVec2(0, 0));

    const float pulse = sinf(now * 5.0f) * 0.5f + 0.5f; // 0→1 sin波

    for (auto &node : nodes_) {
        const int nid = static_cast<int>(node.ID.Get());

        // ── ランタイムステータス取得 ──────────────────────────
        NodeStatus status = NodeStatus::Idle;
        if (isRunning_) {
            auto it = nodeInstanceMap_.find(nid);
            if (it != nodeInstanceMap_.end() && it->second)
                status = it->second->GetStatus();
        }
        const float timer = statusTimers_.count(nid) ? statusTimers_.at(nid) : 0.0f;

        // ── ノードカラー計算 ──────────────────────────────────
        ImVec4 bgColor, borderColor;
        float borderWidth = 1.5f;

        if (status == NodeStatus::Running) {
            // 金色パルス
            bgColor = ImVec4(0.30f + pulse * 0.14f, 0.24f + pulse * 0.06f, 0.0f, 1.0f);
            borderColor = ImVec4(1.0f, 0.68f + pulse * 0.32f, 0.0f, 1.0f);
            borderWidth = 3.0f + pulse * 2.0f;
        } else if (status == NodeStatus::Success || timer > 0.0f) {
            const float i = (status == NodeStatus::Success) ? 1.0f : timer / 1.2f;
            bgColor = ImVec4(0.07f + i * 0.13f, 0.20f + i * 0.18f, 0.07f, 1.0f);
            borderColor = ImVec4(0.15f, 0.88f, 0.25f, 0.30f + i * 0.70f);
            borderWidth = 2.5f;
        } else if (status == NodeStatus::Failure || timer < 0.0f) {
            const float i = (status == NodeStatus::Failure) ? 1.0f : (-timer) / 1.2f;
            bgColor = ImVec4(0.22f + i * 0.12f, 0.07f, 0.07f, 1.0f);
            borderColor = ImVec4(0.88f, 0.15f, 0.15f, 0.30f + i * 0.70f);
            borderWidth = 2.5f;
        } else {
            // カテゴリ別ベースカラー
            switch (node.Type) {
            case EditorNodeType::Sequence:
            case EditorNodeType::SequenceOnce:
                bgColor = ImVec4(0.09f, 0.14f, 0.28f, 1.0f);
                borderColor = ImVec4(0.35f, 0.55f, 0.92f, 0.70f);
                break;
            case EditorNodeType::Selector:
            case EditorNodeType::SelectorRandom:
                bgColor = ImVec4(0.26f, 0.16f, 0.05f, 1.0f);
                borderColor = ImVec4(0.92f, 0.62f, 0.15f, 0.70f);
                break;
            case EditorNodeType::DecoratorWeight:
                bgColor = ImVec4(0.13f, 0.20f, 0.23f, 1.0f);
                borderColor = ImVec4(0.42f, 0.72f, 0.82f, 0.70f);
                break;
            default:
                if (node.IsConditionNode()) {
                    bgColor = ImVec4(0.18f, 0.10f, 0.26f, 1.0f);
                    borderColor = ImVec4(0.75f, 0.35f, 0.95f, 0.70f);
                } else if (node.IsActionNode()) {
                    bgColor = ImVec4(0.09f, 0.20f, 0.12f, 1.0f);
                    borderColor = ImVec4(0.25f, 0.72f, 0.35f, 0.70f);
                } else {
                    bgColor = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
                    borderColor = ImVec4(0.40f, 0.40f, 0.40f, 0.70f);
                }
                break;
            }
            borderWidth = 1.5f;
        }

        ed::PushStyleColor(ed::StyleColor_NodeBg, bgColor);
        ed::PushStyleColor(ed::StyleColor_NodeBorder, borderColor);
        ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, borderWidth);
        ed::PushStyleVar(ed::StyleVar_NodeRounding, 6.0f);

        ed::BeginNode(node.ID);

        // ── タイトル行 ──────────────────────────────────────
        ImVec4 titleColor;
        const char *statusBadge = "";

        if (status == NodeStatus::Running) {
            titleColor = ImVec4(1.0f, 0.92f, 0.20f, 1.0f);
            statusBadge = " [>>]";
        } else if (status == NodeStatus::Success || timer > 0.0f) {
            titleColor = ImVec4(0.28f, 1.0f, 0.40f, 1.0f);
            statusBadge = " [OK]";
        } else if (status == NodeStatus::Failure || timer < 0.0f) {
            titleColor = ImVec4(1.0f, 0.30f, 0.30f, 1.0f);
            statusBadge = " [NG]";
        } else {
            // カテゴリ別タイトル色
            switch (node.Type) {
            case EditorNodeType::Sequence:
            case EditorNodeType::SequenceOnce:
                titleColor = ImVec4(0.60f, 0.82f, 1.0f, 1.0f);
                break;
            case EditorNodeType::Selector:
            case EditorNodeType::SelectorRandom:
                titleColor = ImVec4(1.0f, 0.78f, 0.38f, 1.0f);
                break;
            case EditorNodeType::DecoratorWeight:
                titleColor = ImVec4(0.50f, 0.88f, 1.0f, 1.0f);
                break;
            default:
                if (node.IsConditionNode())
                    titleColor = ImVec4(0.88f, 0.55f, 1.0f, 1.0f);
                else if (node.IsActionNode())
                    titleColor = ImVec4(0.42f, 0.92f, 0.52f, 1.0f);
                else
                    titleColor = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
                break;
            }
        }

        ImGui::TextColored(titleColor, "%s%s", node.Title.c_str(), statusBadge);

        // ツールチップ（説明＋パラメータ）
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextColored(titleColor, "%s", node.Title.c_str());
            ImGui::Separator();
            ImGui::TextUnformatted(GetNodeDescription(node.Type));
            bool hasParam = (node.Parameter != 0.0f || node.Parameter2 != 0.0f ||
                             node.Parameter3 != 0.0f);
            if (hasParam || node.Type == EditorNodeType::ConditionPlayerState) {
                ImGui::Separator();
                if (node.Type == EditorNodeType::ConditionPlayerState) {
                    ImGui::Text("ステート名: %s", node.StateNameParameter.c_str());
                } else {
                    if (node.Parameter != 0.0f)
                        ImGui::Text("Param1: %.3f", node.Parameter);
                    if (node.Parameter2 != 0.0f)
                        ImGui::Text("Param2: %.3f", node.Parameter2);
                    if (node.Parameter3 != 0.0f)
                        ImGui::Text("Param3: %.3f", node.Parameter3);
                    if (node.Parameter4 != 0.0f)
                        ImGui::Text("Param4: %.3f", node.Parameter4);
                    if (node.Parameter5 != 0.0f)
                        ImGui::Text("Param5: %.3f", node.Parameter5);
                }
            }
            // ステータス
            ImGui::Separator();
            const char *statStr =
                (status == NodeStatus::Running) ? "Running" : (status == NodeStatus::Success) ? "Success"
                                                          : (status == NodeStatus::Failure)   ? "Failure"
                                                                                              : "Idle";
            ImGui::Text("Status: %s", statStr);
            ImGui::EndTooltip();
        }

        // コンパクトなパラメータ表示（ノード内）
        if (node.Type == EditorNodeType::ConditionPlayerClose) {
            ImGui::TextDisabled("dist: %.1f~%.1f", node.Parameter, node.Parameter2);
        } else if (node.Type == EditorNodeType::ConditionHealthLow) {
            ImGui::TextDisabled("HP <= %.0f%%", node.Parameter * 100.0f);
        } else if (node.Type == EditorNodeType::ConditionEnergyLow) {
            ImGui::TextDisabled("EP <= %.0f%%", node.Parameter * 100.0f);
        } else if (node.Type == EditorNodeType::ActionIdle) {
            ImGui::TextDisabled("%.1fs", node.Parameter);
        } else if (node.Type == EditorNodeType::ActionShoot) {
            ImGui::TextDisabled("CD: %.1fs", node.Parameter);
        } else if (node.Type == EditorNodeType::ActionBurstShoot) {
            ImGui::TextDisabled("x%.0f  %.2fs", node.Parameter2, node.Parameter);
        } else if (node.Type == EditorNodeType::ConditionPlayerState) {
            // ステート選択ドロップダウン（ノード内インライン編集）
            static const char *kPlayerStates[] = {
                "Idle", "Move", "Jump", "Air",
                "FlyIdle", "FlyMove", "Rush", "Guard",
                "EnergyCharge", "Attack", "Dead"};
            static constexpr int kPlayerStateCount = 11;
            // 現在のStateNameParameterに一致するインデックスを探す
            int currentIdx = 0;
            for (int si = 0; si < kPlayerStateCount; ++si) {
                if (node.StateNameParameter == kPlayerStates[si]) {
                    currentIdx = si;
                    break;
                }
            }
            // ノードIDをユニークIDに使ってウィジェットを識別する
            ImGui::SetNextItemWidth(110.0f);
            std::string comboId = "##state_" + std::to_string((int)node.ID.Get());
            if (ImGui::BeginCombo(comboId.c_str(), kPlayerStates[currentIdx])) {
                for (int si = 0; si < kPlayerStateCount; ++si) {
                    bool selected = (si == currentIdx);
                    if (ImGui::Selectable(kPlayerStates[si], selected))
                        node.StateNameParameter = kPlayerStates[si];
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        } else if (node.IsActionNode() && node.Parameter != 0.0f) {
            ImGui::TextDisabled("%.1f / %.1f", node.Parameter, node.Parameter2);
        }

        // ── 入力ピン ────────────────────────────────────────
        ed::BeginPin(node.InputPinID, ed::PinKind::Input);
        ImGui::TextColored(ImVec4(0.65f, 0.85f, 1.0f, 0.85f), "-> IN");
        ed::EndPin();

        // ── 出力ピン ────────────────────────────────────────
        if (node.IsConditionNode()) {
            ed::BeginPin(node.SuccessPinID, ed::PinKind::Output);
            ImGui::TextColored(ImVec4(0.28f, 1.0f, 0.40f, 1.0f), "OK ->");
            ed::EndPin();
            ed::BeginPin(node.FailurePinID, ed::PinKind::Output);
            ImGui::TextColored(ImVec4(1.0f, 0.30f, 0.30f, 1.0f), "NG ->");
            ed::EndPin();
        } else if (node.IsWeightNode()) {
            for (auto &wo : node.WeightedOutputs) {
                ed::BeginPin(wo.PinID, ed::PinKind::Output);
                ImGui::Text("%.0f%% ->", wo.Weight);
                ed::EndPin();
            }
        } else if (!node.IsActionNode()) {
            ed::BeginPin(node.OutputPinID, ed::PinKind::Output);
            ImGui::TextColored(ImVec4(0.90f, 0.90f, 0.90f, 0.85f), "OUT ->");
            ed::EndPin();
        }

        ed::EndNode();

        ed::PopStyleVar(2);
        ed::PopStyleColor(2);
    }

    // ────────────────────────────────────────────────────────────
    // リンク描画（ステータスカラー＋フローアニメーション）
    // ────────────────────────────────────────────────────────────
    // フローマーカーの色を設定（Running中の流れる粒子）
    ed::PushStyleColor(ed::StyleColor_Flow, ImVec4(1.0f, 0.90f, 0.10f, 1.0f));
    ed::PushStyleColor(ed::StyleColor_FlowMarker, ImVec4(1.0f, 1.00f, 0.40f, 1.0f));

    for (auto &link : links_) {
        ImVec4 linkColor(0.50f, 0.50f, 0.50f, 0.80f);
        float thickness = 1.5f;
        bool doFlow = false;

        if (isRunning_) {
            // リンク起点ノードを検索
            const EditorNode *srcNode = nullptr;
            for (const auto &n : nodes_) {
                if (n.OutputPinID == link.StartPinID ||
                    n.SuccessPinID == link.StartPinID ||
                    n.FailurePinID == link.StartPinID) {
                    srcNode = &n;
                    break;
                }
                if (!srcNode) {
                    for (const auto &wo : n.WeightedOutputs) {
                        if (wo.PinID == link.StartPinID) {
                            srcNode = &n;
                            break;
                        }
                    }
                }
            }
            if (srcNode) {
                const int srcId = static_cast<int>(srcNode->ID.Get());
                auto it = nodeInstanceMap_.find(srcId);
                if (it != nodeInstanceMap_.end() && it->second) {
                    const NodeStatus s = it->second->GetStatus();
                    const float t = statusTimers_.count(srcId) ? statusTimers_.at(srcId) : 0.0f;
                    if (s == NodeStatus::Running) {
                        linkColor = ImVec4(1.0f, 0.78f + pulse * 0.22f, 0.0f, 1.0f);
                        thickness = 3.0f;
                        doFlow = true;
                    } else if (s == NodeStatus::Success || t > 0.0f) {
                        const float i = (s == NodeStatus::Success) ? 1.0f : t / 1.2f;
                        linkColor = ImVec4(0.12f, 0.70f + i * 0.20f, 0.18f, 0.45f + i * 0.55f);
                        thickness = 2.0f;
                    } else if (s == NodeStatus::Failure || t < 0.0f) {
                        const float i = (s == NodeStatus::Failure) ? 1.0f : (-t) / 1.2f;
                        linkColor = ImVec4(0.70f + i * 0.20f, 0.12f, 0.12f, 0.45f + i * 0.55f);
                        thickness = 2.0f;
                    }
                }
            }
        }

        ed::Link(link.ID, link.StartPinID, link.EndPinID, linkColor, thickness);
        if (doFlow)
            ed::Flow(link.ID);
    }

    ed::PopStyleColor(2); // Flow / FlowMarker

    // ────────────────────────────────────────────────────────────
    // バックグラウンドコンテキストメニュー（カテゴリ別）
    // ────────────────────────────────────────────────────────────
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu())
        ImGui::OpenPopup("BTNodeCreateMenu");
    if (ImGui::BeginPopup("BTNodeCreateMenu")) {
        auto addBtn = [&](const char *label, EditorNodeType type) {
            if (ImGui::MenuItem(label))
                CreateNode(label, type);
        };

        ImGui::TextDisabled("── ノードを追加 ──");
        ImGui::Separator();

        if (ImGui::BeginMenu("コンポジット")) {
            addBtn("シーケンス", EditorNodeType::Sequence);
            addBtn("シーケンス(完走)", EditorNodeType::SequenceOnce);
            addBtn("セレクター", EditorNodeType::Selector);
            addBtn("ランダムセレクター", EditorNodeType::SelectorRandom);
            addBtn("重み付けデコレータ", EditorNodeType::DecoratorWeight);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("条件ノード")) {
            addBtn("距離チェック", EditorNodeType::ConditionPlayerClose);
            addBtn("HP低下チェック", EditorNodeType::ConditionHealthLow);
            addBtn("エネルギー低下チェック", EditorNodeType::ConditionEnergyLow);
            addBtn("エネルギー高チェック", EditorNodeType::ConditionEnergyHigh);
            addBtn("地上チェック", EditorNodeType::ConditionIsGrounded);
            addBtn("空中チェック", EditorNodeType::ConditionIsAirborne);
            addBtn("プレイヤーステート", EditorNodeType::ConditionPlayerState);
            addBtn("プレイヤーHP低下チェック", EditorNodeType::ConditionPlayerHPLow);
            addBtn("ロックオン中チェック", EditorNodeType::ConditionIsLockOn);
            addBtn("プレイヤー攻撃中チェック", EditorNodeType::ConditionPlayerAttacking);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("移動アクション")) {
            addBtn("走る", EditorNodeType::ActionRun);
            addBtn("接近", EditorNodeType::ActionApproach);
            addBtn("ダッシュ", EditorNodeType::ActionDash);
            addBtn("ストレイフ", EditorNodeType::ActionStrafe);
            addBtn("後退", EditorNodeType::ActionRetreat);
            addBtn("待機", EditorNodeType::ActionIdle);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("ジャンプ/飛行")) {
            addBtn("ジャンプ", EditorNodeType::ActionJump);
            addBtn("飛行ジャンプ", EditorNodeType::ActionJumpToFly);
            addBtn("上昇", EditorNodeType::ActionFlyAscend);
            addBtn("下降", EditorNodeType::ActionFlyDescend);
            addBtn("着地", EditorNodeType::ActionFlyToGround);
            addBtn("飛行接近", EditorNodeType::ActionFlyApproach);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("戦闘アクション")) {
            addBtn("攻撃", EditorNodeType::ActionAttack);
            addBtn("射撃", EditorNodeType::ActionShoot);
            addBtn("連射", EditorNodeType::ActionBurstShoot);
            addBtn("コンボ(1段)", EditorNodeType::ActionComboStep);
            addBtn("コンボ(全段)", EditorNodeType::ActionComboFull);
            addBtn("溜め攻撃", EditorNodeType::ActionChargeAttack);
            addBtn("重スキル(コンボ+連射)", EditorNodeType::ActionUltimate);
            addBtn("ビーム必殺技", EditorNodeType::ActionBeamUltimate);
            addBtn("ガード", EditorNodeType::ActionGuard);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("その他")) {
            addBtn("エネルギーチャージ", EditorNodeType::ActionEnergyCharge);
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }
    ed::Resume();

    HandleCreateAction();
    DeleteSelectedItems();
    ed::End();
    ed::SetCurrentEditor(nullptr);
}

void BehaviorTreeEditor::CreateNode(const std::string &title, EditorNodeType type) {
    int id = nextNodeId_++;
    EditorNode node(id, title, type);
    nodes_.push_back(node);
    ed::SetNodePosition(node.ID, createPos_);
    ImGuiNotification::Post("ノードを作成しました: " + title);
}

void BehaviorTreeEditor::HandleCreateAction() {
    if (ed::BeginCreate()) {
        ed::PinId start, end;
        if (ed::QueryNewLink(&start, &end)) {
            if (ed::AcceptNewItem()) {
                links_.emplace_back(nextLinkId_++, start, end);
                ImGuiNotification::Post("リンクを作成しました");
            }
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
                auto it = std::find_if(nodes_.begin(), nodes_.end(), [id](const EditorNode &n) { return (int)n.ID.Get() == id; });
                if (it != nodes_.end()) {
                    std::string title = it->Title;
                    nodes_.erase(it);
                    ImGuiNotification::Post("ノードを削除しました: " + title);
                }
            }
        }
        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            if (ed::AcceptDeletedItem()) {
                int id = (int)linkId.Get();
                links_.erase(std::remove_if(links_.begin(), links_.end(), [id](const EditorLink &l) { return (int)l.ID.Get() == id; }), links_.end());
                ImGuiNotification::Post("リンクを削除しました");
            }
        }
    }
    ed::EndDelete();
}
#endif
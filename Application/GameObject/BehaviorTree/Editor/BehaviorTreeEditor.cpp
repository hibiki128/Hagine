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
// BehaviorTreeLoader  (Debug / Release 共通実装)
// ============================================================

namespace {
// EditorNodeType を int にキャストしたときのピンID計算ルール:
//   InputPin  = id*10 + 1
//   OutputPin = id*10 + 2
// これは EditorNode コンストラクタと同じ計算式
inline bool IsInputPinStatic(int pinId) { return (pinId % 10) == 1; }
} // anonymous namespace

// ----- 内部ヘルパー -----

int BehaviorTreeLoader::FindRootNodeId(
    const std::vector<NodeData> &nodes,
    const std::vector<LinkData> &links) {
    for (const auto &node : nodes) {
        int inputPin = node.id * 10 + 1;
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
    for (const auto &link : links) {
        if (link.startPin == outputPinId) {
            // InputPin = id*10+1  →  id = (endPin - 1) / 10
            children.push_back((link.endPin - 1) / 10);
        }
    }
    return children;
}

std::vector<std::pair<int, float>> BehaviorTreeLoader::FindWeightedChildrenNodeIds(
    const NodeData &node,
    const std::vector<LinkData> &links) {
    std::vector<std::pair<int, float>> result;
    for (const auto &wp : node.weightedOutputs) {
        for (const auto &link : links) {
            if (link.startPin == wp.pinId) {
                int childId = (link.endPin - 1) / 10;
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
        // param: 1段のモーション待機時間(秒), param2: コンボ間隔オーバーライド(秒)
        runtimeNode = std::make_shared<EnemyComboStepNode>(
            nd.param > 0.0f ? nd.param : 0.5f,
            nd.param2);
        break;
    case EditorNodeType::ActionComboFull:
        // param: 1段あたりの時間(秒), param2: 最大段数(0=全段), param3: コンボ間隔オーバーライド(秒)
        runtimeNode = std::make_shared<EnemyComboFullNode>(
            nd.param > 0.0f ? nd.param : 0.5f,
            static_cast<int>(nd.param2),
            nd.param3);
        break;
    case EditorNodeType::ActionBurstShoot:
        // param: 発射間隔(秒), param2: 弾数, param3: クールダウン(秒)
        // param4: 拡散角度(度, 0=直進), param5: ホーミング(0=拡散弾, 1=追従弾)
        runtimeNode = std::make_shared<EnemyBurstShootNode>(
            nd.param > 0.0f ? nd.param : 0.2f,
            nd.param2 > 0.0f ? static_cast<int>(nd.param2) : 3,
            nd.param3 > 0.0f ? nd.param3 : 0.5f,
            nd.param4,
            nd.param5 >= 1.0f);
        break;
    case EditorNodeType::ActionEnergyCharge:
        // param: チャージ速度倍率(1.0=通常), param2: 目標エネルギー比率(1.0=最大まで)
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

    // --- 葉ノード判定 ---
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
            // 重み付きノード: 各重み出力ピンから子を接続
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
            int outputPin = nd.id * 10 + 2;
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
        // 条件ノード: 成功/失敗ピン (id*10+3 / id*10+4) からの子を処理
        int successPin = nd.id * 10 + 3;
        int failurePin = nd.id * 10 + 4;
        auto successChildIds = FindChildrenNodeIds(successPin, links);
        auto failureChildIds = FindChildrenNodeIds(failurePin, links);

        if (!successChildIds.empty() || !failureChildIds.empty()) {
            auto selectorWrapper = std::make_shared<SelectorNode>();

            if (!successChildIds.empty()) {
                auto successSeq = std::make_shared<SequenceNode>();
                // 条件ノード本体のコピー
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

// ----- 公開API -----
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

// ============================================================
// BehaviorTreeEditor  (Debugビルド専用)
// ============================================================
#ifdef _DEBUG
#include "Input.h"
#include <ShowFolder/ShowFolder.h>
#include <imgui_internal.h>

namespace ed = ax::NodeEditor;

// ---------------------------------------------------------
// EditorNode
// ---------------------------------------------------------
EditorNode::EditorNode(int id, const std::string &title, EditorNodeType type)
    : ID(id), Title(title), Type(type) {
    InputPinID = id * 10 + 1;
    OutputPinID = id * 10 + 2;
    SuccessPinID = id * 10 + 3;
    FailurePinID = id * 10 + 4;

    if (type == EditorNodeType::ConditionPlayerClose) {
        Parameter = 0.0f;
        Parameter2 = 10.0f;
    } else if (type == EditorNodeType::DecoratorWeight) {
        WeightedOutputs.resize(2);
        WeightedOutputs[0].PinID = id * 10 + 5;
        WeightedOutputs[0].Weight = 1.0f;
        WeightedOutputs[1].PinID = id * 10 + 6;
        WeightedOutputs[1].Weight = 1.0f;
    } else if (type == EditorNodeType::ConditionHealthLow) {
        Parameter = 0.3f;
    } else if (type == EditorNodeType::ConditionEnergyLow) {
        Parameter = 0.3f;
    } else if (type == EditorNodeType::ActionEnergyCharge) {
        Parameter = 1.0f;  // チャージ速度倍率
        Parameter2 = 1.0f; // 目標エネルギー比率
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
        Parameter = 15.0f; // ジャンプ力
    } else if (type == EditorNodeType::ActionFlyAscend) {
        Parameter = 1.0f;   // 最小時間
        Parameter2 = 3.0f;  // 最大時間
        Parameter3 = 15.0f; // 上昇速度
    } else if (type == EditorNodeType::ActionFlyDescend) {
        Parameter = 1.0f;   // 最小時間
        Parameter2 = 3.0f;  // 最大時間
        Parameter3 = 15.0f; // 下降速度
    } else if (type == EditorNodeType::ActionFlyApproach) {
        Parameter = 1.0f;   // 最小時間
        Parameter2 = 3.0f;  // 最大時間
        Parameter3 = 10.0f; // 水平移動速度
    } else if (type == EditorNodeType::ActionShoot) {
        Parameter = 1.0f; // クールダウン秒数
    } else if (type == EditorNodeType::ActionLockOn) {
        Parameter = 1.0f; // 1.0=ON, 0.0=OFF
    } else if (type == EditorNodeType::ActionComboStep) {
        Parameter = 0.5f;  // 1段のモーション待機時間(秒)
        Parameter2 = 0.0f; // コンボ間隔オーバーライド(0=デフォルト0.15s)
    } else if (type == EditorNodeType::ActionComboFull) {
        Parameter = 0.5f;  // 1段あたりの時間(秒)
        Parameter2 = 0.0f; // 最大段数(0=全段)
        Parameter3 = 0.0f; // コンボ間隔オーバーライド(0=デフォルト0.15s)
    } else if (type == EditorNodeType::ActionBurstShoot) {
        Parameter = 0.2f;  // 発射間隔(秒)
        Parameter2 = 3.0f; // 弾数
        Parameter3 = 0.5f; // クールダウン(秒)
        Parameter4 = 0.0f; // 拡散角度(度, 0=直進)
        Parameter5 = 0.0f; // 0=拡散弾, 1=ホーミング弾
    }
}

EditorLink::EditorLink(int id, ed::PinId start, ed::PinId end)
    : ID(id), StartPinID(start), EndPinID(end) {}

// ---------------------------------------------------------
// Editor  (以下は元の実装をそのまま維持)
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

bool BehaviorTreeEditor::IsInputPin(ed::PinId p) { return (p.Get() % 10) == 1; }
bool BehaviorTreeEditor::IsOutputPin(ed::PinId p) { return (p.Get() % 10) == 2; }
bool BehaviorTreeEditor::IsSuccessPin(ed::PinId p) { return (p.Get() % 10) == 3; }
bool BehaviorTreeEditor::IsFailurePin(ed::PinId p) { return (p.Get() % 10) == 4; }

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
        return "プレイヤーが指定されたステート（Idle/Move/Jump/Air/FlyIdle/FlyMove/Rush/EnergyCharge）かチェック";
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
        return "地上からジャンプする（指定した力で跳躍）";
    case EditorNodeType::ActionJumpToFly:
        return "地上からジャンプし、一定時間後に飛行状態へ遷移";
    case EditorNodeType::ActionFlyAscend:
        return "飛行中に上昇する（指定速度と時間）";
    case EditorNodeType::ActionFlyDescend:
        return "飛行中に下降する（指定速度と時間）";
    case EditorNodeType::ActionFlyToGround:
        return "飛行状態から地上へ着地する";
    case EditorNodeType::ActionFlyApproach:
        return "飛行中に水平方向へプレイヤーへ接近する (param: 最小秒, param2: 最大秒, param3: 速度)";
    case EditorNodeType::ActionShoot:
        return "弾を1発発射し、指定秒数(param)クールダウンする";
    case EditorNodeType::ActionLockOn:
        return "ロックオン状態を切り替える (param: 1=ON, 0=OFF)";
    case EditorNodeType::ConditionIsLockOn:
        return "ロックオン中かどうかをチェックする";
    case EditorNodeType::ActionComboStep:
        return "コンボを1段だけ実行する (param: モーション待機秒, param2: コンボ間隔秒(0=デフォルト))";
    case EditorNodeType::ActionComboFull:
        return "コンボを全段実行する (param: 1段の時間秒, param2: 最大段数(0=全段), param3: コンボ間隔秒(0=デフォルト))";
    case EditorNodeType::ActionBurstShoot:
        return "弾をN連発する (param: 発射間隔秒, param2: 弾数, param3: クールダウン秒, param4: 拡散角度度, param5: 0=拡散弾/1=ホーミング弾)";
    case EditorNodeType::ActionEnergyCharge:
        return "エネルギーをチャージする (param: 速度倍率, param2: 目標比率 0.0〜1.0)";
    default:
        return "説明なし";
    }
}

// ------------------------------------------------------------------
// BuildAndRunTree  (エディタ側: Debugのみ)
// ------------------------------------------------------------------
void BehaviorTreeEditor::BuildAndRunTree() {
    m_nodeInstanceMap.clear();
    m_RuntimeRoot = nullptr;

    int rootId = FindRootNodeId();
    if (rootId == -1)
        return;

    m_RuntimeRoot = BuildNodeRecursive(rootId);
    if (m_RuntimeRoot) {
        m_RuntimeRoot->SetContext(m_DebugEnemy, m_DebugPlayer);
        if (m_DebugEnemy)
            m_DebugEnemy->SetBehaviorTree(m_RuntimeRoot);
    }
}

std::shared_ptr<BTNode> BehaviorTreeEditor::BuildNodeRecursive(int editorNodeId) {
    auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(),
                           [editorNodeId](const EditorNode &n) { return (int)n.ID.Get() == editorNodeId; });
    if (it == m_Nodes.end())
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
    case EditorNodeType::ActionLockOn:
        runtimeNode = std::make_shared<EnemyLockOnNode>(eNode.Parameter >= 1.0f);
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
        runtimeNode = std::make_shared<EnemyComboStepNode>(
            eNode.Parameter > 0.0f ? eNode.Parameter : 0.5f,
            eNode.Parameter2);
        break;
    case EditorNodeType::ActionComboFull:
        runtimeNode = std::make_shared<EnemyComboFullNode>(
            eNode.Parameter > 0.0f ? eNode.Parameter : 0.5f,
            static_cast<int>(eNode.Parameter2),
            eNode.Parameter3);
        break;
    case EditorNodeType::ActionBurstShoot:
        runtimeNode = std::make_shared<EnemyBurstShootNode>(
            eNode.Parameter > 0.0f ? eNode.Parameter : 0.2f,
            eNode.Parameter2 > 0.0f ? static_cast<int>(eNode.Parameter2) : 3,
            eNode.Parameter3 > 0.0f ? eNode.Parameter3 : 0.5f,
            eNode.Parameter4,
            eNode.Parameter5 >= 1.0f);
        break;
    case EditorNodeType::ConditionEnergyLow:
        runtimeNode = std::make_shared<IsEnergyLowNode>(eNode.Parameter);
        break;
    case EditorNodeType::ActionEnergyCharge:
        runtimeNode = std::make_shared<EnemyEnergyChargeNode>(
            eNode.Parameter > 0.0f ? eNode.Parameter : 1.0f,
            eNode.Parameter2 > 0.0f ? eNode.Parameter2 : 1.0f);
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
    m_nodeInstanceMap[editorNodeId] = runtimeNode;

    bool isLeaf = eNode.IsActionNode() ||
                  eNode.IsConditionNode();

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
    for (const auto &link : m_Links) {
        if ((int)link.StartPinID.Get() == outputPinId) {
            int endPin = (int)link.EndPinID.Get();
            children.push_back((endPin - 1) / 10);
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
                int childNodeId = ((int)link.EndPinID.Get() - 1) / 10;
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
        if (!hasInput)
            return (int)node.ID.Get();
    }
    return -1;
}

// ------------------------------------------------------------------
// SaveTree / LoadTree
// ------------------------------------------------------------------
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
    for (const auto &link : m_Links) {
        json l;
        l["id"] = (int)link.ID.Get();
        l["start"] = (int)link.StartPinID.Get();
        l["end"] = (int)link.EndPinID.Get();
        linksJson.push_back(l);
    }
    handler.Save("links", linksJson);
    std::cout << "BTを保存しました: " << fileName << ".json" << std::endl;
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

        m_Nodes.push_back(node);
        ed::SetNodePosition(node.ID, ImVec2(x, y));
        if (id > maxNodeId)
            maxNodeId = id;
    }
    m_NextNodeId = maxNodeId + 1;
    m_NextPinId = maxPinId + 1;

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
    std::cout << "BTを読込しました: " << fileName << std::endl;
}

// ------------------------------------------------------------------
// OnImGuiRender  ※元の実装をそのまま維持 (長いので省略なし)
// ------------------------------------------------------------------
void BehaviorTreeEditor::OnImGuiRender() {
    ed::SetCurrentEditor(m_Context);

    // --- 上部コントロール ---
    ImGui::Text("ファイル名:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("##FileName", m_InputFileNameBuf, IM_ARRAYSIZE(m_InputFileNameBuf));
    ImGui::SameLine();
    ImGui::Text(".json");

    ImGui::SameLine();
    if (ImGui::Button("保存")) {
        SaveTree();
    }
    ImGui::SameLine();
    if (ImGui::Button("読込")) {
        m_ShowLoadWindow = true;
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    if (ImGui::Button("ビルド＆実行")) {
        BuildAndRunTree();
        m_IsRunning = true;
        m_LastResultText = "実行中...";
        m_LastResultColor = ImVec4(1, 1, 1, 1);
    }
    ImGui::SameLine();
    if (ImGui::Button("停止")) {
        m_IsRunning = false;
        if (m_RuntimeRoot) {
            try {
                m_RuntimeRoot->Reset();
            } catch (...) {
            }
        }
        m_nodeInstanceMap.clear();
        m_statusTimers.clear();
        m_RuntimeRoot = nullptr;
        if (m_DebugEnemy) {
            m_DebugEnemy->SetBehaviorTree(nullptr);
            m_DebugEnemy->SetVelocity({0, 0, 0});
            m_DebugEnemy->SetMoveSpeed(0.0f);
        }
        m_LastResultText = "停止中";
        m_LastResultColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    }

    ImGui::SameLine();
    ImGui::TextColored(m_LastResultColor, "状態: %s", m_LastResultText.c_str());

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "O");
    ImGui::SameLine();
    ImGui::Text("実行中");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "X");
    ImGui::SameLine();
    ImGui::Text("失敗");

    // --- ロードウィンドウ ---
    if (m_ShowLoadWindow) {
        ImGui::Begin("ビヘイビアツリーを読込", &m_ShowLoadWindow);
        static std::string startPath = "BehaviorTree";
        ShowJsonFile(m_SelectedFileName, startPath);
        if (!m_SelectedFileName.empty()) {
            if (ImGui::Button("選択したファイルを読込")) {
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
        try {
            for (auto const &[nodeId, runtimeNode] : m_nodeInstanceMap) {
                if (runtimeNode && runtimeNode->GetStatus() != NodeStatus::Idle)
                    m_statusTimers[nodeId] = 0.5f;
            }
            NodeStatus rootStatus = m_RuntimeRoot->GetStatus();
            if (rootStatus == NodeStatus::Success) {
                m_LastResultText = "[成功]";
                m_LastResultColor = ImVec4(0, 1, 0, 1);
            } else if (rootStatus == NodeStatus::Failure) {
                m_LastResultText = "[失敗]";
                m_LastResultColor = ImVec4(1, 0, 0, 1);
            } else if (rootStatus == NodeStatus::Running) {
                m_LastResultText = "実行中...";
                m_LastResultColor = ImVec4(0.3f, 0.5f, 0.8f, 1.0f);
            }
        } catch (...) {
            m_IsRunning = false;
            m_RuntimeRoot = nullptr;
            m_nodeInstanceMap.clear();
            m_statusTimers.clear();
            if (m_DebugEnemy) {
                m_DebugEnemy->SetBehaviorTree(nullptr);
                m_DebugEnemy->SetVelocity({0, 0, 0});
                m_DebugEnemy->SetMoveSpeed(0.0f);
            }
            m_LastResultText = "エラーで停止";
            m_LastResultColor = ImVec4(1, 0.5f, 0, 1);
        }
    }

    float dt = ImGui::GetIO().DeltaTime;
    for (auto it = m_statusTimers.begin(); it != m_statusTimers.end();) {
        it->second -= dt;
        it = (it->second <= 0.0f) ? m_statusTimers.erase(it) : std::next(it);
    }

    // --- ノードエディタ ---
    ed::Begin("ビヘイビアツリーエディタ", ImVec2(0, 0));

    for (auto &node : m_Nodes) {
        ImGui::PushID((int)node.ID.Get());

        int nodeId = (int)node.ID.Get();
        NodeStatus status = NodeStatus::Idle;
        bool showHighlight = false;

        if (m_IsRunning && m_nodeInstanceMap.count(nodeId) && m_nodeInstanceMap[nodeId])
            status = m_nodeInstanceMap[nodeId]->GetStatus();

        if (status != NodeStatus::Idle || m_statusTimers.count(nodeId)) {
            showHighlight = true;
            if (status == NodeStatus::Idle && m_nodeInstanceMap.count(nodeId) && m_nodeInstanceMap[nodeId])
                status = m_nodeInstanceMap[nodeId]->GetStatus();
        }

        if (showHighlight) {
            if (status == NodeStatus::Running) {
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
                ed::PushStyleColor(ed::StyleColor_NodeBorder, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            } else if (status == NodeStatus::Failure) {
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ed::PushStyleColor(ed::StyleColor_NodeBorder, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            }
        }

        ed::BeginNode(node.ID);
        ImGui::BeginGroup();
        ImGui::Text("%s", node.Title.c_str());
        if (showHighlight && m_IsRunning) {
            ImGui::SameLine();
            if (status == NodeStatus::Running)
                ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "[実行中]");
            else if (status == NodeStatus::Failure)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[失敗]");
        }
        ImGui::EndGroup();

        ed::BeginPin(node.InputPinID, ed::PinKind::Input);
        ImGui::PushID((int)node.InputPinID.Get());
        ImGui::Text("-> 入力");
        ImGui::PopID();
        ed::EndPin();

        ImGui::PushItemWidth(80);
        bool paramChanged = false;
        // ノードIDを含む一意なウィジェットIDバッファ
        char wid1[32], wid2[32], wid3[32];
        snprintf(wid1, sizeof(wid1), "##p1_%d", nodeId);
        snprintf(wid2, sizeof(wid2), "##p2_%d", nodeId);
        snprintf(wid3, sizeof(wid3), "##p3_%d", nodeId);

        if (node.Type == EditorNodeType::ConditionPlayerClose) {
            ImGui::Text("最小距離");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.1f, 0.0f, 100.0f, "%.1fm"))
                paramChanged = true;
            ImGui::Text("最大距離");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid2, &node.Parameter2, 0.1f, 0.0f, 100.0f, "%.1fm"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ConditionHealthLow) {
            ImGui::Text("HP比率");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.01f, 0.0f, 1.0f, "%.2f"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ConditionEnergyLow) {
            ImGui::Text("エネルギー比率");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.01f, 0.0f, 1.0f, "%.2f"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ConditionPlayerState) {
            // ノードエディタ内ではComboが動作しないため、ボタンで切り替える方式
            static const char *kPlayerStates[] = {
                "Idle", "Move", "Jump", "Air",
                "FlyIdle", "FlyMove", "Rush", "EnergyCharge"};
            static constexpr int kPlayerStateCount = 8;
            ImGui::Text("ステート: %s", node.StateNameParameter.c_str());
            // 4つずつ2行に並べてボタン表示
            for (int si = 0; si < kPlayerStateCount; ++si) {
                char btnId[32];
                snprintf(btnId, sizeof(btnId), "##sb_%d_%d", nodeId, si);
                bool isSelected = (node.StateNameParameter == kPlayerStates[si]);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
                }
                if (ImGui::SmallButton((std::string(kPlayerStates[si]) + btnId).c_str())) {
                    node.StateNameParameter = kPlayerStates[si];
                    paramChanged = true;
                }
                if (isSelected) {
                    ImGui::PopStyleColor(2);
                }
                // 4つごとに改行、それ以外は横に並べる
                if (si % 4 != 3)
                    ImGui::SameLine();
            }
        } else if (node.Type == EditorNodeType::DecoratorWeight) {
            ImGui::Text("出力数:");
            ImGui::SameLine();
            int outputCount = (int)node.WeightedOutputs.size();
            if (ImGui::InputInt(wid1, &outputCount, 1, 1)) {
                outputCount = std::max(1, std::min(10, outputCount));
                int oldSize = (int)node.WeightedOutputs.size();
                node.WeightedOutputs.resize(outputCount);
                for (int i = oldSize; i < outputCount; ++i) {
                    node.WeightedOutputs[i].PinID = m_NextPinId++;
                    node.WeightedOutputs[i].Weight = 1.0f;
                }
                paramChanged = true;
            }
            float totalWeight = 0.0f;
            for (auto &o : node.WeightedOutputs)
                totalWeight += o.Weight;
            for (int i = 0; i < (int)node.WeightedOutputs.size(); ++i) {
                ImGui::PushID(i);
                float pct = (totalWeight > 0.0f) ? node.WeightedOutputs[i].Weight / totalWeight * 100.0f : 0.0f;
                ImGui::Text("出力%d (%.1f%%)", i + 1, pct);
                ImGui::SameLine();
                char wwid[32];
                snprintf(wwid, sizeof(wwid), "##w_%d_%d", nodeId, i);
                if (ImGui::DragFloat(wwid, &node.WeightedOutputs[i].Weight, 0.1f, 0.0f, 100.0f, "%.1f"))
                    paramChanged = true;
                ImGui::PopID();
            }
        } else if (node.Type == EditorNodeType::ActionApproach ||
                   node.Type == EditorNodeType::ActionDash ||
                   node.Type == EditorNodeType::ActionStrafe ||
                   node.Type == EditorNodeType::ActionRetreat) {
            ImGui::Text("最小時間");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.1f, 0.0f, 10.0f, "%.1fs"))
                paramChanged = true;
            ImGui::Text("最大時間");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid2, &node.Parameter2, 0.1f, 0.0f, 10.0f, "%.1fs"))
                paramChanged = true;
            ImGui::Text("速度");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid3, &node.Parameter3, 0.01f, 0.0f, 2.0f, "%.2f"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ActionIdle) {
            ImGui::Text("待機時間");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.1f, 0.1f, 10.0f, "%.1fs"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ActionJump ||
                   node.Type == EditorNodeType::ActionJumpToFly) {
            ImGui::Text("ジャンプ力");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.5f, 5.0f, 30.0f, "%.1f"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ActionFlyAscend ||
                   node.Type == EditorNodeType::ActionFlyDescend ||
                   node.Type == EditorNodeType::ActionFlyApproach) {
            ImGui::Text("最小時間");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.1f, 0.0f, 10.0f, "%.1fs"))
                paramChanged = true;
            ImGui::Text("最大時間");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid2, &node.Parameter2, 0.1f, 0.0f, 10.0f, "%.1fs"))
                paramChanged = true;
            ImGui::Text("速度");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid3, &node.Parameter3, 0.5f, 5.0f, 30.0f, "%.1f"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ActionComboStep) {
            ImGui::Text("モーション待機(秒)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.05f, 0.1f, 5.0f, "%.2fs"))
                paramChanged = true;
            ImGui::Text("コンボ間隔(0=デフォルト)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid2, &node.Parameter2, 0.01f, 0.0f, 2.0f, "%.2fs"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ActionComboFull) {
            ImGui::Text("1段の時間(秒)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.05f, 0.1f, 5.0f, "%.2fs"))
                paramChanged = true;
            ImGui::Text("最大段数(0=全段)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid2, &node.Parameter2, 1.0f, 0.0f, 8.0f, "%.0f段"))
                paramChanged = true;
            ImGui::Text("コンボ間隔(0=デフォルト)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid3, &node.Parameter3, 0.01f, 0.0f, 2.0f, "%.2fs"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ActionBurstShoot) {
            // ウィジェット用IDを追加で確保
            char wid4[64], wid5[64];
            snprintf(wid4, sizeof(wid4), "##param4_%d", (int)node.ID.Get());
            snprintf(wid5, sizeof(wid5), "##param5_%d", (int)node.ID.Get());

            ImGui::Text("発射間隔(秒)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.01f, 0.01f, 5.0f, "%.2fs"))
                paramChanged = true;
            ImGui::Text("弾数");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid2, &node.Parameter2, 1.0f, 1.0f, 30.0f, "%.0f発"))
                paramChanged = true;
            ImGui::Text("クールダウン(秒)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid3, &node.Parameter3, 0.05f, 0.0f, 10.0f, "%.2fs"))
                paramChanged = true;
            ImGui::Text("拡散角度(度)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid4, &node.Parameter4, 1.0f, 0.0f, 180.0f, "%.0f度"))
                paramChanged = true;
            // ホーミングチェックボックス
            bool isHoming = (node.Parameter5 >= 1.0f);
            if (ImGui::Checkbox("ホーミング弾", &isHoming)) {
                node.Parameter5 = isHoming ? 1.0f : 0.0f;
                paramChanged = true;
            }
            if (!isHoming) {
                ImGui::SameLine();
                ImGui::TextDisabled("(拡散弾モード)");
            }
        } else if (node.Type == EditorNodeType::ActionEnergyCharge) {
            ImGui::Text("速度倍率");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid1, &node.Parameter, 0.1f, 0.1f, 10.0f, "%.1f倍"))
                paramChanged = true;
            ImGui::Text("目標比率(1.0=最大まで)");
            ImGui::SameLine();
            if (ImGui::DragFloat(wid2, &node.Parameter2, 0.01f, 0.0f, 1.0f, "%.2f"))
                paramChanged = true;
        }

        if (paramChanged && m_IsRunning)
            BuildAndRunTree();
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // 出力ピン
        if (node.IsConditionNode()) {
            ed::BeginPin(node.SuccessPinID, ed::PinKind::Output);
            ImGui::PushID((int)node.SuccessPinID.Get());
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "成功 ->");
            ImGui::PopID();
            ed::EndPin();
            ed::BeginPin(node.FailurePinID, ed::PinKind::Output);
            ImGui::PushID((int)node.FailurePinID.Get());
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "失敗 ->");
            ImGui::PopID();
            ed::EndPin();
        } else if (node.IsWeightNode()) {
            float totalW = 0.0f;
            for (auto &o : node.WeightedOutputs)
                totalW += o.Weight;
            for (int i = 0; i < (int)node.WeightedOutputs.size(); ++i) {
                float pct = (totalW > 0.0f) ? node.WeightedOutputs[i].Weight / totalW * 100.0f : 0.0f;
                ed::BeginPin(node.WeightedOutputs[i].PinID, ed::PinKind::Output);
                ImGui::PushID((int)node.WeightedOutputs[i].PinID.Get());
                ImGui::Text("出力%d(%.0f%%) ->", i + 1, pct);
                ImGui::PopID();
                ed::EndPin();
            }
        } else if (!node.IsActionNode()) {
            ed::BeginPin(node.OutputPinID, ed::PinKind::Output);
            ImGui::PushID((int)node.OutputPinID.Get());
            ImGui::Text("出力 ->");
            ImGui::PopID();
            ed::EndPin();
        }

        ed::EndNode();
        if (showHighlight) {
            if (status == NodeStatus::Running || status == NodeStatus::Failure) {
                ed::PopStyleColor(2);
            }
        }
        ImGui::PopID();
    }

    // リンク描画
    for (auto &link : m_Links) {
        ed::Link(link.ID, link.StartPinID, link.EndPinID);
    }

    // 右クリックメニュー
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("Create New Node");
        Vector2 mousePos = Input::GetInstance()->GetMousePos();
        m_CreatePos = ed::ScreenToCanvas(ImVec2(mousePos.x, mousePos.y));
    }

    if (ImGui::BeginPopup("Create New Node")) {
        ImGui::Text("ノードを追加");
        ImGui::Separator();

        auto addBtn = [&](const char *label, EditorNodeType type) {
            if (ImGui::MenuItem(label)) {
                CreateNode(label, type);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", GetNodeDescription(type));
        };

        ImGui::Text("[コンポジット]");
        addBtn("シーケンス", EditorNodeType::Sequence);
        addBtn("シーケンス(完遂優先)", EditorNodeType::SequenceOnce);
        addBtn("セレクター", EditorNodeType::Selector);
        addBtn("ランダムセレクター", EditorNodeType::SelectorRandom);
        addBtn("重みデコレーター", EditorNodeType::DecoratorWeight);
        ImGui::Separator();

        ImGui::Text("[条件]");
        addBtn("距離チェック", EditorNodeType::ConditionPlayerClose);
        addBtn("HP低下チェック", EditorNodeType::ConditionHealthLow);
        addBtn("エネルギー低下チェック", EditorNodeType::ConditionEnergyLow);
        addBtn("地上チェック", EditorNodeType::ConditionIsGrounded);
        addBtn("空中チェック", EditorNodeType::ConditionIsAirborne);
        addBtn("ステートチェック", EditorNodeType::ConditionPlayerState);
        addBtn("ロックオンチェック", EditorNodeType::ConditionIsLockOn);
        ImGui::Separator();

        ImGui::Text("[アクション]");
        addBtn("基本アクション", EditorNodeType::ActionRun);
        addBtn("接近", EditorNodeType::ActionApproach);
        addBtn("ダッシュ", EditorNodeType::ActionDash);
        addBtn("左右移動", EditorNodeType::ActionStrafe);
        addBtn("後退", EditorNodeType::ActionRetreat);
        addBtn("攻撃", EditorNodeType::ActionAttack);
        addBtn("待機", EditorNodeType::ActionIdle);
        ImGui::Separator();

        ImGui::Text("[近接コンボ]");
        addBtn("コンボ1段", EditorNodeType::ActionComboStep);
        addBtn("コンボ全段", EditorNodeType::ActionComboFull);
        ImGui::Separator();

        ImGui::Text("[射撃]");
        addBtn("弾発射(単発)", EditorNodeType::ActionShoot);
        addBtn("弾発射(連射)", EditorNodeType::ActionBurstShoot);
        addBtn("ロックオン切替", EditorNodeType::ActionLockOn);
        ImGui::Separator();

        ImGui::Text("[ジャンプ・飛行]");
        addBtn("ジャンプ", EditorNodeType::ActionJump);
        addBtn("飛行遷移", EditorNodeType::ActionJumpToFly);
        addBtn("上昇", EditorNodeType::ActionFlyAscend);
        addBtn("下降", EditorNodeType::ActionFlyDescend);
        addBtn("空中接近", EditorNodeType::ActionFlyApproach);
        addBtn("着地", EditorNodeType::ActionFlyToGround);
        ImGui::Separator();

        ImGui::Text("[エネルギー]");
        addBtn("エネルギーチャージ", EditorNodeType::ActionEnergyCharge);

        ImGui::EndPopup();
    }
    ed::Resume();

    HandleCreateAction();
    DeleteSelectedItems();

    ed::End();
    ed::SetCurrentEditor(nullptr);
}

void BehaviorTreeEditor::CreateNode(const std::string &title, EditorNodeType type) {
    int id = m_NextNodeId++;
    EditorNode node(id, title, type);
    m_Nodes.push_back(node);
    ed::SetNodePosition(node.ID, m_CreatePos);
}

void BehaviorTreeEditor::HandleCreateAction() {
    if (ed::BeginCreate()) {
        ed::PinId start, end;
        if (ed::QueryNewLink(&start, &end)) {
            bool validLink = false;
            int nodeId, outputIndex;
            bool isWeightedStart = IsWeightedOutputPin(start, nodeId, outputIndex);
            bool isWeightedEnd = IsWeightedOutputPin(end, nodeId, outputIndex);

            if ((IsOutputPin(start) || IsSuccessPin(start) || IsFailurePin(start) || isWeightedStart) && IsInputPin(end)) {
                validLink = true;
            } else if (IsInputPin(start) && (IsOutputPin(end) || IsSuccessPin(end) || IsFailurePin(end) || isWeightedEnd)) {
                std::swap(start, end);
                validLink = true;
            }

            if (validLink && ed::AcceptNewItem())
                m_Links.emplace_back(m_NextLinkId++, start, end);
        }
    }
    ed::EndCreate();
}

void BehaviorTreeEditor::DeleteSelectedItems() {
    if (ed::BeginDelete()) {
        ed::NodeId nodeId;
        while (ed::QueryDeletedNode(&nodeId)) {
            if (ed::AcceptDeletedItem()) {
                int deletedNodeId = (int)nodeId.Get();
                auto linkIt = m_Links.begin();
                while (linkIt != m_Links.end()) {
                    bool shouldDelete = false;
                    for (const auto &node : m_Nodes) {
                        if ((int)node.ID.Get() == deletedNodeId) {
                            if (linkIt->StartPinID == node.InputPinID ||
                                linkIt->StartPinID == node.OutputPinID ||
                                linkIt->StartPinID == node.SuccessPinID ||
                                linkIt->StartPinID == node.FailurePinID ||
                                linkIt->EndPinID == node.InputPinID ||
                                linkIt->EndPinID == node.OutputPinID ||
                                linkIt->EndPinID == node.SuccessPinID ||
                                linkIt->EndPinID == node.FailurePinID) {
                                shouldDelete = true;
                                break;
                            }
                            for (const auto &output : node.WeightedOutputs) {
                                if (linkIt->StartPinID == output.PinID ||
                                    linkIt->EndPinID == output.PinID) {
                                    shouldDelete = true;
                                    break;
                                }
                            }
                            if (shouldDelete)
                                break;
                        }
                    }
                    linkIt = shouldDelete ? m_Links.erase(linkIt) : std::next(linkIt);
                }
                m_nodeInstanceMap.erase(deletedNodeId);
                auto it = std::remove_if(m_Nodes.begin(), m_Nodes.end(),
                                         [nodeId](const EditorNode &n) { return n.ID == nodeId; });
                m_Nodes.erase(it, m_Nodes.end());
                if (m_IsRunning)
                    BuildAndRunTree();
            }
        }

        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            if (ed::AcceptDeletedItem()) {
                auto it = std::remove_if(m_Links.begin(), m_Links.end(),
                                         [linkId](const EditorLink &l) { return l.ID == linkId; });
                m_Links.erase(it, m_Links.end());
                if (m_IsRunning)
                    BuildAndRunTree();
            }
        }
    }
    ed::EndDelete();
}

#endif // _DEBUG
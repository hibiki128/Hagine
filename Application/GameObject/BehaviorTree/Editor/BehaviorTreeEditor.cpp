#include "BehaviorTreeEditor.h"

// ユーザー様の環境に合わせてパスを調整しています
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "Input.h"
#include <ShowFolder/ShowFolder.h>

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
    SuccessPinID = id * 10 + 3; // 条件ノード用
    FailurePinID = id * 10 + 4; // 条件ノード用

    // デフォルト値設定
    if (type == EditorNodeType::ConditionPlayerClose) {
        Parameter = 0.0f;   // Min Dist
        Parameter2 = 10.0f; // Max Dist
    } else if (type == EditorNodeType::DecoratorWeight) {
        // 重み付けノード: デフォルトで2つの出力を持つ
        WeightedOutputs.resize(2);
        WeightedOutputs[0].PinID = id * 10 + 5;
        WeightedOutputs[0].Weight = 1.0f;
        WeightedOutputs[1].PinID = id * 10 + 6;
        WeightedOutputs[1].Weight = 1.0f;
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
    } else if (type == EditorNodeType::ActionIdle) {
        Parameter = 1.0f; // デフォルト1秒待機
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
    if (m_Context) {
        ed::DestroyEditor(m_Context);
    }
}

bool BehaviorTreeEditor::IsInputPin(ed::PinId pinId) { return (pinId.Get() % 10) == 1; }
bool BehaviorTreeEditor::IsOutputPin(ed::PinId pinId) { return (pinId.Get() % 10) == 2; }
bool BehaviorTreeEditor::IsSuccessPin(ed::PinId pinId) { return (pinId.Get() % 10) == 3; }
bool BehaviorTreeEditor::IsFailurePin(ed::PinId pinId) { return (pinId.Get() % 10) == 4; }

bool BehaviorTreeEditor::IsWeightedOutputPin(ed::PinId pinId, int &outNodeId, int &outOutputIndex) {
    for (auto &node : m_Nodes) {
        if (node.IsWeightNode()) {
            for (int i = 0; i < node.WeightedOutputs.size(); ++i) {
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

void BehaviorTreeEditor::ParsePathToFolderAndFile(const std::string &fullPath, std::string &outFolder, std::string &outFile) {
    fs::path path(fullPath);
    outFile = path.stem().string();
    fs::path parent = path.parent_path();
    if (parent.has_filename())
        outFolder = parent.filename().string();
    else
        outFolder = "BehaviorTree";
}

const char *BehaviorTreeEditor::GetNodeDescription(EditorNodeType type) {
    switch (type) {
    case EditorNodeType::Sequence:
        return "子ノードを順番に実行し、全て成功で成功を返す";
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
    default:
        return "説明なし";
    }
}

// ---------------------------------------------------------
// OnImGuiRender
// ---------------------------------------------------------
void BehaviorTreeEditor::OnImGuiRender() {
    // コンテキスト設定を一番最初に行う
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

        // ★修正: RuntimeRootをリセット前により慎重に処理
        if (m_RuntimeRoot) {
            try {
                m_RuntimeRoot->Reset(); // ノードの状態をリセット
            } catch (...) {
                // リセット中に例外が発生した場合も安全に継続
            }
        }

        // インスタンスマップとタイマーをクリア
        m_nodeInstanceMap.clear();
        m_statusTimers.clear();

        // RuntimeRootをnullに設定
        m_RuntimeRoot = nullptr;

        // ★重要: 敵のBehaviorTreeもクリアして動きを完全に止める
        if (m_DebugEnemy) {
            m_DebugEnemy->SetBehaviorTree(nullptr); // ツリーをクリア
            m_DebugEnemy->SetVelocity({0, 0, 0});
            m_DebugEnemy->SetMoveSpeed(0.0f); // 移動速度もゼロに
        }

        m_LastResultText = "停止中";
        m_LastResultColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    }

    ImGui::SameLine();
    ImGui::TextColored(m_LastResultColor, "状態: %s", m_LastResultText.c_str());

    // ★追加: ノードの色の凡例を表示(実行中と失敗のみ)
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

    // --- ステータス更新 (視覚化用) ---
    if (m_IsRunning && m_RuntimeRoot) {
        try {
            // ★変更: エディタ側ではTickを呼ばない
            // Enemy::Update()内でrootNode_->Tick()が呼ばれるため、
            // ここでは状態の監視のみを行う

            // ノードのステータスをタイマーに記録
            for (auto const &[nodeId, runtimeNode] : m_nodeInstanceMap) {
                if (runtimeNode && runtimeNode->GetStatus() != NodeStatus::Idle) {
                    m_statusTimers[nodeId] = 0.5f;
                }
            }

            // ルートノードの結果を表示用に取得
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
            // エラーが発生した場合は実行を停止
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

    // タイマーの更新
    float dt = ImGui::GetIO().DeltaTime;
    for (auto it = m_statusTimers.begin(); it != m_statusTimers.end();) {
        it->second -= dt;
        if (it->second <= 0.0f)
            it = m_statusTimers.erase(it);
        else
            ++it;
    }

    // --- ノードエディタ ---
    ed::Begin("ビヘイビアツリーエディタ", ImVec2(0, 0));

    for (auto &node : m_Nodes) {
        // ★重要: IDスコープを開始 (同じラベル名の干渉を防ぐ)
        ImGui::PushID((int)node.ID.Get());

        int nodeId = (int)node.ID.Get();
        NodeStatus status = NodeStatus::Idle;
        bool showHighlight = false;

        // ★修正: nullチェックを追加
        if (m_IsRunning && m_nodeInstanceMap.count(nodeId) && m_nodeInstanceMap[nodeId]) {
            status = m_nodeInstanceMap[nodeId]->GetStatus();
        }
        if (status != NodeStatus::Idle || m_statusTimers.count(nodeId)) {
            showHighlight = true;
            if (status == NodeStatus::Idle && m_nodeInstanceMap.count(nodeId) && m_nodeInstanceMap[nodeId]) {
                status = m_nodeInstanceMap[nodeId]->GetStatus();
            }
        }

        if (showHighlight) {
            if (status == NodeStatus::Running) {
                // ★実行中は薄い青色
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
                ed::PushStyleColor(ed::StyleColor_NodeBorder, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            } else if (status == NodeStatus::Failure) {
                // ★失敗は明るい赤色
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ed::PushStyleColor(ed::StyleColor_NodeBorder, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            }
            // 成功の色は設定しない(デフォルトのまま)
        }

        ed::BeginNode(node.ID);

        // ★追加: ステータス表示をノードタイトルの横に表示
        ImGui::BeginGroup();
        ImGui::Text("%s", node.Title.c_str());

        // 実行中のステータスを表示(実行中と失敗のみ)
        if (showHighlight && m_IsRunning) {
            ImGui::SameLine();
            if (status == NodeStatus::Running) {
                ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "[実行中]");
            } else if (status == NodeStatus::Failure) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[失敗]");
            }
            // 成功は表示しない
        }
        ImGui::EndGroup();

        // 入力ピン
        ed::BeginPin(node.InputPinID, ed::PinKind::Input);
        ImGui::Text("-> 入力");
        ed::EndPin();

        // --- パラメータUI ---
        ImGui::PushItemWidth(80);

        bool paramChanged = false; // パラメータが変更されたかフラグ

        if (node.Type == EditorNodeType::ConditionPlayerClose) {
            ImGui::Text("最小距離");
            ImGui::SameLine();
            if (ImGui::DragFloat("##min", &node.Parameter, 0.1f, 0.0f, 100.0f, "%.1fm"))
                paramChanged = true;
            ImGui::Text("最大距離");
            ImGui::SameLine();
            if (ImGui::DragFloat("##max", &node.Parameter2, 0.1f, 0.0f, 100.0f, "%.1fm"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ConditionHealthLow) {
            ImGui::Text("HP比率");
            ImGui::SameLine();
            if (ImGui::DragFloat("##hp", &node.Parameter, 0.01f, 0.0f, 1.0f, "%.2f"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::DecoratorWeight) {
            // ★修正: 重み付けノードは各出力の重みを表示・編集
            ImGui::Text("出力数:");
            ImGui::SameLine();
            int outputCount = (int)node.WeightedOutputs.size();
            if (ImGui::InputInt("##outputCount", &outputCount, 1, 1)) {
                if (outputCount < 1)
                    outputCount = 1;
                if (outputCount > 10)
                    outputCount = 10;

                int oldSize = (int)node.WeightedOutputs.size();
                node.WeightedOutputs.resize(outputCount);

                // 新しく追加された出力のPinIDを設定
                for (int i = oldSize; i < outputCount; ++i) {
                    node.WeightedOutputs[i].PinID = m_NextPinId++;
                    node.WeightedOutputs[i].Weight = 1.0f;
                }
                paramChanged = true;
            }

            // 各出力の重みを編集
            float totalWeight = 0.0f;
            for (auto &output : node.WeightedOutputs) {
                totalWeight += output.Weight;
            }

            for (int i = 0; i < node.WeightedOutputs.size(); ++i) {
                ImGui::PushID(i);
                float percentage = (totalWeight > 0.0f) ? (node.WeightedOutputs[i].Weight / totalWeight * 100.0f) : 0.0f;
                ImGui::Text("出力%d (%.1f%%)", i + 1, percentage);
                ImGui::SameLine();
                if (ImGui::DragFloat("##weight", &node.WeightedOutputs[i].Weight, 0.1f, 0.0f, 100.0f, "%.1f"))
                    paramChanged = true;
                ImGui::PopID();
            }
        } else if (node.Type == EditorNodeType::ActionApproach ||
                   node.Type == EditorNodeType::ActionDash ||
                   node.Type == EditorNodeType::ActionStrafe ||
                   node.Type == EditorNodeType::ActionRetreat) {
            ImGui::Text("最小時間");
            ImGui::SameLine();
            if (ImGui::DragFloat("##minTime", &node.Parameter, 0.1f, 0.0f, 10.0f, "%.1fs"))
                paramChanged = true;
            ImGui::Text("最大時間");
            ImGui::SameLine();
            if (ImGui::DragFloat("##maxTime", &node.Parameter2, 0.1f, 0.0f, 10.0f, "%.1fs"))
                paramChanged = true;
            ImGui::Text("速度");
            ImGui::SameLine();
            if (ImGui::DragFloat("##speed", &node.Parameter3, 0.01f, 0.0f, 2.0f, "%.2f"))
                paramChanged = true;
        } else if (node.Type == EditorNodeType::ActionIdle) {
            ImGui::Text("待機時間");
            ImGui::SameLine();
            if (ImGui::DragFloat("##idleDuration", &node.Parameter, 0.1f, 0.1f, 10.0f, "%.1fs"))
                paramChanged = true;
        }

        // パラメータが変更され、実行中なら再ビルド
        if (paramChanged && m_IsRunning) {
            BuildAndRunTree();
        }

        ImGui::PopItemWidth();

        // 出力ピン(条件ノードは成功/失敗の2つ、重み付けノードは複数、アクションノードはなし、それ以外は1つ)
        ImGui::Spacing();

        if (node.IsConditionNode()) {
            // 条件ノード: Success と Failure の2つの出力
            ed::BeginPin(node.SuccessPinID, ed::PinKind::Output);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "成功 ->");
            ed::EndPin();

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0)); // スペース
            ImGui::SameLine();

            ed::BeginPin(node.FailurePinID, ed::PinKind::Output);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "失敗 ->");
            ed::EndPin();
        } else if (node.IsWeightNode()) {
            // ★修正: 重み付けノードは複数の出力ピンを表示
            float totalWeight = 0.0f;
            for (auto &output : node.WeightedOutputs) {
                totalWeight += output.Weight;
            }

            for (int i = 0; i < node.WeightedOutputs.size(); ++i) {
                float percentage = (totalWeight > 0.0f) ? (node.WeightedOutputs[i].Weight / totalWeight * 100.0f) : 0.0f;
                ed::BeginPin(node.WeightedOutputs[i].PinID, ed::PinKind::Output);
                ImGui::Text("出力%d (%.1f%%) ->", i + 1, percentage);
                ed::EndPin();

                // 改行して次の出力ピンへ
                if (i < node.WeightedOutputs.size() - 1) {
                    ImGui::Spacing();
                }
            }
        } else if (!node.IsActionNode()) {
            // 通常ノード(コンポジットノード): 1つの出力
            ed::BeginPin(node.OutputPinID, ed::PinKind::Output);
            ImGui::Text("出力 ->");
            ed::EndPin();
        }
        // アクションノードは出力ピンなし

        ed::EndNode();

        if (showHighlight) {
            // ★修正: 実行中と失敗のみカラーをポップ
            if (status == NodeStatus::Running || status == NodeStatus::Failure) {
                ed::PopStyleColor(2);
            }
        }

        ImGui::PopID();
    }

    // リンク描画
    for (const auto &link : m_Links) {
        ed::Link(link.ID, link.StartPinID, link.EndPinID);
    }

    HandleCreateAction();
    DeleteSelectedItems();

    // --- 右クリックメニュー (背景) ---
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("ノード作成");
        Vector2 mousePos = Input::GetInstance()->GetMousePos();
        m_CreatePos = ed::ScreenToCanvas(ImVec2(mousePos.x, mousePos.y));
    }
    if (ImGui::BeginPopup("ノード作成")) {
        // ★修正: ツールチップを追加
        if (ImGui::MenuItem("シーケンス")) {
            CreateNode("Sequence", EditorNodeType::Sequence);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::Sequence));
        }

        if (ImGui::MenuItem("セレクター")) {
            CreateNode("Selector", EditorNodeType::Selector);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::Selector));
        }

        if (ImGui::MenuItem("ランダムセレクター")) {
            CreateNode("Random Selector", EditorNodeType::SelectorRandom);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::SelectorRandom));
        }

        if (ImGui::MenuItem("重みデコレーター")) {
            CreateNode("Weight", EditorNodeType::DecoratorWeight);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::DecoratorWeight));
        }

        ImGui::Separator();
        ImGui::Text("条件");
        if (ImGui::MenuItem("  距離チェック")) {
            CreateNode("距離チェック", EditorNodeType::ConditionPlayerClose);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ConditionPlayerClose));
        }

        if (ImGui::MenuItem("  HP低下チェック")) {
            CreateNode("HP低下", EditorNodeType::ConditionHealthLow);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ConditionHealthLow));
        }

        ImGui::Separator();
        ImGui::Text("アクション");
        if (ImGui::MenuItem("  接近")) {
            CreateNode("接近", EditorNodeType::ActionApproach);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ActionApproach));
        }

        if (ImGui::MenuItem("  高速接近")) {
            CreateNode("ダッシュ", EditorNodeType::ActionDash);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ActionDash));
        }

        if (ImGui::MenuItem("  左右移動")) {
            CreateNode("左右移動", EditorNodeType::ActionStrafe);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ActionStrafe));
        }

        if (ImGui::MenuItem("  後退")) {
            CreateNode("後退", EditorNodeType::ActionRetreat);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ActionRetreat));
        }

        if (ImGui::MenuItem("  攻撃")) {
            CreateNode("攻撃", EditorNodeType::ActionAttack);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ActionAttack));
        }

        if (ImGui::MenuItem("  待機")) {
            CreateNode("待機", EditorNodeType::ActionIdle);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ActionIdle));
        }

        if (ImGui::MenuItem("  実行")) {
            CreateNode("Run", EditorNodeType::ActionRun);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", GetNodeDescription(EditorNodeType::ActionRun));
        }

        ImGui::EndPopup();
    }
    ed::Resume();
    ed::End();
    ed::SetCurrentEditor(nullptr);
}

void BehaviorTreeEditor::HandleCreateAction() {
    if (ed::BeginCreate()) {
        ed::PinId start, end;
        if (ed::QueryNewLink(&start, &end)) {
            // 入力ピンと出力ピンの正しい組み合わせかチェック
            bool validLink = false;

            int nodeId, outputIndex;

            // 重み付けノードの出力ピンかチェック
            bool isWeightedStart = IsWeightedOutputPin(start, nodeId, outputIndex);
            bool isWeightedEnd = IsWeightedOutputPin(end, nodeId, outputIndex);

            if ((IsOutputPin(start) || IsSuccessPin(start) || IsFailurePin(start) || isWeightedStart) && IsInputPin(end)) {
                validLink = true;
            } else if (IsInputPin(start) && (IsOutputPin(end) || IsSuccessPin(end) || IsFailurePin(end) || isWeightedEnd)) {
                std::swap(start, end);
                validLink = true;
            }

            if (validLink && ed::AcceptNewItem()) {
                m_Links.emplace_back(m_NextLinkId++, start, end);
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
    if (m_RuntimeRoot) {
        m_RuntimeRoot->SetContext(m_DebugEnemy, m_DebugPlayer);

        // ★重要: 敵にツリーをセット
        if (m_DebugEnemy) {
            m_DebugEnemy->SetBehaviorTree(m_RuntimeRoot);
        }
    }
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
    case EditorNodeType::SelectorRandom:
        runtimeNode = std::make_shared<RandomSelectorNode>();
        break;
    case EditorNodeType::DecoratorWeight:
        // ★修正: 重み付けノードはRandomSelectorNodeとして構築
        runtimeNode = std::make_shared<RandomSelectorNode>();
        break;
    }

    if (!runtimeNode)
        return nullptr;
    m_nodeInstanceMap[editorNodeId] = runtimeNode;

    bool isLeaf = (eNode.Type == EditorNodeType::ActionRun ||
                   eNode.Type == EditorNodeType::ConditionPlayerClose ||
                   eNode.Type == EditorNodeType::ConditionHealthLow ||
                   eNode.Type == EditorNodeType::ActionApproach ||
                   eNode.Type == EditorNodeType::ActionDash ||
                   eNode.Type == EditorNodeType::ActionStrafe ||
                   eNode.Type == EditorNodeType::ActionRetreat ||
                   eNode.Type == EditorNodeType::ActionAttack ||
                   eNode.Type == EditorNodeType::ActionIdle);

    if (!isLeaf) {
        if (eNode.IsWeightNode()) {
            // ★修正: 重み付けノードの場合、各出力ピンから子ノードを取得して重み付きで追加
            auto weightedChildren = FindWeightedChildrenNodeIds(eNode);
            for (auto &[childId, weight] : weightedChildren) {
                auto childNode = BuildNodeRecursive(childId);
                if (childNode) {
                    // WeightDecoratorNodeでラップして追加
                    auto weightDecorator = std::make_shared<WeightDecoratorNode>(weight);
                    weightDecorator->AddChild(childNode);
                    runtimeNode->AddChild(weightDecorator);
                }
            }
        } else {
            int outputPinId = (int)eNode.OutputPinID.Get();
            std::vector<int> childIds = FindChildrenNodeIds(outputPinId);
            for (int childId : childIds) {
                auto childNode = BuildNodeRecursive(childId);
                if (childNode)
                    runtimeNode->AddChild(childNode);
            }
        }
    } else if (eNode.IsConditionNode()) {
        // 条件ノードの場合、成功ピンと失敗ピンからの子ノードを確認
        int successPinId = (int)eNode.SuccessPinID.Get();
        int failurePinId = (int)eNode.FailurePinID.Get();
        std::vector<int> successChildIds = FindChildrenNodeIds(successPinId);
        std::vector<int> failureChildIds = FindChildrenNodeIds(failurePinId);

        // ★修正: 成功時・失敗時のアクションがある場合の処理を改善
        if (!successChildIds.empty() || !failureChildIds.empty()) {
            // Selectorでラップ(どちらか一方が実行される)
            auto selectorWrapper = std::make_shared<SelectorNode>();

            // 成功ルート: Sequence(条件 -> 成功時アクション)
            if (!successChildIds.empty()) {
                auto successSequence = std::make_shared<SequenceNode>();
                // 条件ノード自体のコピーを作成
                std::shared_ptr<BTNode> conditionCopy = nullptr;
                if (eNode.Type == EditorNodeType::ConditionPlayerClose) {
                    conditionCopy = std::make_shared<IsPlayerCloseNode>(eNode.Parameter, eNode.Parameter2);
                } else if (eNode.Type == EditorNodeType::ConditionHealthLow) {
                    conditionCopy = std::make_shared<IsHealthLowNode>(eNode.Parameter);
                }

                if (conditionCopy) {
                    successSequence->AddChild(conditionCopy); // 条件をチェック
                    for (int childId : successChildIds) {
                        auto childNode = BuildNodeRecursive(childId);
                        if (childNode)
                            successSequence->AddChild(childNode);
                    }
                    selectorWrapper->AddChild(successSequence);
                }
            }

            // ★重要な変更: 失敗ルートは条件チェックなしで直接実行
            // これにより、成功ルートが失敗したら即座に失敗ルートのアクションを実行
            if (!failureChildIds.empty()) {
                // 失敗時は条件チェックをスキップして直接アクションを実行
                for (int childId : failureChildIds) {
                    auto childNode = BuildNodeRecursive(childId);
                    if (childNode)
                        selectorWrapper->AddChild(childNode);
                }
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

    for (int i = 0; i < node.WeightedOutputs.size(); ++i) {
        int outputPinId = (int)node.WeightedOutputs[i].PinID.Get();
        float weight = node.WeightedOutputs[i].Weight;

        // このピンから出ているリンクを探す
        for (const auto &link : m_Links) {
            if ((int)link.StartPinID.Get() == outputPinId) {
                int endPin = (int)link.EndPinID.Get();
                int childNodeId = (endPin - 1) / 10;
                result.push_back({childNodeId, weight});
            }
        }
    }

    return result;
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

        // 重み付けノードの出力を保存
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
    int maxNodeId = 0;
    int maxPinId = 0;
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
        node.Parameter3 = param3;

        // 重み付けノードの出力を読み込み
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
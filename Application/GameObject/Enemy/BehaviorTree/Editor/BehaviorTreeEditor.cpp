#define NOMINMAX
#include "BehaviorTreeEditor.h"
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/ActionNodes.h>
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/CompositeNodes.h>
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/ConditionNodes.h>
#include <algorithm>
#include <functional>
#include <Data/DataHandler.h>
#ifdef _DEBUG

BehaviorTreeEditor::BehaviorTreeEditor() {
    ed::Config config;
    config.SettingsFile = "BehaviorTreeEditor.json";
    editorContext_ = ed::CreateEditor(&config);

    // 初期化時にレイアウトが必要
    needsInitialLayout_ = true;
    needsNavigateToContent_ = false;
}

BehaviorTreeEditor::~BehaviorTreeEditor() {
    if (editorContext_) {
        ed::DestroyEditor(editorContext_);
    }
}

void BehaviorTreeEditor::DrawEditor(BehaviorNode *root) {
    currentRoot_ = root;
    if (!root) {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("ビヘイビアツリーエディター");
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                           "エラー: ビヘイビアツリーが初期化されていません");
        ImGui::Text("Enemy::InitializeBehaviorTree()を呼び出してください");
        ImGui::End();
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("ビヘイビアツリーエディター");

    DrawToolbar(std::string(treeNameBuffer_));
    ImGui::Separator();

    float windowWidth = ImGui::GetContentRegionAvail().x;
    float windowHeight = ImGui::GetContentRegionAvail().y;
    float treeViewWidth = std::max(600.0f, windowWidth * 0.7f);
    float propertiesWidth = std::max(300.0f, windowWidth - treeViewWidth - 20.0f);

    ImGui::BeginGroup();

    ed::SetCurrentEditor(editorContext_);
    ed::Begin("BehaviorTreeCanvas", ImVec2(treeViewWidth, windowHeight));

    BuildNodeEditorData(root);

    if (needsNavigateToContent_) {
        ed::NavigateToContent(0.1f);
        needsNavigateToContent_ = false;
    }

    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::EndGroup();

    ImGui::SameLine();

    // 右側: プロパティパネル
    ImGui::BeginGroup();
    ImGui::BeginChild("Properties", ImVec2(0, 0), true);
    ImGui::Text("ノードプロパティ");
    ImGui::Separator();

    if (selectedNode_) {
        DrawNodeProperties(selectedNode_);
    } else {
        ImGui::TextWrapped("ノードを選択してプロパティを編集");
    }

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("実行履歴");
    ImGui::Separator();

    if (executionHistory_.empty()) {
        ImGui::TextWrapped("まだ実行されていません");
    } else {
        ImGui::TextWrapped("最新 → 古い順:");
        ImGui::Spacing();

        for (int i = static_cast<int>(executionHistory_.size()) - 1; i >= 0; i--) {
            BehaviorNode *historyNode = executionHistory_[i];
            bool isCurrent = (historyNode == executingNode_);

            if (isCurrent) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                ImGui::Text("▶ %s (実行中)", historyNode->GetNodeName());
                ImGui::PopStyleColor();
            } else {
                ImGui::TextDisabled("  %s", historyNode->GetNodeName());
            }
        }
    }

    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::End();
}

void BehaviorTreeEditor::BuildNodeEditorData(BehaviorNode *root) {
    if (!root)
        return;

    // 初回のみノード位置を初期化
    if (needsInitialLayout_) {
        InitializeNodePositions(root);
        needsInitialLayout_ = false;
        needsNavigateToContent_ = true;
    }

    nodeToEditorId_.clear();
    editorIdToNode_.clear();
    nextEditorNodeId_ = 1;
    nextEditorLinkId_ = 1;

    std::function<void(BehaviorNode *, BehaviorNode *)> drawNodeRecursive =
        [&](BehaviorNode *node, BehaviorNode *parent) {
            if (!node)
                return;

            int nodeId = GetOrCreateNodeId(node);

            ed::BeginNode(nodeId);

            // 初回レイアウト時のみ位置を設定
            if (nodePositions_.find(node) != nodePositions_.end()) {
                ImVec2 nodePos = nodePositions_[node];
                ed::SetNodePosition(nodeId, nodePos);
                nodePositions_.erase(node);
            }

            ImGui::PushID(nodeId);

            // ノードの色を決定
            ImVec4 headerColor;
            ImVec4 bgColor;
            bool isExecuting = (executingNode_ == node);
            bool isSelected = (selectedNode_ == node);
            bool isInHistory = (std::find(executionHistory_.begin(), executionHistory_.end(), node) != executionHistory_.end());

            if (isExecuting) {
                // 実行中のノードは明るい緑色
                headerColor = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
                bgColor = ImVec4(0.1f, 0.4f, 0.1f, 0.9f);
            } else if (isInHistory) {
                // 履歴にあるノードは黄色
                headerColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                bgColor = ImVec4(0.4f, 0.35f, 0.1f, 0.9f);
            } else if (isSelected) {
                // 選択中のノードは赤色
                headerColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                bgColor = ImVec4(0.3f, 0.1f, 0.1f, 0.9f);
            } else {
                // デフォルトは青色
                headerColor = ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
                bgColor = ImVec4(0.15f, 0.25f, 0.35f, 0.9f);
            }

            // ノードの背景色を設定
            ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
            ImGui::PushStyleColor(ImGuiCol_Header, headerColor);

            // ヘッダー部分
            ImGui::BeginGroup();
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));

            // ノード名を表示
            ImGui::Spacing();
            ImGui::Indent(8.0f);
            ImGui::Text("%s", node->GetNodeName());

            // 実行状態を表示
            if (isExecuting) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), " [Running]");
            }

            ImGui::Unindent(8.0f);
            ImGui::Spacing();

            ImGui::PopStyleVar();
            ImGui::EndGroup();

            ImGui::Separator();
            ImGui::Spacing();

            // 入力ピン
            ed::BeginPin(nodeId * 1000, ed::PinKind::Input);
            ImGui::Indent(8.0f);
            ImGui::Text("In");
            ImGui::Unindent(8.0f);
            ed::EndPin();

            ImGui::Spacing();

            // 出力ピン
            ed::BeginPin(nodeId * 1000 + 1, ed::PinKind::Output);
            ImGui::Indent(8.0f);
            ImGui::Text("Out");
            ImGui::Unindent(8.0f);
            ed::EndPin();

            ImGui::Spacing();

            ImGui::PopStyleColor(2);
            ImGui::PopID();
            ed::EndNode();

            // クリック検出
            if (ed::GetSelectedObjectCount() > 0) {
                std::vector<ed::NodeId> selectedNodes;
                selectedNodes.resize(ed::GetSelectedObjectCount());
                ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));

                if (!selectedNodes.empty() && selectedNodes[0].Get() == nodeId) {
                    selectedNode_ = node;
                }
            }

            // 親ノードとの接続を描画
            if (parent) {
                int parentId = GetOrCreateNodeId(parent);
                int linkId = nextEditorLinkId_++;

                // 実行中のノードへの接続は緑色にする
                if (isExecuting || isInHistory) {
                    ed::PushStyleColor(ed::StyleColor_Flow, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                    ed::PushStyleVar(ed::StyleVar_FlowSpeed, 100.0f);
                    ed::Link(linkId, parentId * 1000 + 1, nodeId * 1000, ImVec4(0.2f, 1.0f, 0.2f, 1.0f), 2.0f);
                    ed::PopStyleVar();
                    ed::PopStyleColor();
                } else {
                    ed::Link(linkId, parentId * 1000 + 1, nodeId * 1000);
                }
            }

            // 子ノードを再帰的に描画
            if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
                for (auto &child : sequence->GetChildren()) {
                    drawNodeRecursive(child.get(), node);
                }
            } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
                for (auto &child : selector->GetChildren()) {
                    drawNodeRecursive(child.get(), node);
                }
            } else if (auto *weightedSelector = dynamic_cast<WeightedSelectorNode *>(node)) {
                for (auto &child : weightedSelector->GetChildren()) {
                    drawNodeRecursive(child.get(), node);
                }
            }
        };

    drawNodeRecursive(root, nullptr);
}

void BehaviorTreeEditor::InitializeNodePositions(BehaviorNode *root) {
    if (!root)
        return;

    nodePositions_.clear();

    // ツリー全体のサイズを事前計算
    std::unordered_map<BehaviorNode *, int> subtreeWidths;
    std::function<int(BehaviorNode *)> calcWidth = [&](BehaviorNode *node) -> int {
        if (!node)
            return 0;

        std::vector<std::unique_ptr<BehaviorNode>> *children = nullptr;
        if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
            children = &sequence->GetChildren();
        } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
            children = &selector->GetChildren();
        } else if (auto *weightedSelector = dynamic_cast<WeightedSelectorNode *>(node)) {
            children = &weightedSelector->GetChildren();
        }

        if (!children || children->empty()) {
            subtreeWidths[node] = 1;
            return 1;
        }

        int totalWidth = 0;
        for (auto &child : *children) {
            totalWidth += calcWidth(child.get());
        }
        subtreeWidths[node] = totalWidth;
        return totalWidth;
    };

    int totalTreeWidth = calcWidth(root);

    // ノード間のスペーシング(調整)
    const float nodeWidth = 180.0f;
    const float nodeHeight = 100.0f;
    const float horizontalSpacing = 80.0f;
    const float verticalSpacing = 120.0f;

    std::function<void(BehaviorNode *, int, float)> layoutNodes =
        [&](BehaviorNode *node, int depth, float centerX) {
            if (!node)
                return;

            // 現在のノードの位置
            float y = 50.0f + depth * (nodeHeight + verticalSpacing);
            nodePositions_[node] = ImVec2(centerX, y);

            // 子ノードの処理
            std::vector<std::unique_ptr<BehaviorNode>> *children = nullptr;
            if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
                children = &sequence->GetChildren();
            } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
                children = &selector->GetChildren();
            } else if (auto *weightedSelector = dynamic_cast<WeightedSelectorNode *>(node)) {
                children = &weightedSelector->GetChildren();
            }

            if (children && !children->empty()) {
                // 子ノード全体の幅を計算
                float totalChildWidth = 0.0f;
                for (auto &child : *children) {
                    totalChildWidth += subtreeWidths[child.get()] * (nodeWidth + horizontalSpacing);
                }
                totalChildWidth -= horizontalSpacing;

                // 子ノードの開始位置(中央揃え)
                float startX = centerX - totalChildWidth * 0.5f;
                float currentX = startX;

                for (auto &child : *children) {
                    int childWidth = subtreeWidths[child.get()];
                    float childTreeWidth = childWidth * (nodeWidth + horizontalSpacing) - horizontalSpacing;
                    float childCenterX = currentX + childTreeWidth * 0.5f;

                    layoutNodes(child.get(), depth + 1, childCenterX);

                    currentX += childTreeWidth + horizontalSpacing;
                }
            }
        };

    // ルートを画面中央付近に配置
    float rootX = 100.0f + totalTreeWidth * (nodeWidth + horizontalSpacing) * 0.5f;
    layoutNodes(root, 0, rootX);
}

int BehaviorTreeEditor::GetOrCreateNodeId(BehaviorNode *node) {
    auto it = nodeToEditorId_.find(node);
    if (it != nodeToEditorId_.end()) {
        return it->second;
    }

    int newId = nextEditorNodeId_++;
    nodeToEditorId_[node] = newId;
    editorIdToNode_[newId] = node;
    return newId;
}

void BehaviorTreeEditor::DrawToolbar(const std::string &treeName) {
    ImGui::Text("ツリー名:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputText("##TreeName", treeNameBuffer_, sizeof(treeNameBuffer_));

    ImGui::SameLine();
    if (ImGui::Button("保存")) {
        ImGui::OpenPopup("SaveConfirm");
    }

    ImGui::SameLine();
    if (ImGui::Button("読み込み")) {
        ImGui::OpenPopup("LoadConfirm");
    }

    // 保存確認ポップアップ
    if (ImGui::BeginPopupModal("SaveConfirm", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("設定を保存しますか?");
        ImGui::Text("ツリー名: %s", treeNameBuffer_);
        ImGui::Separator();

        if (ImGui::Button("保存", ImVec2(120, 0))) {
            if (currentRoot_) {
                SaveSettings(std::string(treeNameBuffer_), currentRoot_);
                ImGui::Text("保存しました!");
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 読み込み確認ポップアップ
    if (ImGui::BeginPopupModal("LoadConfirm", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("設定を読み込みますか?");
        ImGui::Text("現在の編集内容は保存されていない可能性があります。");
        ImGui::Separator();

        if (ImGui::Button("読み込み", ImVec2(120, 0))) {
            if (currentRoot_) {
                LoadSettings(std::string(treeNameBuffer_), currentRoot_);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("ビューをフィット")) {
        needsNavigateToContent_ = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("レイアウト再計算")) {
        needsInitialLayout_ = true;
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
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x);

    ImGui::Text("ノードタイプ:");
    ImGui::TextWrapped("%s", node->GetNodeName());
    ImGui::Separator();

    // ノードタイプごとの設定
    if (auto *approachNode = dynamic_cast<ApproachNode *>(node)) {
        float speed = approachNode->GetSpeed();
        if (ImGui::DragFloat("移動速度##approach", &speed, 0.5f, 0.0f, 50.0f)) {
            approachNode->SetSpeed(speed);
        }

        int speedType = static_cast<int>(approachNode->GetSpeedType());
        if (ImGui::Combo("速度タイプ", &speedType, "Fast\0Slow\0")) {
            approachNode->SetSpeedType(static_cast<MoveSpeedType>(speedType));
        }
    } else if (auto *closeNode = dynamic_cast<CloseApproachNode *>(node)) {
        float speed = closeNode->GetSpeed();
        float targetDist = closeNode->GetTargetDistance();
        if (ImGui::DragFloat("接近速度##close", &speed, 0.5f, 0.0f, 30.0f)) {
            closeNode->SetSpeed(speed);
        }
        if (ImGui::DragFloat("目標距離##close", &targetDist, 0.1f, 0.0f, 10.0f)) {
            closeNode->SetTargetDistance(targetDist);
        }
    } else if (auto *strafeNode = dynamic_cast<StrafeNode *>(node)) {
        float speed = strafeNode->GetSpeed();
        if (ImGui::DragFloat("横移動速度##strafe", &speed, 0.5f, 0.0f, 30.0f)) {
            strafeNode->SetSpeed(speed);
        }
    } else if (auto *retreatNode = dynamic_cast<RetreatNode *>(node)) {
        float speed = retreatNode->GetSpeed();
        float retreatDist = retreatNode->GetRetreatDistance();
        if (ImGui::DragFloat("後退速度##retreat", &speed, 0.5f, 0.0f, 30.0f)) {
            retreatNode->SetSpeed(speed);
        }
        if (ImGui::DragFloat("後退距離##retreat", &retreatDist, 0.1f, 0.0f, 20.0f)) {
            retreatNode->SetRetreatDistance(retreatDist);
        }
    } else if (auto *distNode = dynamic_cast<DistanceCheckNode *>(node)) {
        float minDist = distNode->GetMinDistance();
        float maxDist = distNode->GetMaxDistance();
        if (ImGui::DragFloat("最小距離##dist", &minDist, 0.1f, 0.0f, 100.0f)) {
            distNode->SetDistances(minDist, maxDist);
        }
        if (ImGui::DragFloat("最大距離##dist", &maxDist, 0.1f, 0.0f, 100.0f)) {
            distNode->SetDistances(minDist, maxDist);
        }
    }

    // 重み付きセレクターの重み設定
    if (auto *weightedSelector = dynamic_cast<WeightedSelectorNode *>(node)) {
        ImGui::Separator();
        ImGui::Text("子ノードの重み設定:");
        auto &children = weightedSelector->GetChildren();
        auto &weights = weightedSelector->GetWeights();

        for (size_t i = 0; i < children.size() && i < weights.size(); i++) {
            std::string label = "重み##" + std::to_string(i);
            ImGui::DragFloat(label.c_str(), &weights[i], 0.1f, 0.0f, 10.0f);
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", children[i]->GetNodeName());
        }
    }

    ImGui::PopTextWrapPos();
}

void BehaviorTreeEditor::SaveSettings(const std::string &treeName, BehaviorNode *root) {
    if (!root) {
        return;
    }

    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("BehaviorTree", treeName);

    // 全ノードを収集し、IDを割り当て
    std::vector<BehaviorNode *> allNodes;
    int currentId = 0;

    std::function<void(BehaviorNode *)> collectNodes = [&](BehaviorNode *node) {
        if (!node)
            return;

        // ノードIDを割り当て
        node->nodeId = currentId++;
        allNodes.push_back(node);

        if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
            for (auto &child : sequence->GetChildren()) {
                collectNodes(child.get());
            }
        } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
            for (auto &child : selector->GetChildren()) {
                collectNodes(child.get());
            }
        } else if (auto *weightedSelector = dynamic_cast<WeightedSelectorNode *>(node)) {
            for (auto &child : weightedSelector->GetChildren()) {
                collectNodes(child.get());
            }
        }
    };

    // rootから全ノードを収集
    collectNodes(root);

    // ノード情報を保存
    data->Save("nodeCount", static_cast<int>(allNodes.size()));

    for (size_t i = 0; i < allNodes.size(); i++) {
        BehaviorNode *node = allNodes[i];
        std::string prefix = "node_" + std::to_string(i) + "_";

        data->Save(prefix + "nodeId", node->nodeId);
        data->Save(prefix + "nodeName", std::string(node->GetNodeName()));

        // ノード固有のパラメータを保存
        if (auto *approachNode = dynamic_cast<ApproachNode *>(node)) {
            data->Save(prefix + "type", std::string("Approach"));
            data->Save(prefix + "speed", approachNode->GetSpeed());
            data->Save(prefix + "speedType", static_cast<int>(approachNode->GetSpeedType()));
        } else if (auto *closeNode = dynamic_cast<CloseApproachNode *>(node)) {
            data->Save(prefix + "type", std::string("CloseApproach"));
            data->Save(prefix + "speed", closeNode->GetSpeed());
            data->Save(prefix + "targetDistance", closeNode->GetTargetDistance());
        } else if (auto *strafeNode = dynamic_cast<StrafeNode *>(node)) {
            data->Save(prefix + "type", std::string("Strafe"));
            data->Save(prefix + "speed", strafeNode->GetSpeed());
        } else if (auto *retreatNode = dynamic_cast<RetreatNode *>(node)) {
            data->Save(prefix + "type", std::string("Retreat"));
            data->Save(prefix + "speed", retreatNode->GetSpeed());
            data->Save(prefix + "retreatDistance", retreatNode->GetRetreatDistance());
        } else if (auto *distNode = dynamic_cast<DistanceCheckNode *>(node)) {
            data->Save(prefix + "type", std::string("DistanceCheck"));
            data->Save(prefix + "minDistance", distNode->GetMinDistance());
            data->Save(prefix + "maxDistance", distNode->GetMaxDistance());
        } else if (auto *weightedSelector = dynamic_cast<WeightedSelectorNode *>(node)) {
            data->Save(prefix + "type", std::string("WeightedSelector"));
            auto &weights = weightedSelector->GetWeights();
            data->Save(prefix + "childCount", static_cast<int>(weights.size()));
            for (size_t j = 0; j < weights.size(); j++) {
                data->Save(prefix + "weight_" + std::to_string(j), weights[j]);
            }
        } else if (dynamic_cast<SequenceNode *>(node)) {
            data->Save(prefix + "type", std::string("Sequence"));
        } else if (dynamic_cast<SelectorNode *>(node)) {
            data->Save(prefix + "type", std::string("Selector"));
        } else if (dynamic_cast<StopNode *>(node)) {
            data->Save(prefix + "type", std::string("Stop"));
        }
    }
}

void BehaviorTreeEditor::LoadSettings(const std::string &treeName, BehaviorNode *root) {
    if (!root)
        return;

    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("BehaviorTree", treeName);

    // ノード数を読み込み
    int nodeCount = data->Load("nodeCount", 0);

    if (nodeCount <= 0) {
        return;
    }

    // ノードIDからノードへのマップを作成
    std::unordered_map<int, BehaviorNode *> nodeMap;
    std::vector<BehaviorNode *> allNodes;
    int currentId = 0;

    std::function<void(BehaviorNode *)> buildNodeMap = [&](BehaviorNode *node) {
        if (!node)
            return;

        // ノードIDを割り当て
        node->nodeId = currentId++;
        nodeMap[node->nodeId] = node;
        allNodes.push_back(node);

        if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
            for (auto &child : sequence->GetChildren()) {
                buildNodeMap(child.get());
            }
        } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
            for (auto &child : selector->GetChildren()) {
                buildNodeMap(child.get());
            }
        } else if (auto *weightedSelector = dynamic_cast<WeightedSelectorNode *>(node)) {
            for (auto &child : weightedSelector->GetChildren()) {
                buildNodeMap(child.get());
            }
        }
    };

    buildNodeMap(root);

    // 保存されたノード数とツリーのノード数が一致するか確認
    if (nodeCount != static_cast<int>(allNodes.size())) {
        // 警告を出す
        return;
    }

    // 保存されたノード情報を読み込んで適用
    for (int i = 0; i < nodeCount; i++) {
        std::string prefix = "node_" + std::to_string(i) + "_";

        int savedNodeId = data->Load(prefix + "nodeId", -1);
        std::string nodeName = data->Load(prefix + "nodeName", std::string(""));
        std::string nodeType = data->Load(prefix + "type", std::string(""));

        // インデックスiのノードに対応するデータを読み込む
        if (i < static_cast<int>(allNodes.size())) {
            BehaviorNode *node = allNodes[i];

            // ノード固有のパラメータを読み込み
            if (auto *approachNode = dynamic_cast<ApproachNode *>(node)) {
                float speed = data->Load(prefix + "speed", 15.0f);
                int speedType = data->Load(prefix + "speedType", 0);
                approachNode->SetSpeed(speed);
                approachNode->SetSpeedType(static_cast<MoveSpeedType>(speedType));
            } else if (auto *closeNode = dynamic_cast<CloseApproachNode *>(node)) {
                float speed = data->Load(prefix + "speed", 5.0f);
                float targetDist = data->Load(prefix + "targetDistance", 2.0f);
                closeNode->SetSpeed(speed);
                closeNode->SetTargetDistance(targetDist);
            } else if (auto *strafeNode = dynamic_cast<StrafeNode *>(node)) {
                float speed = data->Load(prefix + "speed", 8.0f);
                strafeNode->SetSpeed(speed);
            } else if (auto *retreatNode = dynamic_cast<RetreatNode *>(node)) {
                float speed = data->Load(prefix + "speed", 7.0f);
                float retreatDist = data->Load(prefix + "retreatDistance", 5.0f);
                retreatNode->SetSpeed(speed);
                retreatNode->SetRetreatDistance(retreatDist);
            } else if (auto *distNode = dynamic_cast<DistanceCheckNode *>(node)) {
                float minDist = data->Load(prefix + "minDistance", 0.0f);
                float maxDist = data->Load(prefix + "maxDistance", 100.0f);
                distNode->SetDistances(minDist, maxDist);
            } else if (auto *weightedSelector = dynamic_cast<WeightedSelectorNode *>(node)) {
                int childCount = data->Load(prefix + "childCount", 0);
                auto &weights = weightedSelector->GetWeights();
                for (int j = 0; j < childCount && j < static_cast<int>(weights.size()); j++) {
                    weights[j] = data->Load(prefix + "weight_" + std::to_string(j), 1.0f);
                }
            }
        }
    }
}

void BehaviorTreeEditor::AddExecutionHistory(BehaviorNode *node) {
    if (!node)
        return;

    // 既に履歴にある場合は削除してから追加（最新に移動）
    auto it = std::find(executionHistory_.begin(), executionHistory_.end(), node);
    if (it != executionHistory_.end()) {
        executionHistory_.erase(it);
    }

    // 履歴に追加
    executionHistory_.push_back(node);

    // 最大数を超えたら古いものを削除
    if (executionHistory_.size() > MAX_HISTORY) {
        executionHistory_.erase(executionHistory_.begin());
    }
}

#endif // _DEBUG
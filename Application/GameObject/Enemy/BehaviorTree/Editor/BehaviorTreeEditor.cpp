#define NOMINMAX
#include "BehaviorTreeEditor.h"
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/ActionNodes.h>
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/ConditionNodes.h>
#include <Application/GameObject/Enemy/BehaviorTree/Nodes/SequenceNode.h>
#include <algorithm>
#ifdef _DEBUG

BehaviorTreeEditor::BehaviorTreeEditor() {
    ed::Config config;
    config.SettingsFile = "BehaviorTreeEditor.json";
    editorContext_ = ed::CreateEditor(&config);
}

BehaviorTreeEditor::~BehaviorTreeEditor() {
    if (editorContext_) {
        ed::DestroyEditor(editorContext_);
    }
}

void BehaviorTreeEditor::DrawEditor(BehaviorNode *root) {
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
    float treeViewWidth = std::max(600.0f, windowWidth * 0.7f);
    float propertiesWidth = std::max(300.0f, windowWidth - treeViewWidth - 20.0f);

    // ノードエディタ領域
    ImGui::BeginChild("NodeEditor", ImVec2(treeViewWidth, -1), true);

    ed::SetCurrentEditor(editorContext_);
    ed::Begin("BehaviorTreeCanvas");

    BuildNodeEditorData(root);

    ed::End();
    ed::SetCurrentEditor(nullptr);

    ImGui::EndChild();

    ImGui::SameLine();

    // プロパティパネル（既存のコード）
    ImGui::BeginChild("Properties", ImVec2(propertiesWidth, -1), true);
    ImGui::Text("ノードプロパティ");
    ImGui::Separator();

    if (selectedNode_) {
        DrawNodeProperties(selectedNode_);
    } else {
        ImGui::TextWrapped("ノードを選択してプロパティを編集");
    }

    ImGui::Separator();
    ImGui::Spacing();

    // 実行履歴の表示（既存のコード）
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
    ImGui::End();
}

void BehaviorTreeEditor::BuildNodeEditorData(BehaviorNode *root) {
    if (!root)
        return;

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

            ImGui::PushID(nodeId);

            // ノードの色を決定
            ImVec4 color;
            bool isExecuting = (executingNode_ == node);
            bool isSelected = (selectedNode_ == node);
            bool isInHistory = (std::find(executionHistory_.begin(), executionHistory_.end(), node) != executionHistory_.end());

            if (isExecuting) {
                color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
            } else if (isSelected) {
                color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
            } else if (isInHistory) {
                color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
            } else {
                color = ImVec4(0.2f, 0.6f, 0.8f, 1.0f);
            }

            ImGui::PushStyleColor(ImGuiCol_Header, color);
            ImGui::Text("%s", node->GetNodeName());
            ImGui::PopStyleColor();

            // 入力ピン
            ed::BeginPin(nodeId * 1000, ed::PinKind::Input);
            ImGui::Text("→ In");
            ed::EndPin();

            ImGui::SameLine();

            // 出力ピン
            ed::BeginPin(nodeId * 1000 + 1, ed::PinKind::Output);
            ImGui::Text("Out →");
            ed::EndPin();

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
                ed::Link(linkId, parentId * 1000 + 1, nodeId * 1000);
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
            }
        };

    drawNodeRecursive(root, nullptr);
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
        SaveSettings(std::string(treeNameBuffer_));
    }

    ImGui::SameLine();
    if (ImGui::Button("読み込み")) {
        // 読み込みはルートノードが必要なので、外部で呼ぶ必要があります
        ImGui::OpenPopup("LoadConfirm");
    }

    // 確認ポップアップ
    if (ImGui::BeginPopupModal("LoadConfirm", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("設定を読み込みますか？");
        ImGui::Text("現在の編集内容は保存されていない可能性があります。");
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            // ロード処理は外部から呼ぶ
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("ズームリセット")) {
        zoom_ = 1.0f;
        panOffset_ = ImVec2(0.0f, 0.0f);
    }
}

void BehaviorTreeEditor::DrawNode(BehaviorNode *node, ImVec2 pos, int depth, ImVec2 canvasOrigin) {
    if (!node)
        return;
    node->nodeId = nodeIdCounter_++;

    // ズームとパンを適用した最終座標を計算
    ImVec2 finalPos = ImVec2(
        canvasOrigin.x + (pos.x + panOffset_.x) * zoom_,
        canvasOrigin.y + (pos.y + panOffset_.y) * zoom_);
    float zoomedWidth = NODE_WIDTH * zoom_;
    float zoomedHeight = NODE_HEIGHT * zoom_;

    // ノードのインタラクティブな描画
    bool isSelected = (selectedNode_ == node);
    bool isExecuting = (executingNode_ == node);
    bool isInHistory = (std::find(executionHistory_.begin(), executionHistory_.end(), node) != executionHistory_.end());

    ImVec4 color;
    if (isExecuting) {
        color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    } else if (isSelected) {
        color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    } else if (isInHistory) {
        color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
    } else {
        color = ImVec4(0.2f, 0.6f, 0.8f, 1.0f);
    }

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    // ノードの矩形を描画
    drawList->AddRectFilled(finalPos,
                            ImVec2(finalPos.x + zoomedWidth, finalPos.y + zoomedHeight),
                            ImGui::ColorConvertFloat4ToU32(color), 4.0f * zoom_);
    drawList->AddRect(finalPos,
                      ImVec2(finalPos.x + zoomedWidth, finalPos.y + zoomedHeight),
                      IM_COL32(255, 255, 255, 255), 4.0f * zoom_, 0, 2.0f * zoom_);

    // テキストを描画（ズームに合わせてサイズ変更）
    const char *nodeName = node->GetNodeName();

    // ズームが小さすぎる場合はテキストを省略
    if (zoom_ >= 0.4f) {
        ImFont *font = ImGui::GetFont();
        float baseFontSize = ImGui::GetFontSize(); // 修正: FontSize → GetFontSize()
        float fontSize = baseFontSize * zoom_;

        // テキストサイズを計算（ズーム適用後）
        ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, nodeName);
        ImVec2 textPos = ImVec2(
            finalPos.x + (zoomedWidth - textSize.x) * 0.5f,
            finalPos.y + (zoomedHeight - textSize.y) * 0.5f);

        drawList->AddText(font, fontSize, textPos, IM_COL32(255, 255, 255, 255), nodeName);
    }

    // クリック判定
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isHovered = (mousePos.x >= finalPos.x && mousePos.x <= finalPos.x + zoomedWidth &&
                      mousePos.y >= finalPos.y && mousePos.y <= finalPos.y + zoomedHeight);

    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        selectedNode_ = node;
    }

    // 子ノードの描画
    if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
        DrawChildren(sequence->GetChildren(), pos, depth, canvasOrigin);
    } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
        DrawChildren(selector->GetChildren(), pos, depth, canvasOrigin);
    }
}

void BehaviorTreeEditor::DrawChildren(std::vector<std::unique_ptr<BehaviorNode>> &children,
                                      ImVec2 parentPos, int depth, ImVec2 canvasOrigin) {
    if (children.empty())
        return;

    ImVec2 parentCenter(parentPos.x + NODE_WIDTH * 0.5f, parentPos.y + NODE_HEIGHT);

    float totalWidth = 0.0f;
    for (auto &child : children) {
        int childTreeWidth = CalculateTreeWidth(child.get());
        totalWidth += childTreeWidth * (NODE_WIDTH + HORIZONTAL_SPACING);
    }

    if (!children.empty()) {
        totalWidth -= HORIZONTAL_SPACING;
    }

    float startX = parentCenter.x - totalWidth * 0.5f;
    float currentX = startX;
    float childY = parentPos.y + VERTICAL_SPACING;

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    for (size_t i = 0; i < children.size(); ++i) {
        int childTreeWidth = CalculateTreeWidth(children[i].get());
        float childWidth = childTreeWidth * (NODE_WIDTH + HORIZONTAL_SPACING) - HORIZONTAL_SPACING;

        float childCenterX = currentX + childWidth * 0.5f;
        ImVec2 childPos(childCenterX - NODE_WIDTH * 0.5f, childY);

        // ラインを描画（ズーム適用）
        ImVec2 p1(
            canvasOrigin.x + (parentCenter.x + panOffset_.x) * zoom_,
            canvasOrigin.y + (parentCenter.y + panOffset_.y) * zoom_);
        ImVec2 p2(
            canvasOrigin.x + (childCenterX + panOffset_.x) * zoom_,
            canvasOrigin.y + (childY + panOffset_.y) * zoom_);

        drawList->AddLine(p1, p2, IM_COL32(255, 255, 255, 200), 2.0f * zoom_);

        DrawNode(children[i].get(), childPos, depth + 1, canvasOrigin);

        currentX += childWidth + HORIZONTAL_SPACING;
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

    // 重み付け（優先度）設定
    if (nodeWeights_.find(node) == nodeWeights_.end()) {
        nodeWeights_[node] = 1.0f;
    }

    float weight = nodeWeights_[node];
    ImGui::DragFloat("優先度ウェイト", &weight, 0.1f, 0.0f, 10.0f);
    nodeWeights_[node] = weight;

    ImGui::Separator();
    ImGui::Text("ノード固有の設定:");
    ImGui::Spacing();

    // ノードタイプごとの設定
    if (auto *moveNode = dynamic_cast<MoveToTargetNode *>(node)) {
        float speed = moveNode->GetSpeed();
        if (ImGui::DragFloat("移動速度##move", &speed, 0.5f, 0.0f, 500.0f)) {
            moveNode->SetSpeed(speed);
        }
    } else if (auto *flyNode = dynamic_cast<FlyToTargetNode *>(node)) {
        float speed = flyNode->GetSpeed();
        if (ImGui::DragFloat("飛行速度##fly", &speed, 0.5f, 0.0f, 500.0f)) {
            flyNode->SetSpeed(speed);
        }
    } else if (auto *rushNode = dynamic_cast<RushAttackNode *>(node)) {
        float speed = rushNode->GetRushSpeed();
        float minDist = rushNode->GetMinDistance();
        if (ImGui::DragFloat("突進速度##rush", &speed, 0.5f, 0.0f, 500.0f)) {
            rushNode->SetRushSpeed(speed);
        }
        if (ImGui::DragFloat("最小距離##rush", &minDist, 0.1f, 0.0f, 50.0f)) {
            rushNode->SetMinDistance(minDist);
        }
    } else if (auto *distNode = dynamic_cast<DistanceToTargetNode *>(node)) {
        float minDist = distNode->GetMinDistance();
        float maxDist = distNode->GetMaxDistance();
        if (ImGui::DragFloat("最小距離##dist", &minDist, 0.1f, 0.0f, 100.0f)) {
            distNode->SetDistances(minDist, maxDist);
        }
        if (ImGui::DragFloat("最大距離##dist", &maxDist, 0.1f, 0.0f, 100.0f)) {
            distNode->SetDistances(minDist, maxDist);
        }
    }

    ImGui::PopTextWrapPos();
}

void BehaviorTreeEditor::SaveSettings(const std::string &treeName) {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("BehaviorTree", treeName);

    // エディター設定の保存
    data->Save("zoom", zoom_);
    data->Save<Vector2>("panOffset", Vector2(panOffset_.x, panOffset_.y));

    // ノードの重み付け情報を保存
    data->Save("nodeWeightCount", static_cast<int>(nodeWeights_.size()));

    int index = 0;
    for (const auto &[node, weight] : nodeWeights_) {
        std::string prefix = "weight_" + std::to_string(index) + "_";
        data->Save(prefix + "nodeId", node->nodeId);
        data->Save(prefix + "nodeName", std::string(node->GetNodeName()));
        data->Save(prefix + "weight", weight);

        // ノード固有のパラメータを保存
        if (auto *moveNode = dynamic_cast<MoveToTargetNode *>(node)) {
            data->Save(prefix + "speed", moveNode->GetSpeed());
        } else if (auto *flyNode = dynamic_cast<FlyToTargetNode *>(node)) {
            data->Save(prefix + "speed", flyNode->GetSpeed());
        } else if (auto *rushNode = dynamic_cast<RushAttackNode *>(node)) {
            data->Save(prefix + "rushSpeed", rushNode->GetRushSpeed());
            data->Save(prefix + "minDistance", rushNode->GetMinDistance());
        } else if (auto *distNode = dynamic_cast<DistanceToTargetNode *>(node)) {
            data->Save(prefix + "minDistance", distNode->GetMinDistance());
            data->Save(prefix + "maxDistance", distNode->GetMaxDistance());
        }

        index++;
    }
}

void BehaviorTreeEditor::LoadSettings(const std::string &treeName, BehaviorNode *root) {
    if (!root)
        return;

    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("BehaviorTree", treeName);

    // エディター設定の読み込み（デフォルト値を指定）
    zoom_ = data->Load("zoom", 1.0f);
    Vector2 panOffset = data->Load<Vector2>("panOffset", Vector2(0.0f, 0.0f));
    panOffset_ = ImVec2(panOffset.x, panOffset.y);

    // ノードの重み付け情報を読み込み
    int weightCount = data->Load("nodeWeightCount", 0);

    // weightCountが0の場合は読み込みをスキップ（初回起動時など）
    if (weightCount <= 0) {
        return;
    }

    // ノードIDからノードへのマップを作成（再帰的に構築）
    std::unordered_map<int, BehaviorNode *> nodeMap;
    std::function<void(BehaviorNode *)> buildNodeMap = [&](BehaviorNode *node) {
        if (!node)
            return;
        nodeMap[node->nodeId] = node;

        if (auto *sequence = dynamic_cast<SequenceNode *>(node)) {
            for (auto &child : sequence->GetChildren()) {
                buildNodeMap(child.get());
            }
        } else if (auto *selector = dynamic_cast<SelectorNode *>(node)) {
            for (auto &child : selector->GetChildren()) {
                buildNodeMap(child.get());
            }
        }
    };

    // まずツリーを一度描画してnodeIdを設定
    nodeIdCounter_ = 0;
    buildNodeMap(root);

    nodeWeights_.clear();

    for (int i = 0; i < weightCount; i++) {
        std::string prefix = "weight_" + std::to_string(i) + "_";

        // 各値を安全に読み込み
        int nodeId = data->Load(prefix + "nodeId", -1);
        std::string nodeName = data->Load(prefix + "nodeName", std::string(""));
        float weight = data->Load(prefix + "weight", 1.0f);

        // ノードIDからノードを検索
        if (nodeMap.find(nodeId) != nodeMap.end()) {
            BehaviorNode *node = nodeMap[nodeId];
            nodeWeights_[node] = weight;

            // ノード固有のパラメータを読み込み
            if (auto *moveNode = dynamic_cast<MoveToTargetNode *>(node)) {
                float speed = data->Load(prefix + "speed", 10.0f);
                moveNode->SetSpeed(speed);
            } else if (auto *flyNode = dynamic_cast<FlyToTargetNode *>(node)) {
                float speed = data->Load(prefix + "speed", 10.0f);
                flyNode->SetSpeed(speed);
            } else if (auto *rushNode = dynamic_cast<RushAttackNode *>(node)) {
                float rushSpeed = data->Load(prefix + "rushSpeed", 20.0f);
                float minDist = data->Load(prefix + "minDistance", 5.0f);
                rushNode->SetRushSpeed(rushSpeed);
                rushNode->SetMinDistance(minDist);
            } else if (auto *distNode = dynamic_cast<DistanceToTargetNode *>(node)) {
                float minDist = data->Load(prefix + "minDistance", 0.0f);
                float maxDist = data->Load(prefix + "maxDistance", 100.0f);
                distNode->SetDistances(minDist, maxDist);
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
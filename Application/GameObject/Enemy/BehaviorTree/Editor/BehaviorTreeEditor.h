#pragma once
#include "Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"
#ifdef _DEBUG
#include "imgui.h"
#include "imgui_node_editor.h"
#endif // DEBUG
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _DEBUG
namespace ed = ax::NodeEditor;
#endif

/// <summary>
/// デバッグ用のビヘイビアツリーエディター（_DEBUG ビルド時有効)
/// </summary>
class BehaviorTreeEditor {
#ifdef _DEBUG
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    BehaviorTreeEditor();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~BehaviorTreeEditor();

    /// <summary>
    /// エディターの描画
    /// </summary>
    /// <param name="root"> デバッグしたいツリーノードの登録 </param>
    void DrawEditor(BehaviorNode *root);

    /// <summary>
    /// 各ノードの設定を保存
    /// </summary>
    /// <param name="treeName"> ツリーノードの名前 </param>
    /// <param name="root"> ルートノード </param>
    void SaveSettings(const std::string &treeName, BehaviorNode *root);

    /// <summary>
    /// 各ノードの設定を読み込み
    /// </summary>
    /// <param name="treeName"> ツリーノードの名前 </param>
    void LoadSettings(const std::string &treeName, BehaviorNode *root);

    /// <summary>
    /// 実行中のノードのセット
    /// </summary>
    /// <param name="node">設定する実行中のノードを指す BehaviorNode のポインタ</param>
    void SetExecutingNode(BehaviorNode *node) { executingNode_ = node; }

    /// <summary>
    /// 実行中のノードのクリア
    /// </summary>
    void ClearExecutingNode() { executingNode_ = nullptr; }

    /// <summary>
    /// ノードの実行履歴を追加
    /// </summary>
    /// <param name="node">追加する BehaviorNode へのポインタ</param>
    void AddExecutionHistory(BehaviorNode *node);

    /// <summary>
    /// ノード実行履歴をクリア
    /// </summary>
    void ClearExecutionHistory() { executionHistory_.clear(); }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// ツリー全体のノード位置を初期化
    /// </summary>
    /// <param name="root">ルートノード</param>
    void InitializeNodePositions(BehaviorNode *root);

    /// <summary>
    /// imgui-node-editor用のノードとリンクを構築して描画
    /// </summary>
    /// <param name="root">描画するツリーのルートノードへのポインタ</param>
    void BuildNodeEditorData(BehaviorNode *root);

    /// <summary>
    /// BehaviorNodeに対応するエディタ用のノードIDを取得または作成
    /// </summary>
    /// <param name="node">IDを取得したい BehaviorNode へのポインタ</param>
    /// <returns>エディタ内で使用する一意のノードID</returns>
    int GetOrCreateNodeId(BehaviorNode *node);

    /// <summary>
    /// 指定したノードを根とするツリーの幅を計算
    /// </summary>
    /// <param name="node">幅を計算するツリーの根となる BehaviorNode へのポインタ</param>
    /// <returns>計算されたツリーの幅を表す整数（単位は実装依存）</returns>
    int CalculateTreeWidth(BehaviorNode *node);

    /// <summary>
    /// BehaviorNode のプロパティを描画
    /// </summary>
    /// <param name="node">プロパティを描画する対象の BehaviorNode へのポインタ</param>
    void DrawNodeProperties(BehaviorNode *node);

    /// <summary>
    /// 指定したツリー名に対応するツールバーを描画
    /// </summary>
    /// <param name="treeName">描画対象のツリーを識別する名前（const std::string&）</param>
    void DrawToolbar(const std::string &treeName);

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    int nodeIdCounter_ = 0;
    BehaviorNode *selectedNode_ = nullptr;
    BehaviorNode *executingNode_ = nullptr;
    BehaviorNode *currentRoot_ = nullptr;
    std::vector<BehaviorNode *> executionHistory_;
    const int MAX_HISTORY = 10;

    // ノードの重み付け情報
    std::unordered_map<BehaviorNode *, float> nodeWeights_;

    // セーブ/ロード用のツリー名
    char treeNameBuffer_[256] = "DefaultTree";

    ed::EditorContext *editorContext_ = nullptr;
    std::unordered_map<BehaviorNode *, int> nodeToEditorId_;
    std::unordered_map<int, BehaviorNode *> editorIdToNode_;
    int nextEditorNodeId_ = 1;
    int nextEditorLinkId_ = 1;

    std::unordered_map<BehaviorNode *, ImVec2> nodePositions_;
    bool needsInitialLayout_ = true;
    bool needsNavigateToContent_ = false;
#endif // _DEBUG
};
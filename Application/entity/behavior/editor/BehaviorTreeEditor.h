#pragma once
#include "Application/entity/behavior/node/BehaviorNode.h"
#include "utility/data/DataHandler.h"
#include <memory>
#include <string>

// ============================================================
// ノードタイプ定義 (Debug / Release 共通)
// ============================================================
enum class EditorNodeType
{
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
    ActionShoot, // 21
    // ActionLockOn (22) は自動ロックオン化に伴い削除。22は欠番として予約し、
    // 以降のint値がずれないよう ConditionIsLockOn を明示的に 23 へ固定する
    ConditionIsLockOn = 23, // 23
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
    ActionFlyApproach, // 30  飛行中にプレイヤーへ水平接近
    // ===== チャージ攻撃・必殺技 =====
    ActionChargeAttack, // 31  溜め→一斉射撃
    ActionUltimate,     // 32  フルコンボ＋大量射撃（エネルギー消費）
    // ===== 追加条件ノード =====
    ConditionPlayerHPLow, // 33  プレイヤーHPが閾値以下か
    ConditionEnergyHigh,  // 34  自身のエネルギーが閾値以上か
    // ===== ガード =====
    ActionGuard,              // 35  一定時間ガードして被ダメージを軽減
    ConditionPlayerAttacking, // 36  プレイヤーが攻撃的(脅威)かチェック
    // ===== ビーム必殺技 =====
    ActionBeamUltimate, // 37  溜め→ビーム必殺技 (MakanAttackSkill相当)
    // ===== 近接追撃 =====
    ActionMeleeChaseCombo // 38  プレイヤーへ詰めながらコンボを出し切る
};

/// <summary>
/// ビヘイビアツリーをJSONから読み込み構築するクラス
/// </summary>
class BehaviorTreeLoader
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 指定JSONファイルからランタイムBTを構築して返す
    /// </summary>
    /// <param name="folder">リソースフォルダ名 (例: "BehaviorTree")</param>
    /// <param name="file">ファイル名 (.json なし, 例: "EnemyBehavior")</param>
    /// <returns>ルートノード (失敗時は nullptr)</returns>
    static std::shared_ptr<BTNode> LoadAndBuild(const std::string &folder, const std::string &file);

  private:
    // --- JSONから復元したノード情報 (内部用) ---
    struct NodeData
    {
        int id = 0;
        EditorNodeType type = EditorNodeType::ActionIdle;
        float param = 0.0f;
        float param2 = 0.0f;
        float param3 = 0.0f;
        float param4 = 0.0f; // 拡散角度など追加パラメータ用
        float param5 = 0.0f; // homingMode(0=拡散, 1=ホーミング)など
        std::string stateName = "Idle";
        // 重み付けノード用
        struct WeightPin
        {
            int pinId = 0;
            float weight = 1.0f;
        };
        std::vector<WeightPin> weightedOutputs;
    };

    struct LinkData
    {
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
#include <nlohmann/json.hpp>
#include <map>
#include <vector>

class Enemy;
class Player;

// 重み付けノードの各出力の情報
struct WeightedOutput
{
    ax::NodeEditor::PinId PinID;
    float Weight = 1.0f;
};

/// <summary>
/// エディタ上のノード情報
/// </summary>
struct EditorNode
{
    ax::NodeEditor::NodeId ID;          // ノードID
    std::string Title;                  // タイトル
    EditorNodeType Type;                // タイプ
    ImVec2 Position;                    // 表示位置
    ax::NodeEditor::PinId InputPinID;   // 入力ピンID
    ax::NodeEditor::PinId OutputPinID;  // 出力ピンID
    ax::NodeEditor::PinId SuccessPinID; // 成功ピンID
    ax::NodeEditor::PinId FailurePinID; // 失敗ピンID

    std::vector<WeightedOutput> WeightedOutputs; // 重み付き出力リスト

    float Parameter = 0.0f;
    float Parameter2 = 0.0f;
    float Parameter3 = 0.0f;
    float Parameter4 = 0.0f; // 拡散角度など
    float Parameter5 = 0.0f; // homingMode(0=拡散, 1=ホーミング)など

    std::string StateNameParameter = "Idle";

    /// <summary>
    /// コンストラクタ
    /// </summary>
    EditorNode(int id, const std::string &title, EditorNodeType type);

    /// <summary>
    /// 条件ノードか判定
    /// </summary>
    bool IsConditionNode() const
    {
        return Type == EditorNodeType::ConditionPlayerClose ||
               Type == EditorNodeType::ConditionHealthLow ||
               Type == EditorNodeType::ConditionEnergyLow ||
               Type == EditorNodeType::ConditionIsGrounded ||
               Type == EditorNodeType::ConditionIsAirborne ||
               Type == EditorNodeType::ConditionPlayerState ||
               Type == EditorNodeType::ConditionIsLockOn ||
               Type == EditorNodeType::ConditionPlayerHPLow ||
               Type == EditorNodeType::ConditionEnergyHigh ||
               Type == EditorNodeType::ConditionPlayerAttacking;
    }

    /// <summary>
    /// アクションノードか判定
    /// </summary>
    bool IsActionNode() const
    {
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
               Type == EditorNodeType::ActionComboStep ||
               Type == EditorNodeType::ActionComboFull ||
               Type == EditorNodeType::ActionBurstShoot ||
               Type == EditorNodeType::ActionEnergyCharge ||
               Type == EditorNodeType::ActionFlyApproach ||
               Type == EditorNodeType::ActionChargeAttack ||
               Type == EditorNodeType::ActionUltimate ||
               Type == EditorNodeType::ActionGuard ||
               Type == EditorNodeType::ActionBeamUltimate ||
               Type == EditorNodeType::ActionMeleeChaseCombo;
    }

    /// <summary>
    /// 重み付けノードか判定
    /// </summary>
    bool IsWeightNode() const
    {
        return Type == EditorNodeType::DecoratorWeight;
    }
};

/// <summary>
/// エディタ上の接続（リンク）情報
/// </summary>
struct EditorLink
{
    ax::NodeEditor::LinkId ID;        // リンクID
    ax::NodeEditor::PinId StartPinID; // 開始ピンID
    ax::NodeEditor::PinId EndPinID;   // 終了ピンID

    /// <summary>
    /// コンストラクタ
    /// </summary>
    EditorLink(int id, ax::NodeEditor::PinId start, ax::NodeEditor::PinId end);
};

/// <summary>
/// ビヘイビアツリーエディタクラス
/// </summary>
class BehaviorTreeEditor
{
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
    /// ImGui描画処理
    /// </summary>
    void OnImGuiRender();

    /// <summary>
    /// デバッグ対象を設定
    /// </summary>
    /// <param name="enemy">敵オブジェクト</param>
    /// <param name="player">プレイヤーオブジェクト</param>
    void SetDebugTargets(Enemy *enemy, Player *player)
    {
        pDebugEnemy_ = enemy;
        pDebugPlayer_ = player;
    }

    /// <summary>
    /// ランタイムのルートノードを取得
    /// </summary>
    std::shared_ptr<BTNode> GetRuntimeRoot() { return runtimeRoot_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 新規ノード作成
    /// </summary>
    void CreateNode(const std::string &title, EditorNodeType type);

    /// <summary>
    /// 選択中のアイテムを削除
    /// </summary>
    void DeleteSelectedItems();

    /// <summary>
    /// 接続作成のハンドル
    /// </summary>
    void HandleCreateAction();

    /// <summary>
    /// ツリーを構築して実行
    /// </summary>
    void BuildAndRunTree();

    /// <summary>
    /// エディタノードからランタイムノードを再帰的にビルド
    /// </summary>
    std::shared_ptr<BTNode> BuildNodeRecursive(int editorNodeId);

    /// <summary>
    /// 指定出力ピンの子ノードIDリストを取得
    /// </summary>
    std::vector<int> FindChildrenNodeIds(int outputPinId);

    /// <summary>
    /// 重み付き子ノードIDリストを取得
    /// </summary>
    std::vector<std::pair<int, float>> FindWeightedChildrenNodeIds(const EditorNode &node);

    /// <summary>
    /// ルートノードのIDを特定
    /// </summary>
    int FindRootNodeId();

    /// <summary>
    /// 入力ピンか判定
    /// </summary>
    bool IsInputPin(ax::NodeEditor::PinId pinId);

    /// <summary>
    /// 出力ピンか判定
    /// </summary>
    bool IsOutputPin(ax::NodeEditor::PinId pinId);

    /// <summary>
    /// 成功出力ピンか判定
    /// </summary>
    bool IsSuccessPin(ax::NodeEditor::PinId pinId);

    /// <summary>
    /// 失敗出力ピンか判定
    /// </summary>
    bool IsFailurePin(ax::NodeEditor::PinId pinId);

    /// <summary>
    /// 重み付き出力ピンか判定
    /// </summary>
    bool IsWeightedOutputPin(ax::NodeEditor::PinId pinId, int &outNodeId, int &outOutputIndex);

    /// <summary>
    /// ツリーを保存
    /// </summary>
    void SaveTree();

    /// <summary>
    /// ツリーを読み込み
    /// </summary>
    void LoadTree(const std::string &filePath);

    /// <summary>
    /// パスをフォルダとファイル名に分割
    /// </summary>
    void ParsePathToFolderAndFile(const std::string &fullPath, std::string &outFolder, std::string &outFile);

    /// <summary>
    /// ノードの説明文を取得
    /// </summary>
    const char *GetNodeDescription(EditorNodeType type);

    /// <summary>
    /// 選択中ノードのプロパティを編集するインスペクターパネルを描画
    /// </summary>
    void DrawInspectorPanel();

    /// <summary>
    /// 指定ノードの編集可能なパラメータ群をインスペクターに描画
    /// </summary>
    /// <param name="node">対象のエディタノード</param>
    void DrawNodeParameters(EditorNode &node);

    /// ===================================================
    /// private variants
    /// ===================================================

    ax::NodeEditor::EditorContext *pContext_ = nullptr; // エディタコンテキスト
    std::vector<EditorNode> nodes_;                    // ノードリスト
    std::vector<EditorLink> links_;                    // リンク（接続）リスト

    int nextNodeId_ = 1;              // 次回割り当てノードID
    int nextLinkId_ = 1;              // 次回割り当てリンクID
    int nextPinId_ = 200000;          // 次回割り当てピンID
    ImVec2 createPos_ = ImVec2(0, 0); // ノード作成位置

    bool isRunning_ = false;        // 実行中フラグ
    Enemy *pDebugEnemy_ = nullptr;   // デバッグ対象敵
    Player *pDebugPlayer_ = nullptr; // デバッグ対象プレイヤー

    std::shared_ptr<BTNode> runtimeRoot_ = nullptr;          // ランタイムツリーのルート
    std::map<int, std::shared_ptr<BTNode>> nodeInstanceMap_; // ノードIDとインスタンスのマップ
    std::map<int, float> statusTimers_;                      // ステータス表示用タイマー
    std::string lastResultText_ = "待機中";                  // 最終実行結果テキスト
    ImVec4 lastResultColor_ = ImVec4(1, 1, 1, 1);            // 最終実行結果表示色

    char inputFileNameBuf_[128] = "NewBehavior"; // ファイル名入力バッファ
    std::string selectedFileName_ = "";          // 選択されたファイル名
    bool showLoadWindow_ = false;                // 読込窓表示フラグ

    bool layoutDirty_ = false;                       // レイアウト変更フラグ
    float saveCooldown_ = 0.0f;                      // 保存クールダウン
    static constexpr float kSaveCooldownTime = 2.0f; // 自動保存までの待機時間

    float inspectorWidth_ = 340.0f; // 右側インスペクターパネルの横幅

    // ── ノード単体デバッグ実行 ──────────────────────────
    int singleTestNodeId_ = -1;                        // 単体テスト対象のエディタノードID (-1=未選択)
    std::shared_ptr<BTNode> singleTestNode_ = nullptr; // 単体テスト用ランタイムノード
    bool isSingleTesting_ = false;                     // 単体テスト実行中フラグ
};

#endif // _DEBUG
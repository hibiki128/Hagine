#pragma once
#include <memory>
#include <string>
#include <vector>

class Enemy;
class BehaviorTreeEditor;
class DistanceCheckNode;

/// <summary>
/// ノードの実行状態を表す列挙型（enum class）。
/// </summary>
enum class NodeStatus {
    Running,
    Success,
    Failure
};

/// <summary>
/// ビヘイビアツリーのノード基底クラス
/// </summary>
class BehaviorBaseNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BehaviorBaseNode() = default;

    /// <summary>
    /// 派生クラスが実装する、Enemy に対する実行処理を表す純粋仮想関数
    /// </summary>
    virtual NodeStatus Execute(Enemy &enemy, float deltaTime) = 0;

    /// <summary>
    /// ノードの名前を取得する純粋仮想関数。派生クラスで実装する必要がある。
    /// </summary>
    virtual const char *GetNodeName() const = 0;

    /// <summary>
    /// エディターを設定（デバッグ用）
    /// </summary>
    static void SetEditor(BehaviorTreeEditor *editor) { editor_ = editor; }

    // ===================================================
    // ツリー構造を辿るための親ノードポインタ
    // ===================================================
    void SetParent(BehaviorBaseNode *parent) { parent_ = parent; } // 親ノード設定
    BehaviorBaseNode *GetParent() const { return parent_; }        // 親ノード取得

    int nodeId = 0;

  protected:
    static BehaviorTreeEditor *editor_;
    BehaviorBaseNode *parent_ = nullptr;
};

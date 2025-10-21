#pragma once
#include <memory>
#include <string>
#include <vector>

class Enemy;
class BehaviorTreeEditor;

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
class BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BehaviorNode() = default;

    /// <summary>
    /// 派生クラスが実装する、Enemy に対する実行処理を表す純粋仮想関数
    /// </summary>
    /// <param name="enemy">処理対象のエネミーを参照で受け取る。呼び出しによりエネミーの状態が変更される可能性がある。</param>
    /// <param name="deltaTime">前フレームからの経過時間（秒）。時間依存の処理やアニメーション更新に使用する。</param>
    /// <returns> NodeStatus 型の値。ノードの実行結果（例: 成功、失敗、実行中）を示す </returns>
    virtual NodeStatus Execute(Enemy &enemy, float deltaTime) = 0;

    /// <summary>
    /// ノードの名前を取得する純粋仮想関数。派生クラスで実装する必要がある。
    /// </summary>
    /// <returns>ノード名を指すヌル終端の文字列への const char*。返されたポインタの有効期間や所有権は実装に依存する。</returns>
    virtual const char *GetNodeName() const = 0;

    /// <summary>
    /// エディターを設定（デバッグ用）
    /// </summary>
    /// <param name="editor">BehaviorTreeEditorへのポインタ</param>
    static void SetEditor(BehaviorTreeEditor *editor) { editor_ = editor; }

    int nodeId = 0;

  protected:
    static BehaviorTreeEditor *editor_;
};
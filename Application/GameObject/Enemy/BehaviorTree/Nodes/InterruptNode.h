#pragma once
#include "../BehaviorNode/BehaviorNode.h"
#include "../Nodes/ConditionNodes.h" // ← DistanceCheckNode参照に必要

/// <summary>
/// 割り込み優先度
/// </summary>
enum class InterruptPriority {
    None = 0,    // 割り込みなし
    Low = 1,     // 低優先度
    Medium = 2,  // 中優先度
    High = 3,    // 高優先度
    Critical = 4 // 最高優先度(弾回避など)
};

/// <summary>
/// 割り込み可能なノードの基底クラス
/// </summary>
class InterruptableNode : public BehaviorNode {
  public:
    virtual ~InterruptableNode() = default;

    /// <summary>
    /// 割り込み条件をチェック
    /// </summary>
    virtual bool CheckInterruptCondition(Enemy &enemy) = 0;

    /// <summary>
    /// 割り込み優先度を取得
    /// </summary>
    virtual InterruptPriority GetInterruptPriority() const = 0;

    /// <summary>
    /// 現在実行中のノードが割り込み可能か
    /// </summary>
    virtual bool CanBeInterrupted() const { return canBeInterrupted_; }

    /// <summary>
    /// 割り込み可能状態を設定
    /// </summary>
    void SetCanBeInterrupted(bool canInterrupt) { canBeInterrupted_ = canInterrupt; }

    /// <summary>
    /// ノードをリセット(割り込み時に呼ばれる)
    /// </summary>
    virtual void Reset() = 0;

    /// ===================================================
    /// 追加: 距離ノード参照関連の汎用ロジック
    /// ===================================================

    /// <summary>
    /// 上位に存在する DistanceCheckNode を探索して返す
    /// </summary>
    DistanceCheckNode *FindLinkedDistanceCheck() const {
        BehaviorNode *current = GetParent();
        while (current) {
            if (auto *distNode = dynamic_cast<DistanceCheckNode *>(current)) {
                return distNode; // 距離チェックノードを見つけたら返す
            }
            current = current->GetParent();
        }
        return nullptr; // 見つからなければnullptr
    }

    /// <summary>
    /// 距離ノード参照を有効にするかどうか
    /// </summary>
    void SetUseLinkedDistance(bool enable) { useLinkedDistance_ = enable; }

    /// <summary>
    /// 距離ノード参照を使用中か
    /// </summary>
    bool GetUseLinkedDistance() const { return useLinkedDistance_; }

  protected:
    bool canBeInterrupted_ = true;  // デフォルトで割り込み可能
    bool useLinkedDistance_ = true; // 追加: デフォルトで距離ノード参照を有効
};

/// <summary>
/// 割り込み対応セレクターノード
/// 優先度の高い割り込み条件をチェックして、必要に応じて実行中のノードを中断
/// </summary>
class InterruptSelectorNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "InterruptSelector"; }

    void AddChild(std::unique_ptr<BehaviorNode> child) {
        child->SetParent(this); // 親ノードを設定（重要）
        children_.push_back(std::move(child));
    }

    std::vector<std::unique_ptr<BehaviorNode>> &GetChildren() { return children_; }

    /// <summary>
    /// 現在実行中のノードをリセット
    /// </summary>
    void ResetCurrentChild();

  private:
    std::vector<std::unique_ptr<BehaviorNode>> children_;
    int currentChildIndex_ = -1;
    InterruptPriority currentPriority_ = InterruptPriority::None;

    /// <summary>
    /// 最も優先度の高い割り込み可能なノードを検索
    /// </summary>
    int FindHighestPriorityInterrupt(Enemy &enemy);
};

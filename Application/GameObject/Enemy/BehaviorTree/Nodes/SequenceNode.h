#pragma once
#include "Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"

/// <summary>
/// すべての子ノードが成功するまで順序で実行するビヘイビアノード
/// 一つでも失敗すればその時点で失敗を返す
/// </summary>
class SequenceNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// ノードの実行処理
    /// </summary>
    /// <param name="enemy">対象のEnemy参照</param>
    /// <param name="deltaTime">フレームの経過時間</param>
    /// <returns>ノードの実行結果</returns>
    NodeStatus Execute(Enemy &enemy, float deltaTime) override {
        for (auto &child : children_) {
            NodeStatus status = child->Execute(enemy, deltaTime);
            if (status != NodeStatus::Success) {
                return status;
            }
        }
        return NodeStatus::Success;
    }

    /// <summary>
    /// ノード名取得
    /// </summary>
    /// <returns>ノードの名前</returns>
    const char *GetNodeName() const override { return "Sequence"; }

    /// <summary>
    /// 子ノードを追加
    /// </summary>
    /// <param name="child">追加する子ノード</param>
    void AddChild(std::unique_ptr<BehaviorNode> child) {
        children_.push_back(std::move(child));
    }

    /// <summary>
    /// 子ノードの一覧を取得
    /// </summary>
    /// <returns>子ノードのvector参照</returns>
    std::vector<std::unique_ptr<BehaviorNode>> &GetChildren() { return children_; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    std::vector<std::unique_ptr<BehaviorNode>> children_; // 子ノードの一覧
};

/// <summary>
/// いずれかの子ノードが成功するまで順序で実行するビヘイビアノード
/// 一つでも成功すればその時点で成功を返す
/// </summary>
class SelectorNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// ノードの実行処理
    /// </summary>
    /// <param name="enemy">対象のEnemy参照</param>
    /// <param name="deltaTime">フレームの経過時間</param>
    /// <returns>ノードの実行結果</returns>
    NodeStatus Execute(Enemy &enemy, float deltaTime) override {
        for (auto &child : children_) {
            NodeStatus status = child->Execute(enemy, deltaTime);
            if (status != NodeStatus::Failure) {
                return status;
            }
        }
        return NodeStatus::Failure;
    }

    /// <summary>
    /// ノード名取得
    /// </summary>
    /// <returns>ノードの名前</returns>
    const char *GetNodeName() const override { return "Selector"; }

    /// <summary>
    /// 子ノードを追加
    /// </summary>
    /// <param name="child">追加する子ノード</param>
    void AddChild(std::unique_ptr<BehaviorNode> child) {
        children_.push_back(std::move(child));
    }

    /// <summary>
    /// 子ノードの一覧を取得
    /// </summary>
    /// <returns>子ノードのvector参照</returns>
    std::vector<std::unique_ptr<BehaviorNode>> &GetChildren() { return children_; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    std::vector<std::unique_ptr<BehaviorNode>> children_; // 子ノードの一覧
};
#pragma once
#include "myMath.h" // 環境に合わせて
#include <memory>
#include <string>
#include <vector>

class Enemy;
class Player;

enum class NodeStatus {
    Success,
    Failure,
    Running,
    Idle
};

class BTNode {
  public:
    virtual ~BTNode() = default;
    NodeStatus Tick();
    virtual void AddChild(std::shared_ptr<BTNode> child);
    NodeStatus GetStatus() const { return m_Status; }
    virtual void Reset() { m_Status = NodeStatus::Idle; }
    virtual void SetContext(Enemy *enemy, Player *player);

  protected:
    virtual void OnEnter();
    virtual NodeStatus OnUpdate() = 0;
    virtual void OnExit();

    NodeStatus m_Status = NodeStatus::Idle;
};

class ContextNode : public BTNode {
  protected:
    Enemy *m_Enemy = nullptr;
    Player *m_Player = nullptr;
    void SetContext(Enemy *enemy, Player *player) override {
        m_Enemy = enemy;
        m_Player = player;
    }
};

class CompositeNode : public BTNode {
  public:
    void AddChild(std::shared_ptr<BTNode> child) override;
    void SetContext(Enemy *enemy, Player *player) override;
    void Reset() override {
        BTNode::Reset();
        m_CurrentChildIndex = 0;
        for (auto &child : m_Children) {
            child->Reset();
        }
    }

  protected:
    void OnEnter() override;
    std::vector<std::shared_ptr<BTNode>> m_Children;
    int m_CurrentChildIndex = 0;
};

// ---------------------------------------------------------
// SequenceNode
// ---------------------------------------------------------
class SequenceNode : public CompositeNode {
  protected:
    NodeStatus OnUpdate() override;
};

// ---------------------------------------------------------
// SelectorNode
// ---------------------------------------------------------
class SelectorNode : public CompositeNode {
  protected:
    NodeStatus OnUpdate() override;
};

// ---------------------------------------------------------
// RunActionNode
// ---------------------------------------------------------
class RunActionNode : public BTNode {
  public:
    RunActionNode();
    void Reset() override {
        BTNode::Reset();
        m_Counter = 0;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    int m_Counter;
    int m_Duration;
};

// =========================================================
// 具体的な条件ノード
// =========================================================

// ★変更: プレイヤーが「指定範囲内」にいるか？
class IsPlayerCloseNode : public ContextNode {
  public:
    // コンストラクタで最小・最大を受け取る
    IsPlayerCloseNode(float min, float max) : m_MinDist(min), m_MaxDist(max) {}

  protected:
    NodeStatus OnUpdate() override;

  private:
    float m_MinDist;
    float m_MaxDist;
};

// HPが低いか？
class IsHealthLowNode : public ContextNode {
  public:
    IsHealthLowNode(float percentage) : m_ThresholdPercentage(percentage) {}

  protected:
    NodeStatus OnUpdate() override;

  private:
    float m_ThresholdPercentage;
};

// =========================================================
// 具体的なアクションノード
// =========================================================

class EnemyApproachNode : public ContextNode {
  protected:
    NodeStatus OnUpdate() override;
};

class EnemyAttackNode : public ContextNode {
  public:
    EnemyAttackNode() { m_Timer = 0.0f; }
    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
    }

  protected:
    NodeStatus OnUpdate() override;

  private:
    float m_Timer = 0.0f;
};
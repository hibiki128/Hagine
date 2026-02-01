#pragma once
#include "myMath.h" // 環境に合わせて
#include <memory>
#include <random.h>
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

// =========================================================
// 時間制限付きアクションの基底クラス
// =========================================================
class TimedActionNode : public ContextNode {
  public:
    // minTime:最小時間, maxTime:最大時間, speed:移動速度
    TimedActionNode(float minTime, float maxTime, float speed)
        : m_MinTime(minTime), m_MaxTime(maxTime), m_Speed(speed) {}

    void Reset() override {
        BTNode::Reset();
        m_CurrentTimer = 0.0f;
        m_TargetDuration = 0.0f;
    }

  protected:
    void OnEnter() override;

    NodeStatus OnUpdate() override;

    // 派生クラスで実装する
    virtual void ExecuteAction() = 0;
    virtual void SetupAction() {} // 必要ならオーバーライド

    float m_MinTime;
    float m_MaxTime;
    float m_Speed;
    float m_CurrentTimer = 0.0f;
    float m_TargetDuration = 0.0f;
};

class WeightDecoratorNode : public CompositeNode {
  public:
    WeightDecoratorNode(float weight) : m_Weight(weight) {}

    // 実行時は単に子ノード(アクションなど)へパススルーするだけ
    NodeStatus OnUpdate() override {
        if (m_Children.empty())
            return NodeStatus::Failure;
        return m_Children[0]->Tick();
    }

    float GetWeight() const { return m_Weight; }

  private:
    float m_Weight = 1.0f;
};

class RandomSelectorNode : public CompositeNode {
  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    int m_SelectedChildIndex = -1;
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

// ★変更: プレイヤーが「指定範囲内」にいるか?
class IsPlayerCloseNode : public ContextNode {
  public:
    // コンストラクタで最小・最大を受け取る
    IsPlayerCloseNode(float min, float max) : m_MinDist(min), m_MaxDist(max) {}

    void Reset() override {
        BTNode::Reset();
        m_LastResult = NodeStatus::Idle;
        m_StableTimer = 0.0f;
    }

  protected:
    NodeStatus OnUpdate() override;

  private:
    float m_MinDist;
    float m_MaxDist;

    // ★追加: ヒステリシス用の変数
    NodeStatus m_LastResult = NodeStatus::Idle;      // 前回の判定結果
    float m_StableTimer = 0.0f;                      // 状態が安定している時間
    static constexpr float kHysteresisMargin = 1.0f; // ヒステリシスのマージン(1.0m)
    static constexpr float kMinStableTime = 0.3f;    // 最小安定時間(0.3秒)
    static constexpr float kSuccessHoldTime = 0.5f;  // 成功状態の保持時間(0.5秒)
};

// HPが低いか?
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

// 通常接近
class EnemyApproachNode : public TimedActionNode {
  public:
    using TimedActionNode::TimedActionNode; // コンストラクタ継承
  protected:
    void ExecuteAction() override;
};

// 高速接近 (ロジックは接近と同じだが、エディタでパラメータを変えて使い分ける)
class EnemyDashNode : public TimedActionNode {
  public:
    using TimedActionNode::TimedActionNode;

  protected:
    void ExecuteAction() override; // 中身はApproachと同じでOK
};

// 左右移動 (Strafe)
class EnemyStrafeNode : public TimedActionNode {
  public:
    using TimedActionNode::TimedActionNode;

  protected:
    void SetupAction() override; // ここで左右どちらに行くか決める
    void ExecuteAction() override;
};

// 後退 (Retreat)
class EnemyRetreatNode : public TimedActionNode {
  public:
    using TimedActionNode::TimedActionNode;

  protected:
    void ExecuteAction() override;
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

// 待機アクション (何もしない)
class EnemyIdleNode : public ContextNode {
  public:
    EnemyIdleNode(float duration = 1.0f) : m_Duration(duration), m_Timer(0.0f) {}

    void Reset() override;

    void OnEnter() override;

  protected:
    NodeStatus OnUpdate() override {
        m_Timer += 1.0f / 60.0f;
        if (m_Timer >= m_Duration)
            return NodeStatus::Success;
        return NodeStatus::Running;
    }

  private:
    float m_Duration;
    float m_Timer;
};
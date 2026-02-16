#pragma once
#include "myMath.h"
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

    // 終了時に速度をゼロにする
    void OnExit() override;

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

// プレイヤーが「指定範囲内」にいるか?
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

    // ヒステリシス用の変数
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

// ★新規追加: 地上にいるかチェック
class IsGroundedNode : public ContextNode {
  public:
    IsGroundedNode() {}

  protected:
    NodeStatus OnUpdate() override;
};

// ★新規追加: 空中にいるかチェック
class IsAirborneNode : public ContextNode {
  public:
    IsAirborneNode() {}

  protected:
    NodeStatus OnUpdate() override;
};

// =========================================================
// 地上移動アクションノード
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

// =========================================================
// ★新規追加: ジャンプ・飛行関連ノード
// =========================================================

// ジャンプノード (プレイヤーの PlayerStateJump に相当)
class EnemyJumpNode : public ContextNode {
  public:
    EnemyJumpNode(float jumpPower = 15.0f) : m_JumpPower(jumpPower) {}

    void Reset() override {
        BTNode::Reset();
        m_JumpExecuted = false;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    float m_JumpPower;           // ジャンプ力
    bool m_JumpExecuted = false; // ジャンプが実行されたか
};

// ジャンプから飛行状態への遷移ノード
// PlayerStateAir の飛行遷移ロジックを参考
class EnemyJumpToFlyNode : public ContextNode {
  public:
    // ★修正: ジャンプ力と飛行遷移時間をパラメータとして受け取る
    EnemyJumpToFlyNode(float jumpPower = 15.0f, float transitionTime = 0.5f)
        : m_JumpPower(jumpPower), m_FlyTransitionTime(transitionTime), m_ElapsedTime(0.0f), m_HasJumped(false) {}

    void Reset() override {
        BTNode::Reset();
        m_ElapsedTime = 0.0f;
        m_HasJumped = false;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    float m_JumpPower;         // ★追加: ジャンプ力
    float m_FlyTransitionTime; // ★追加: 飛行遷移までの時間
    float m_ElapsedTime;
    bool m_HasJumped; // ★追加: ジャンプ済みフラグ
};

// 飛行状態: 上昇動作 (PlayerStateFlyMove の上昇処理を参考)
class EnemyFlyAscendNode : public ContextNode {
  public:
    EnemyFlyAscendNode(float minTime, float maxTime, float speed = 15.0f)
        : m_MinTime(minTime), m_MaxTime(maxTime), m_Speed(speed) {}

    void Reset() override {
        BTNode::Reset();
        m_CurrentTimer = 0.0f;
        m_TargetDuration = 0.0f;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;
    void OnExit() override;

  private:
    float m_MinTime;
    float m_MaxTime;
    float m_Speed; // 上昇速度
    float m_CurrentTimer;
    float m_TargetDuration;
    static constexpr float kFlyAcceleration = 30.0f; // 飛行加速度
};

// 飛行状態: 下降動作
class EnemyFlyDescendNode : public ContextNode {
  public:
    EnemyFlyDescendNode(float minTime, float maxTime, float speed = 15.0f)
        : m_MinTime(minTime), m_MaxTime(maxTime), m_Speed(speed) {}

    void Reset() override {
        BTNode::Reset();
        m_CurrentTimer = 0.0f;
        m_TargetDuration = 0.0f;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;
    void OnExit() override;

  private:
    float m_MinTime;
    float m_MaxTime;
    float m_Speed; // 下降速度
    float m_CurrentTimer;
    float m_TargetDuration;
    static constexpr float kFlyAcceleration = 30.0f;
};

// 飛行から地上への遷移ノード (着地)
class EnemyFlyToGroundNode : public ContextNode {
  public:
    EnemyFlyToGroundNode() {}

    void Reset() override {
        BTNode::Reset();
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    static constexpr float kGroundLevel = 0.0f; // 地面レベル
};
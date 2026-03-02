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
// SequenceNode  (Reactive)
// ---------------------------------------------------------
class SequenceNode : public CompositeNode {
  protected:
    NodeStatus OnUpdate() override;
};

// ---------------------------------------------------------
// SequenceOnceNode  (Non-Reactive)
// Running中のアクションは条件変化で中断されない
// ---------------------------------------------------------
class SequenceOnceNode : public CompositeNode {
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

// エネルギーが低いか?
class IsEnergyLowNode : public ContextNode {
  public:
    IsEnergyLowNode(float percentage) : m_ThresholdPercentage(percentage) {}

  protected:
    NodeStatus OnUpdate() override;

  private:
    float m_ThresholdPercentage; // エネルギー比率の閾値(0.0〜1.0)
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

// ★新規追加: プレイヤーのステートをチェック
class IsPlayerStateNode : public ContextNode {
  public:
    IsPlayerStateNode(const std::string &stateName) : m_StateName(stateName) {}

  protected:
    NodeStatus OnUpdate() override;

  private:
    std::string m_StateName; // チェックするステート名
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
// ジャンプ→最高到達点で重力カット→飛行状態へ移行
class EnemyJumpToFlyNode : public ContextNode {
  public:
    EnemyJumpToFlyNode(float jumpPower = 15.0f) : m_JumpPower(jumpPower), m_ElapsedTime(0.0f) {}

    void Reset() override {
        BTNode::Reset();
        m_ElapsedTime = 0.0f;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;
    void OnExit() override;

  private:
    float m_JumpPower; // ジャンプ力
    float m_ElapsedTime;
    static constexpr float kFlyTransitionTime = 1.0f; // 飛行遷移可能時間
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

// =========================================================
// 飛行状態: 水平接近動作
// 飛行中のまま水平方向にプレイヤーへ近づく
// =========================================================
class EnemyFlyApproachNode : public ContextNode {
  public:
    // minTime: 最小実行秒, maxTime: 最大実行秒, speed: 水平移動速度
    EnemyFlyApproachNode(float minTime, float maxTime, float speed = 10.0f)
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
    float m_Speed;
    float m_CurrentTimer = 0.0f;
    float m_TargetDuration = 0.0f;
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

// =========================================================
// 射撃・ロックオン関連ノード
// =========================================================

/// <summary>
/// 弾を発射するアクションノード
/// param: 発射後の待機時間(クールダウン秒数)
/// </summary>
class EnemyShootNode : public ContextNode {
  public:
    EnemyShootNode(float cooldown = 1.0f) : m_Cooldown(cooldown) {}

    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
        m_HasShot = false;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    float m_Cooldown;       // 発射後の待機時間(秒)
    float m_Timer = 0.0f;   // 経過時間
    bool m_HasShot = false; // 発射済みフラグ
};

/// <summary>
/// ロックオン状態を切り替えるアクションノード
/// param: 1.0 = ON, 0.0 = OFF
/// </summary>
class EnemyLockOnNode : public ContextNode {
  public:
    EnemyLockOnNode(bool lockOn = true) : m_LockOn(lockOn) {}

    void Reset() override { BTNode::Reset(); }

  protected:
    NodeStatus OnUpdate() override;

  private:
    bool m_LockOn; // trueでロックオンON、falseでOFF
};

/// <summary>
/// ロックオン中かどうかをチェックする条件ノード
/// </summary>
class IsEnemyLockOnNode : public ContextNode {
  public:
    IsEnemyLockOnNode() {}

  protected:
    NodeStatus OnUpdate() override;
};

// =========================================================
// 近接攻撃コンボノード
// =========================================================

/// <summary>
/// コンボを1段階だけ実行するアクションノード
/// param : 1段階のモーション待機時間(秒)
/// param2: コンボ間隔オーバーライド(秒)。0=ComboSystemデフォルト(0.15s)
/// </summary>
class EnemyComboStepNode : public ContextNode {
  public:
    EnemyComboStepNode(float stepDuration = 0.5f, float comboInterval = 0.0f)
        : m_StepDuration(stepDuration), m_ComboInterval(comboInterval) {}

    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
        m_HasStep = false;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    float m_StepDuration;  // 1段階の実行時間(秒)
    float m_ComboInterval; // コンボ間隔オーバーライド(0=デフォルト)
    float m_Timer = 0.0f;
    bool m_HasStep = false;
};

/// <summary>
/// コンボを最後まで全段実行するアクションノード
/// param  : 1段あたりのモーション待機時間(秒)
/// param2 : 最大コンボ段数 (0 = 全段)
/// param3 : コンボ間隔オーバーライド(秒)。0=ComboSystemデフォルト(0.15s)
/// </summary>
class EnemyComboFullNode : public ContextNode {
  public:
    EnemyComboFullNode(float stepDuration = 0.5f, int maxSteps = 0, float comboInterval = 0.0f)
        : m_StepDuration(stepDuration), m_MaxSteps(maxSteps), m_ComboInterval(comboInterval) {}

    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
        m_StepCount = 0;
        m_WaitingStep = false;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    float m_StepDuration;  // 1段あたりの実行時間(秒)
    int m_MaxSteps;        // 最大段数 (0 = ComboSystemが終わるまで)
    float m_ComboInterval; // コンボ間隔オーバーライド(0=デフォルト)
    float m_Timer = 0.0f;
    int m_StepCount = 0;
    bool m_WaitingStep = false;
};

// =========================================================
// 連射ノード
// =========================================================

/// <summary>
/// 指定弾数を連発するアクションノード（拡散・ホーミング切替対応）
///
/// param  : 発射間隔(秒)
/// param2 : 発射弾数
/// param3 : 全弾発射後のクールダウン(秒)
/// spreadAngle    : 拡散角度(度)。0=直進、30=±15度に広がる
/// homingMode     : true=ロックオン追従弾、false=拡散弾（向いている方向ベース）
/// </summary>
class EnemyBurstShootNode : public ContextNode {
  public:
    EnemyBurstShootNode(
        float interval = 0.2f,
        int count = 3,
        float cooldown = 0.5f,
        float spreadAngle = 0.0f,
        bool homingMode = false)
        : m_Interval(interval), m_BurstCount(count), m_Cooldown(cooldown), m_SpreadAngle(spreadAngle), m_HomingMode(homingMode) {}

    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
        m_ShotsFired = 0;
        m_Phase = Phase::Shooting;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    enum class Phase { Shooting,
                       Cooldown };

    void FireOneBullet(); // 1発分の発射処理（拡散込み）

    float m_Interval;    // 発射間隔(秒)
    int m_BurstCount;    // 連発弾数
    float m_Cooldown;    // 全弾後クールダウン(秒)
    float m_SpreadAngle; // 拡散角度の半角(度)。合計拡散幅 = SpreadAngle
    bool m_HomingMode;   // true=ロックオン追従, false=拡散固定弾

    float m_Timer = 0.0f;
    int m_ShotsFired = 0;
    Phase m_Phase = Phase::Shooting;
};

// =========================================================
// エネルギーチャージノード
// =========================================================

/// <summary>
/// エネルギーが最大値に達するまでチャージするアクションノード
/// param : チャージ速度倍率(1.0=通常速度, 2.0=2倍速)
/// param2: 目標エネルギー比率(0.0〜1.0, 0=最大まで)
/// </summary>
class EnemyEnergyChargeNode : public ContextNode {
  public:
    EnemyEnergyChargeNode(float chargeRateMultiplier = 1.0f, float targetRatio = 1.0f)
        : m_ChargeRateMultiplier(chargeRateMultiplier), m_TargetRatio(targetRatio) {}

    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;
    void OnExit() override;

  private:
    float m_ChargeRateMultiplier; // チャージ速度の倍率
    float m_TargetRatio;          // 目標エネルギー比率(1.0=最大値まで)
    float m_Timer = 0.0f;         // 経過時間
    float m_OriginalRecoveryRate = 0.0f; // 元の回復レートを保存用
};
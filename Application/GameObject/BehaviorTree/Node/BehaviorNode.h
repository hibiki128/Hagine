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

/// <summary>
/// ビヘイビアツリーのノード基底クラス
/// </summary>
class BTNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    virtual ~BTNode() = default;

    /// <summary>
    /// ノードの実行
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    NodeStatus Tick();

    /// <summary>
    /// 子ノードの追加
    /// </summary>
    /// <param name="child">追加する子ノード</param>
    virtual void AddChild(std::shared_ptr<BTNode> child);

    /// <summary>
    /// 現在の状態を取得
    /// </summary>
    /// <returns>NodeStatus: 現在の状態</returns>
    NodeStatus GetStatus() const { return m_Status; }

    /// <summary>
    /// 状態をリセット
    /// </summary>
    virtual void Reset() { m_Status = NodeStatus::Idle; }

    /// <summary>
    /// コンテキスト（敵・プレイヤー）を設定
    /// </summary>
    /// <param name="enemy">敵オブジェクト</param>
    /// <param name="player">プレイヤーオブジェクト</param>
    virtual void SetContext(Enemy *enemy, Player *player);

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    virtual void OnEnter();

    /// <summary>
    /// 更新処理（派生クラスで実装）
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    virtual NodeStatus OnUpdate() = 0;

    /// <summary>
    /// ノード終了時の処理
    /// </summary>
    virtual void OnExit();

    NodeStatus m_Status = NodeStatus::Idle; // ノードの現在の状態
};

/// <summary>
/// 敵とプレイヤーのコンテキストを持つノードクラス
/// </summary>
class ContextNode : public BTNode {
  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// コンテキスト（敵・プレイヤー）を設定
    /// </summary>
    /// <param name="enemy">敵オブジェクト</param>
    /// <param name="player">プレイヤーオブジェクト</param>
    void SetContext(Enemy *enemy, Player *player) override {
        m_Enemy = enemy;
        m_Player = player;
    }

    /// ===================================================
    /// protected variants
    /// ===================================================
    Enemy *m_Enemy = nullptr;  // 敵オブジェクトへのポインタ
    Player *m_Player = nullptr; // プレイヤーオブジェクトへのポインタ
};

/// <summary>
/// 複数の子ノードを持つコンポジットノードの基底クラス
/// </summary>
class CompositeNode : public BTNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 子ノードの追加
    /// </summary>
    /// <param name="child">追加する子ノード</param>
    void AddChild(std::shared_ptr<BTNode> child) override;

    /// <summary>
    /// コンテキストを設定
    /// </summary>
    /// <param name="enemy">敵オブジェクト</param>
    /// <param name="player">プレイヤーオブジェクト</param>
    void SetContext(Enemy *enemy, Player *player) override;

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_CurrentChildIndex = 0;
        for (auto &child : m_Children) {
            child->Reset();
        }
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// ===================================================
    /// protected variants
    /// ===================================================
    std::vector<std::shared_ptr<BTNode>> m_Children; // 子ノードのリスト
    int m_CurrentChildIndex = 0;                      // 現在実行中の子ノードのインデックス
};

/// <summary>
/// 時間制限付きアクションの基底クラス
/// </summary>
class TimedActionNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="minTime">最小実行時間</param>
    /// <param name="maxTime">最大実行時間</param>
    /// <param name="speed">移動速度</param>
    TimedActionNode(float minTime, float maxTime, float speed)
        : m_MinTime(minTime), m_MaxTime(maxTime), m_Speed(speed) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_CurrentTimer = 0.0f;
        m_TargetDuration = 0.0f;
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    NodeStatus OnUpdate() override;

    /// <summary>
    /// ノード終了時の処理（速度をゼロにする）
    /// </summary>
    void OnExit() override;

    /// <summary>
    /// アクションの実行（派生クラスで実装）
    /// </summary>
    virtual void ExecuteAction() = 0;

    /// <summary>
    /// アクションのセットアップ
    /// </summary>
    virtual void SetupAction() {}

    /// ===================================================
    /// protected variants
    /// ===================================================
    float m_MinTime;                 // 最小実行時間
    float m_MaxTime;                 // 最大実行時間
    float m_Speed;                   // 移動速度
    float m_CurrentTimer = 0.0f;    // 現在の経過時間
    float m_TargetDuration = 0.0f;  // 目標の実行時間
};

/// <summary>
/// 重み付きデコレーターノード
/// 確率的な選択に使用される
/// </summary>
class WeightDecoratorNode : public CompositeNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="weight">重み</param>
    WeightDecoratorNode(float weight) : m_Weight(weight) {}

    /// <summary>
    /// 更新処理（子ノードにパススルー）
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    NodeStatus OnUpdate() override {
        if (m_Children.empty())
            return NodeStatus::Failure;
        return m_Children[0]->Tick();
    }

    /// <summary>
    /// 重みを取得
    /// </summary>
    /// <returns>float: 重みの値</returns>
    float GetWeight() const { return m_Weight; }

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    float m_Weight = 1.0f; // 重みの値
};

/// <summary>
/// 子ノードからランダムに一つを選択して実行するノード
/// </summary>
class RandomSelectorNode : public CompositeNode {
  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理（ランダムに子を選択）
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    int m_SelectedChildIndex = -1; // 選択された子のインデックス
};

/// <summary>
/// 子ノードを順番に実行するノード (Reactive)
/// 毎フレーム条件を再評価する
/// </summary>
class SequenceNode : public CompositeNode {
  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    NodeStatus OnUpdate() override;
};

/// <summary>
/// 子ノードを順番に最後まで実行するノード (Non-Reactive)
/// Running中のアクションは条件変化で中断されない
/// </summary>
class SequenceOnceNode : public CompositeNode {
  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    NodeStatus OnUpdate() override;
};

/// <summary>
/// 子ノードを順番に実行し、一つでも成功すれば終了するノード
/// </summary>
class SelectorNode : public CompositeNode {
  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    NodeStatus OnUpdate() override;
};

/// <summary>
/// 指定時間実行し続けるアクションノード
/// </summary>
class RunActionNode : public BTNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    RunActionNode();

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_Counter = 0;
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果の状態</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    int m_Counter;  // 経過フレームカウンター
    int m_Duration; // 実行フレーム数
};

/// <summary>
/// プレイヤーとの距離を判定する条件ノード
/// </summary>
class IsPlayerCloseNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="min">最小距離</param>
    /// <param name="max">最大距離</param>
    IsPlayerCloseNode(float min, float max) : m_MinDist(min), m_MaxDist(max) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_LastResult = NodeStatus::Idle;
        m_StableTimer = 0.0f;
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 判定結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    float m_MinDist; // 判定の最小距離
    float m_MaxDist; // 判定の最大距離

    NodeStatus m_LastResult = NodeStatus::Idle;      // 前回の判定結果
    float m_StableTimer = 0.0f;                      // 状態が安定している時間
    static constexpr float kHysteresisMargin = 1.0f; // ヒステリシスのマージン
    static constexpr float kMinStableTime = 0.3f;    // 最小安定時間
    static constexpr float kSuccessHoldTime = 0.5f;  // 成功状態の保持時間
};

/// <summary>
/// HPが閾値を下回っているか判定する条件ノード
/// </summary>
class IsHealthLowNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="percentage">閾値（0.0〜1.0）</param>
    IsHealthLowNode(float percentage) : m_ThresholdPercentage(percentage) {}

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 判定結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    float m_ThresholdPercentage; // HP比率の閾値
};

/// <summary>
/// エネルギーが閾値を下回っているか判定する条件ノード
/// </summary>
class IsEnergyLowNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="percentage">閾値（0.0〜1.0）</param>
    IsEnergyLowNode(float percentage) : m_ThresholdPercentage(percentage) {}

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 判定結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    float m_ThresholdPercentage; // エネルギー比率の閾値
};

/// <summary>
/// 接地しているか判定する条件ノード
/// </summary>
class IsGroundedNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    IsGroundedNode() {}

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 判定結果</returns>
    NodeStatus OnUpdate() override;
};

/// <summary>
/// 空中にいるか判定する条件ノード
/// </summary>
class IsAirborneNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    IsAirborneNode() {}

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 判定結果</returns>
    NodeStatus OnUpdate() override;
};

/// <summary>
/// プレイヤーのステートを判定する条件ノード
/// </summary>
class IsPlayerStateNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="stateName">ステート名</param>
    IsPlayerStateNode(const std::string &stateName) : m_StateName(stateName) {}

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 判定結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    std::string m_StateName; // チェックするステート名
};

/// <summary>
/// プレイヤーに通常接近するアクションノード
/// </summary>
class EnemyApproachNode : public TimedActionNode {
  public:
    using TimedActionNode::TimedActionNode;
  protected:
    /// <summary>
    /// アクションの実行
    /// </summary>
    void ExecuteAction() override;
};

/// <summary>
/// プレイヤーに高速接近するアクションノード
/// </summary>
class EnemyDashNode : public TimedActionNode {
  public:
    using TimedActionNode::TimedActionNode;
  protected:
    /// <summary>
    /// アクションの実行
    /// </summary>
    void ExecuteAction() override;
};

/// <summary>
/// 左右移動（回り込み）を行うアクションノード
/// </summary>
class EnemyStrafeNode : public TimedActionNode {
  public:
    using TimedActionNode::TimedActionNode;
  protected:
    /// <summary>
    /// アクションのセットアップ
    /// </summary>
    void SetupAction() override;

    /// <summary>
    /// アクションの実行
    /// </summary>
    void ExecuteAction() override;
};

/// <summary>
/// プレイヤーから遠ざかるアクションノード
/// </summary>
class EnemyRetreatNode : public TimedActionNode {
  public:
    using TimedActionNode::TimedActionNode;
  protected:
    /// <summary>
    /// アクションの実行
    /// </summary>
    void ExecuteAction() override;
};

/// <summary>
/// 攻撃を行うアクションノード
/// </summary>
class EnemyAttackNode : public ContextNode {
  public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    EnemyAttackNode() { m_Timer = 0.0f; }

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
    }

  protected:
    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

  private:
    float m_Timer = 0.0f; // 攻撃の経過時間
};

/// <summary>
/// 待機するアクションノード
/// </summary>
class EnemyIdleNode : public ContextNode {
  public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="duration">待機時間</param>
    EnemyIdleNode(float duration = 1.0f) : m_Duration(duration), m_Timer(0.0f) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override;

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

  protected:
    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override {
        m_Timer += 1.0f / 60.0f;
        if (m_Timer >= m_Duration)
            return NodeStatus::Success;
        return NodeStatus::Running;
    }

  private:
    float m_Duration; // 待機する秒数
    float m_Timer;    // 現在の経過時間
};

/// <summary>
/// ジャンプを行うアクションノード
/// </summary>
class EnemyJumpNode : public ContextNode {
  public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="jumpPower">ジャンプ力</param>
    EnemyJumpNode(float jumpPower = 15.0f) : m_JumpPower(jumpPower) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_JumpExecuted = false;
    }

  protected:
    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

  private:
    float m_JumpPower;           // ジャンプ力
    bool m_JumpExecuted = false; // ジャンプが実行されたか
};

/// <summary>
/// ジャンプから飛行状態へ遷移するノード
/// </summary>
class EnemyJumpToFlyNode : public ContextNode {
  public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="jumpPower">ジャンプ力</param>
    EnemyJumpToFlyNode(float jumpPower = 15.0f) : m_JumpPower(jumpPower), m_ElapsedTime(0.0f) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_ElapsedTime = 0.0f;
    }

  protected:
    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

    /// <summary>
    /// ノード終了時の処理
    /// </summary>
    void OnExit() override;

  private:
    float m_JumpPower;  // ジャンプ力
    float m_ElapsedTime; // 経過時間
    static constexpr float kFlyTransitionTime = 1.0f; // 飛行遷移可能時間
};

/// <summary>
/// 飛行状態での上昇アクションノード
/// </summary>
class EnemyFlyAscendNode : public ContextNode {
  public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="minTime">最小実行時間</param>
    /// <param name="maxTime">最大実行時間</param>
    /// <param name="speed">上昇速度</param>
    EnemyFlyAscendNode(float minTime, float maxTime, float speed = 15.0f)
        : m_MinTime(minTime), m_MaxTime(maxTime), m_Speed(speed) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_CurrentTimer = 0.0f;
        m_TargetDuration = 0.0f;
    }

  protected:
    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

    /// <summary>
    /// ノード終了時の処理
    /// </summary>
    void OnExit() override;

  private:
    float m_MinTime;                 // 最小実行時間
    float m_MaxTime;                 // 最大実行時間
    float m_Speed;                   // 上昇速度
    float m_CurrentTimer;           // 現在の経過時間
    float m_TargetDuration;         // 目標の実行時間
    static constexpr float kFlyAcceleration = 30.0f; // 飛行加速度
};

/// <summary>
/// 飛行状態での下降アクションノード
/// </summary>
class EnemyFlyDescendNode : public ContextNode {
  public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="minTime">最小実行時間</param>
    /// <param name="maxTime">最大実行時間</param>
    /// <param name="speed">下降速度</param>
    EnemyFlyDescendNode(float minTime, float maxTime, float speed = 15.0f)
        : m_MinTime(minTime), m_MaxTime(maxTime), m_Speed(speed) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_CurrentTimer = 0.0f;
        m_TargetDuration = 0.0f;
    }

  protected:
    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

    /// <summary>
    /// ノード終了時の処理
    /// </summary>
    void OnExit() override;

  private:
    float m_MinTime;                 // 最小実行時間
    float m_MaxTime;                 // 最大実行時間
    float m_Speed;                   // 下降速度
    float m_CurrentTimer;           // 現在の経過時間
    float m_TargetDuration;         // 目標の実行時間
    static constexpr float kFlyAcceleration = 30.0f; // 飛行加速度
};

/// <summary>
/// 飛行状態での水平接近アクションノード
/// </summary>
class EnemyFlyApproachNode : public ContextNode {
  public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="minTime">最小実行時間</param>
    /// <param name="maxTime">最大実行時間</param>
    /// <param name="speed">移動速度</param>
    EnemyFlyApproachNode(float minTime, float maxTime, float speed = 10.0f)
        : m_MinTime(minTime), m_MaxTime(maxTime), m_Speed(speed) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_CurrentTimer = 0.0f;
        m_TargetDuration = 0.0f;
    }

  protected:
    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

    /// <summary>
    /// ノード終了時の処理
    /// </summary>
    void OnExit() override;

  private:
    float m_MinTime;                 // 最小実行時間
    float m_MaxTime;                 // 最大実行時間
    float m_Speed;                   // 移動速度
    float m_CurrentTimer = 0.0f;    // 現在の経過時間
    float m_TargetDuration = 0.0f;  // 目標の実行時間
};

/// <summary>
/// 飛行状態から着地する遷移ノード
/// </summary>
class EnemyFlyToGroundNode : public ContextNode {
  public:
    EnemyFlyToGroundNode() {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
    }

  protected:
    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

  private:
    static constexpr float kGroundLevel = 0.0f; // 地面レベル
};

/// <summary>
/// 弾を発射するアクションノード
/// </summary>
class EnemyShootNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="cooldown">発射後のクールダウン時間</param>
    EnemyShootNode(float cooldown = 1.0f) : m_Cooldown(cooldown) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
        m_HasShot = false;
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    float m_Cooldown;       // 発射後の待機時間(秒)
    float m_Timer = 0.0f;   // 経過時間
    bool m_HasShot = false; // 発射済みフラグ
};

/// <summary>
/// ロックオン状態を切り替えるアクションノード
/// </summary>
class EnemyLockOnNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="lockOn">ロックオンを有効にするか</param>
    EnemyLockOnNode(bool lockOn = true) : m_LockOn(lockOn) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override { BTNode::Reset(); }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    bool m_LockOn; // trueでロックオンON、falseでOFF
};

/// <summary>
/// ロックオン中かどうかをチェックする条件ノード
/// </summary>
class IsEnemyLockOnNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    IsEnemyLockOnNode() {}

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 判定結果</returns>
    NodeStatus OnUpdate() override;
};

/// <summary>
/// コンボを1段階実行するアクションノード
/// </summary>
class EnemyComboStepNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="stepDuration">1段階の実行時間</param>
    /// <param name="comboInterval">コンボ間隔オーバーライド</param>
    EnemyComboStepNode(float stepDuration = 0.5f, float comboInterval = 0.0f)
        : m_StepDuration(stepDuration), m_ComboInterval(comboInterval) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
        m_HasStep = false;
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    float m_StepDuration;  // 1段階の実行時間(秒)
    float m_ComboInterval; // コンボ間隔オーバーライド
    float m_Timer = 0.0f;   // 経過時間
    bool m_HasStep = false; // ステップ実行済みフラグ
};

/// <summary>
/// コンボを全段実行するアクションノード
/// </summary>
class EnemyComboFullNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="stepDuration">1段あたりの実行時間</param>
    /// <param name="maxSteps">最大段数</param>
    /// <param name="comboInterval">コンボ間隔オーバーライド</param>
    EnemyComboFullNode(float stepDuration = 0.5f, int maxSteps = 0, float comboInterval = 0.0f)
        : m_StepDuration(stepDuration), m_MaxSteps(maxSteps), m_ComboInterval(comboInterval) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
        m_StepCount = 0;
        m_WaitingStep = false;
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    float m_StepDuration;  // 1段あたりの実行時間(秒)
    int m_MaxSteps;        // 最大段数
    float m_ComboInterval; // コンボ間隔オーバーライド
    float m_Timer = 0.0f;   // 経過時間
    int m_StepCount = 0;   // 現在の段数
    bool m_WaitingStep = false; // ステップ待ちフラグ
};

/// <summary>
/// 指定弾数を連射するアクションノード
/// </summary>
class EnemyBurstShootNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="interval">発射間隔</param>
    /// <param name="count">発射弾数</param>
    /// <param name="cooldown">全弾発射後のクールダウン</param>
    /// <param name="spreadAngle">拡散角度</param>
    /// <param name="homingMode">ホーミングモードか</param>
    EnemyBurstShootNode(
        float interval = 0.2f,
        int count = 3,
        float cooldown = 0.5f,
        float spreadAngle = 0.0f,
        bool homingMode = false)
        : m_Interval(interval), m_BurstCount(count), m_Cooldown(cooldown), m_SpreadAngle(spreadAngle), m_HomingMode(homingMode) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
        m_ShotsFired = 0;
        m_Phase = Phase::Shooting;
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 1発の弾を発射する
    /// </summary>
    void FireOneBullet();

    /// ===================================================
    /// private variants
    /// ===================================================
    enum class Phase { Shooting,
                       Cooldown };

    float m_Interval;    // 発射間隔(秒)
    int m_BurstCount;    // 連発弾数
    float m_Cooldown;    // 全弾後クールダウン(秒)
    float m_SpreadAngle; // 拡散角度の半角(度)
    bool m_HomingMode;   // true=ロックオン追従, false=拡散固定弾

    float m_Timer = 0.0f;   // 経過時間
    int m_ShotsFired = 0;   // 発射済み弾数
    Phase m_Phase = Phase::Shooting; // 現在のフェーズ
};

/// <summary>
/// エネルギーをチャージするアクションノード
/// </summary>
class EnemyEnergyChargeNode : public ContextNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="chargeRateMultiplier">チャージ速度倍率</param>
    /// <param name="targetRatio">目標エネルギー比率</param>
    EnemyEnergyChargeNode(float chargeRateMultiplier = 1.0f, float targetRatio = 1.0f)
        : m_ChargeRateMultiplier(chargeRateMultiplier), m_TargetRatio(targetRatio) {}

    /// <summary>
    /// 状態をリセット
    /// </summary>
    void Reset() override {
        BTNode::Reset();
        m_Timer = 0.0f;
    }

  protected:
    /// ===================================================
    /// protected method
    /// ===================================================

    /// <summary>
    /// ノード開始時の処理
    /// </summary>
    void OnEnter() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <returns>NodeStatus: 実行結果</returns>
    NodeStatus OnUpdate() override;

    /// <summary>
    /// ノード終了時の処理
    /// </summary>
    void OnExit() override;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================
    float m_ChargeRateMultiplier; // チャージ速度の倍率
    float m_TargetRatio;          // 目標エネルギー比率
    float m_Timer = 0.0f;         // 経過時間
    float m_OriginalRecoveryRate = 0.0f; // 元の回復レート保存用
};

/// <summary>
/// プレイヤーHPが閾値以下かチェックする条件ノード
/// </summary>
class IsPlayerHPLowNode : public ContextNode {
  public:
    IsPlayerHPLowNode(float threshold) : m_Threshold(threshold) {}

  protected:
    NodeStatus OnUpdate() override;

  private:
    float m_Threshold;
};

/// <summary>
/// 自身のエネルギーが閾値以上かチェックする条件ノード
/// </summary>
class IsEnergyHighNode : public ContextNode {
  public:
    IsEnergyHighNode(float threshold) : m_Threshold(threshold) {}

  protected:
    NodeStatus OnUpdate() override;

  private:
    float m_Threshold;
};

/// <summary>
/// 溜め攻撃ノード: 一定時間溜めた後、一斉にホーミング弾を発射する
/// param  : chargeDuration （溜め時間・秒）
/// param2 : burstCount     （発射弾数）
/// param3 : energyCost     （消費エネルギー）
/// </summary>
class EnemyChargeAttackNode : public ContextNode {
  public:
    EnemyChargeAttackNode(float chargeDuration, int burstCount, float /*energyCost*/)
        : m_ChargeDuration(chargeDuration), m_BurstCount(burstCount) {}

    void Reset() override {
        BTNode::Reset();
        m_Timer     = 0.0f;
        m_ShotsFired = 0;
        m_Phase     = Phase::Charge;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;

  private:
    enum class Phase { Charge, Shoot, Cooldown };

    float m_ChargeDuration;
    int   m_BurstCount;
    float m_Timer     = 0.0f;
    int   m_ShotsFired = 0;
    Phase m_Phase     = Phase::Charge;

    static constexpr float kShootInterval = 0.07f;
    static constexpr float kCooldown      = 0.4f;
};

/// <summary>
/// 必殺技ノード: エネルギーを消費してフルコンボ＋大量ホーミング射撃を行う
/// param  : energyCost   （消費エネルギー）
/// param2 : shotCount    （発射弾数）
/// param3 : stepDuration （コンボ1段の時間・秒）
/// </summary>
class EnemyUltimateNode : public ContextNode {
  public:
    EnemyUltimateNode(float /*energyCost*/, int shotCount, float stepDuration)
        : m_ShotCount(shotCount), m_StepDuration(stepDuration) {}

    void Reset() override {
        BTNode::Reset();
        m_Timer      = 0.0f;
        m_StepCount  = 0;
        m_ShotsFired = 0;
        m_WaitingStep = false;
        m_Phase      = Phase::Combo;
    }

  protected:
    void OnEnter() override;
    NodeStatus OnUpdate() override;
    void OnExit() override;

  private:
    enum class Phase { Combo, Shoot, Cooldown };

    int   m_ShotCount;
    float m_StepDuration;
    float m_Timer      = 0.0f;
    int   m_StepCount  = 0;
    int   m_ShotsFired = 0;
    bool  m_WaitingStep = false;
    Phase m_Phase      = Phase::Combo;

    static constexpr int   kMaxComboSteps  = 8;
    static constexpr float kShootInterval  = 0.08f;
    static constexpr float kCooldown       = 0.6f;
};
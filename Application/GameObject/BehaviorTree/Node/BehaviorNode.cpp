#define NOMINMAX
#include "BehaviorNode.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include <iostream>

// ---------------------------------------------------------
// BTNode / CompositeNode 基底
// ---------------------------------------------------------
NodeStatus BTNode::Tick() {
    if (m_Status != NodeStatus::Running)
        OnEnter();
    m_Status = OnUpdate();
    if (m_Status != NodeStatus::Running)
        OnExit();
    return m_Status;
}
void BTNode::AddChild(std::shared_ptr<BTNode> child) {}
void BTNode::SetContext(Enemy *enemy, Player *player) {}
void BTNode::OnEnter() {}
void BTNode::OnExit() {}

void CompositeNode::AddChild(std::shared_ptr<BTNode> child) { m_Children.push_back(child); }
void CompositeNode::SetContext(Enemy *enemy, Player *player) {
    BTNode::SetContext(enemy, player);
    for (auto &child : m_Children)
        child->SetContext(enemy, player);
}
void CompositeNode::OnEnter() { m_CurrentChildIndex = 0; }

// ---------------------------------------------------------
// SequenceNode (Reactive)
// ---------------------------------------------------------
NodeStatus SequenceNode::OnUpdate() {
    // 子ノードが空の場合は失敗
    if (m_Children.empty()) {
        return NodeStatus::Failure;
    }

    // ★修正: 実行していない子ノードのステータスをリセット
    // 毎回先頭(0番目)からチェックし直すことで、
    // 途中で条件(例:距離)がFalseになったら即座に中断するように変更
    for (int i = 0; i < m_Children.size(); ++i) {
        NodeStatus childStatus = m_Children[i]->Tick();

        if (childStatus == NodeStatus::Running) {
            // ★追加: Running以降のノードはリセット
            for (int j = i + 1; j < m_Children.size(); ++j) {
                m_Children[j]->Reset();
            }
            return NodeStatus::Running; // 実行中ならそこで待つ(次回も0番目からチェックされる)
        }
        if (childStatus == NodeStatus::Failure) {
            // ★追加: 失敗した時点で以降のノードをリセット
            for (int j = i + 1; j < m_Children.size(); ++j) {
                m_Children[j]->Reset();
            }
            return NodeStatus::Failure; // 条件不一致などで失敗したら、後続のアクションも中断
        }
    }
    return NodeStatus::Success; // 全て成功
}

// ---------------------------------------------------------
// SelectorNode (Reactive)
// ---------------------------------------------------------
NodeStatus SelectorNode::OnUpdate() {
    // 子ノードが空の場合は失敗
    if (m_Children.empty()) {
        return NodeStatus::Failure;
    }

    // ★修正: 実行していない子ノードのステータスをリセット
    // Selectorも毎回先頭(0番目)から評価する (Reactive)
    for (int i = 0; i < m_Children.size(); ++i) {
        NodeStatus childStatus = m_Children[i]->Tick();

        if (childStatus == NodeStatus::Running) {
            // ★追加: 成功した子ノード以降はリセット
            for (int j = i + 1; j < m_Children.size(); ++j) {
                m_Children[j]->Reset();
            }
            return NodeStatus::Running;
        }
        if (childStatus == NodeStatus::Success) {
            // ★追加: 成功した子ノード以降はリセット
            for (int j = i + 1; j < m_Children.size(); ++j) {
                m_Children[j]->Reset();
            }
            return NodeStatus::Success;
        }
    }
    return NodeStatus::Failure;
}

// ---------------------------------------------------------
// Action (Run / Approach / Attack)
// ---------------------------------------------------------
RunActionNode::RunActionNode() : m_Counter(0), m_Duration(180) {}
void RunActionNode::OnEnter() { m_Counter = 0; }
NodeStatus RunActionNode::OnUpdate() {
    m_Counter++;
    if (m_Counter < m_Duration)
        return NodeStatus::Running;
    return NodeStatus::Success;
}

void TimedActionNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    // ランダムな実行時間を決定
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);
    // 速度をセット (Enemyポインタがあれば)
    if (m_Enemy) {
        m_Enemy->SetMoveSpeed(m_Speed);
        SetupAction(); // 各派生クラスごとの初期化(方向決めなど)
    }
}

NodeStatus TimedActionNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // 具体的な行動を実行
    ExecuteAction();

    // 時間経過チェック (60fps想定)
    m_CurrentTimer += 1.0f / 60.0f;
    if (m_CurrentTimer >= m_TargetDuration) {
        return NodeStatus::Success; // 指定時間動いたら完了
    }
    return NodeStatus::Running;
}

// ★追加: アクション終了時に速度をゼロにイージング
void TimedActionNode::OnExit() {
    if (m_Enemy) {
        // 移動を滑らかに停止
        m_Enemy->StopMovement();
    }
}

// 通常接近 & 高速接近 (中身は同じMoveToTargetだが、速度設定が異なる)
void EnemyApproachNode::ExecuteAction() {
    m_Enemy->MoveToTarget(m_Player->GetWorldPosition());
}

void EnemyDashNode::ExecuteAction() {
    m_Enemy->MoveToTarget(m_Player->GetWorldPosition());
}

// 左右移動
void EnemyStrafeNode::SetupAction() {
    // 開始時に左右どちらに行くかランダムで決める
    int dir = (Random::Range(0, 1) == 0) ? -1 : 1;
    m_Enemy->SetStrafeDirection(dir);
}

void EnemyStrafeNode::ExecuteAction() {
    m_Enemy->MoveStrafe();
}

// 後退
void EnemyRetreatNode::ExecuteAction() {
    m_Enemy->MoveRetreat();
}

// 攻撃 (既存)
NodeStatus EnemyAttackNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;
    m_Enemy->PerformAttack();
    m_Timer += 1.0f / 60.0f;
    if (m_Timer >= 1.0f)
        return NodeStatus::Success;
    return NodeStatus::Running;
}

// ---------------------------------------------------------
// IsPlayerCloseNode (範囲チェック)
// ---------------------------------------------------------
NodeStatus IsPlayerCloseNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    float dist = (m_Enemy->GetWorldPosition() - m_Player->GetWorldPosition()).Length();

    // ★ヒステリシス機能の実装
    // 前回の結果に応じてマージンを設定
    float effectiveMinDist = m_MinDist;
    float effectiveMaxDist = m_MaxDist;

    // 前回成功していた場合、マージンを大きく広げて判定を緩くする
    if (m_LastResult == NodeStatus::Success) {
        effectiveMinDist -= kHysteresisMargin * 2.0f; // より広いマージン
        effectiveMaxDist += kHysteresisMargin * 2.0f;
    }
    // 前回失敗していた場合、マージンを狭めて判定を厳しくする
    else if (m_LastResult == NodeStatus::Failure) {
        effectiveMinDist += kHysteresisMargin;
        effectiveMaxDist -= kHysteresisMargin;
    }

    // 最小・最大距離内にいるかチェック
    bool isInRange = (dist >= effectiveMinDist && dist <= effectiveMaxDist);

    NodeStatus newResult = isInRange ? NodeStatus::Success : NodeStatus::Failure;

    // 結果が変わった場合
    if (newResult != m_LastResult) {
        // 成功から失敗に変わる場合、保持時間をチェック
        if (m_LastResult == NodeStatus::Success && m_StableTimer < kSuccessHoldTime) {
            // まだ保持時間が経過していないので、成功を維持
            m_StableTimer += 1.0f / 60.0f;
            return NodeStatus::Success;
        }

        // タイマーをリセットして新しい結果に更新
        m_StableTimer = 0.0f;
        m_LastResult = newResult;
    } else {
        // 同じ結果が続いている場合、タイマーを増やす
        m_StableTimer += 1.0f / 60.0f;
    }

    // 最小安定時間が経過していない場合、前回の結果を返す
    if (m_StableTimer < kMinStableTime && m_LastResult != NodeStatus::Idle) {
        return m_LastResult;
    }

    return newResult;
}

NodeStatus IsHealthLowNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;
    // float max = m_Enemy->GetMaxHP();
    // float current = m_Enemy->GetHP();
    // if (max > 0 && (current / max) <= m_ThresholdPercentage) return NodeStatus::Success;
    return NodeStatus::Failure;
}

// =========================================================
// ★新規実装: 地上判定ノード
// =========================================================
NodeStatus IsGroundedNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 地面にいる場合は成功、空中にいる場合は失敗
    return m_Enemy->GetIsGrounded() ? NodeStatus::Success : NodeStatus::Failure;
}

// =========================================================
// ★新規実装: 空中判定ノード
// =========================================================
NodeStatus IsAirborneNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 空中にいる場合は成功、地面にいる場合は失敗
    return !m_Enemy->GetIsGrounded() ? NodeStatus::Success : NodeStatus::Failure;
}

void RandomSelectorNode::OnEnter() {
    m_SelectedChildIndex = -1;
    if (m_Children.empty())
        return;

    // 1. 全ての子ノードの重み合計を計算する
    float totalWeight = 0.0f;
    std::vector<float> weights;

    for (const auto &child : m_Children) {
        float w = 1.0f; // デフォルトの重み

        // もし子が WeightDecoratorNode なら、その設定値を採用
        // dynamic_pointer_cast で型を確認する
        auto weightNode = std::dynamic_pointer_cast<WeightDecoratorNode>(child);
        if (weightNode) {
            w = weightNode->GetWeight();
        }

        // 重みが0未満にならないようにケア
        if (w < 0.0f)
            w = 0.0f;

        weights.push_back(w);
        totalWeight += w;
    }

    // 2. 乱数で選択する
    // ランダムな値 (0 ~ totalWeight) を取得
    // ※ myMath.h や random.h の仕様に合わせて調整してください。
    //   ここでは Random::Range(min, max) がある前提で書きます。
    float randomValue = Random::Range(0.0f, totalWeight);

    // 3. どのノードに当たったか判定
    float currentSum = 0.0f;
    for (int i = 0; i < weights.size(); ++i) {
        currentSum += weights[i];
        if (randomValue <= currentSum) {
            m_SelectedChildIndex = i;
            break;
        }
    }

    // 計算誤差などで決まらなかった場合は最後の子にする
    if (m_SelectedChildIndex == -1 && !m_Children.empty()) {
        m_SelectedChildIndex = (int)m_Children.size() - 1;
    }
}

NodeStatus RandomSelectorNode::OnUpdate() {
    // 子ノードが空の場合は失敗
    if (m_Children.empty()) {
        return NodeStatus::Failure;
    }

    // 選択された子ノードだけを実行する
    if (m_SelectedChildIndex < 0 || m_SelectedChildIndex >= m_Children.size()) {
        return NodeStatus::Failure;
    }

    NodeStatus result = m_Children[m_SelectedChildIndex]->Tick();

    // 実行中ならRunningを返し、終了(Success/Failure)したらそのまま結果を返す
    // Sequenceのように「次へ」とは行かず、今回はこれだけで終わり
    return result;
}

void EnemyIdleNode::Reset() {
    BTNode::Reset();
    m_Timer = 0.0f;
}

void EnemyIdleNode::OnEnter() {
    BTNode::Reset();
    m_Timer = 0.0f;
}

// =========================================================
// ★新規実装: ジャンプノード
// =========================================================
void EnemyJumpNode::OnEnter() {
    m_JumpExecuted = false;

    if (!m_Enemy)
        return;

    // ★地面にいる場合のみジャンプ実行
    if (m_Enemy->GetIsGrounded()) {
        // プレイヤーの PlayerStateJump::Enter() を参考
        // 設定されたジャンプ力を使用
        m_Enemy->SetVerticalVelocity(m_JumpPower);
        m_Enemy->SetIsGrounded(false);

        // ★重要: 重力加速度を設定（これがないと落下しない）
        m_Enemy->SetVerticalAcceleration(m_Enemy->GetFallSpeed());

        m_JumpExecuted = true; // ジャンプ成功
    }
}

NodeStatus EnemyJumpNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // ジャンプが実行された場合は成功
    if (m_JumpExecuted) {
        return NodeStatus::Success;
    }

    // 空中にいてジャンプできなかった場合は失敗
    return NodeStatus::Failure;
}

// =========================================================
// ★新規実装: ジャンプから飛行状態への遷移ノード
// =========================================================
void EnemyJumpToFlyNode::OnEnter() {
    m_ElapsedTime = 0.0f;

    if (!m_Enemy)
        return;

    // ジャンプ中の落下加速度を設定 (PlayerStateAir::Enter を参考)
    m_Enemy->SetVerticalAcceleration(m_Enemy->GetFallSpeed());
}

NodeStatus EnemyJumpToFlyNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // 横方向の移動を継続 (プレイヤーに向かって移動など)
    m_Enemy->Move();

    // 重力による落下速度の更新
    float currentVelocityY = m_Enemy->GetVerticalVelocity();
    float acceleration = m_Enemy->GetVerticalAcceleration();
    m_Enemy->SetVerticalVelocity(currentVelocityY + acceleration * (1.0f / 60.0f));

    // 方向更新
    m_Enemy->DirectionUpdate();

    m_ElapsedTime += 1.0f / 60.0f;

    // 一定時間内であれば飛行状態に遷移可能
    // この判定は外部の Sequence や Selector で制御することを想定
    if (m_ElapsedTime >= kFlyTransitionTime) {
        return NodeStatus::Success; // 時間経過したので成功
    }

    return NodeStatus::Running;
}

// =========================================================
// ★新規実装: 飛行状態 - 上昇動作
// =========================================================
void EnemyFlyAscendNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);

    if (!m_Enemy)
        return;

    // 飛行開始時に加速度と速度をリセット
    m_Enemy->SetVerticalAcceleration(0.0f);
    m_Enemy->SetVerticalVelocity(0.0f);
}

NodeStatus EnemyFlyAscendNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // PlayerStateFlyMove の上昇処理を参考
    float currentVelocityY = m_Enemy->GetVerticalVelocity();

    // 設定された速度を目標として上昇加速
    currentVelocityY = std::min(currentVelocityY + kFlyAcceleration * (1.0f / 60.0f), m_Speed);
    m_Enemy->SetVerticalVelocity(currentVelocityY);

    // 横方向の移動も継続 (プレイヤーに向かって移動など)
    m_Enemy->Move();

    // 方向更新
    m_Enemy->DirectionUpdate();

    // 時間経過チェック
    m_CurrentTimer += 1.0f / 60.0f;
    if (m_CurrentTimer >= m_TargetDuration) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyFlyAscendNode::OnExit() {
    if (!m_Enemy)
        return;

    // 上昇終了時に速度を減衰させる (急停止を避ける)
    float currentVelocityY = m_Enemy->GetVerticalVelocity();
    m_Enemy->SetVerticalVelocity(currentVelocityY * 0.7f);
}

// =========================================================
// ★新規実装: 飛行状態 - 下降動作
// =========================================================
void EnemyFlyDescendNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);

    if (!m_Enemy)
        return;

    m_Enemy->SetVerticalAcceleration(0.0f);
    m_Enemy->SetVerticalVelocity(0.0f);
}

NodeStatus EnemyFlyDescendNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // PlayerStateFlyMove の下降処理を参考
    float currentVelocityY = m_Enemy->GetVerticalVelocity();

    // 設定された速度を目標として下降加速（負の値）
    currentVelocityY = std::max(currentVelocityY - kFlyAcceleration * (1.0f / 60.0f), -m_Speed);
    m_Enemy->SetVerticalVelocity(currentVelocityY);

    // 横方向の移動も継続
    m_Enemy->Move();

    // 方向更新
    m_Enemy->DirectionUpdate();

    // 時間経過チェック
    m_CurrentTimer += 1.0f / 60.0f;
    if (m_CurrentTimer >= m_TargetDuration) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyFlyDescendNode::OnExit() {
    if (!m_Enemy)
        return;

    // 下降終了時に速度を減衰させる
    float currentVelocityY = m_Enemy->GetVerticalVelocity();
    m_Enemy->SetVerticalVelocity(currentVelocityY * 0.7f);
}

// =========================================================
// ★新規実装: 飛行から地上への遷移ノード (着地)
// =========================================================
void EnemyFlyToGroundNode::OnEnter() {
    // 特に初期化処理は不要
}

NodeStatus EnemyFlyToGroundNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 地面レベル以下に到達したか確認
    if (m_Enemy->GetLocalPosition().y <= kGroundLevel) {
        // 着地処理
        m_Enemy->SetIsGrounded(true);
        m_Enemy->SetVerticalVelocity(0.0f);
        m_Enemy->SetVerticalAcceleration(0.0f);

        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}
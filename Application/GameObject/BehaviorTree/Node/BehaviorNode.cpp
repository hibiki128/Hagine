#include "BehaviorNode.h"
#include <iostream>
// 環境に合わせてパスを修正してください
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"

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
// ★修正: SequenceNode (Reactive)
// ---------------------------------------------------------
NodeStatus SequenceNode::OnUpdate() {
    // 毎回先頭(0番目)からチェックし直すことで、
    // 途中で条件(例:距離)がFalseになったら即座に中断するように変更
    for (int i = 0; i < m_Children.size(); ++i) {
        NodeStatus childStatus = m_Children[i]->Tick();

        if (childStatus == NodeStatus::Running) {
            return NodeStatus::Running; // 実行中ならそこで待つ（次回も0番目からチェックされる）
        }
        if (childStatus == NodeStatus::Failure) {
            return NodeStatus::Failure; // 条件不一致などで失敗したら、後続のアクションも中断
        }
    }
    return NodeStatus::Success; // 全て成功
}

// ---------------------------------------------------------
// SelectorNode
// ---------------------------------------------------------
NodeStatus SelectorNode::OnUpdate() {
    // Selectorも同様にReactiveにするか、状態を持つかは設計次第ですが、
    // ここでは従来の「順番に試す」方式を維持します
    while (m_CurrentChildIndex < m_Children.size()) {
        NodeStatus childStatus = m_Children[m_CurrentChildIndex]->Tick();

        if (childStatus == NodeStatus::Running)
            return NodeStatus::Running;
        if (childStatus == NodeStatus::Success)
            return NodeStatus::Success;

        m_CurrentChildIndex++;
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

NodeStatus EnemyApproachNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;
    m_Enemy->MoveToTarget(m_Player->GetWorldPosition());

    // 接近完了判定 (例: 2m以内)
    float dist = (m_Enemy->GetWorldPosition() - m_Player->GetWorldPosition()).Length();
    if (dist <= 2.0f)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

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
// ★修正: IsPlayerCloseNode (範囲チェック)
// ---------------------------------------------------------
NodeStatus IsPlayerCloseNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    float dist = (m_Enemy->GetWorldPosition() - m_Player->GetWorldPosition()).Length();

    // 最小距離 <= 現在距離 <= 最大距離 なら成功
    if (dist >= m_MinDist && dist <= m_MaxDist) {
        return NodeStatus::Success;
    }
    return NodeStatus::Failure;
}

NodeStatus IsHealthLowNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;
    // float max = m_Enemy->GetMaxHP();
    // float current = m_Enemy->GetHP();
    // if (max > 0 && (current / max) <= m_ThresholdPercentage) return NodeStatus::Success;
    return NodeStatus::Failure;
}
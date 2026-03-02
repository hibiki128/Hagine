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

// =========================================================
// ★新規実装: プレイヤーステート判定ノード
// =========================================================
NodeStatus IsPlayerStateNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // プレイヤーの現在のステート名を取得
    std::string currentState = m_Player->GetCurrentStateName();

    // 指定されたステートと一致すれば成功、違えば失敗
    return (currentState == m_StateName) ? NodeStatus::Success : NodeStatus::Failure;
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
// ★修正: ジャンプノード - 着地するまで実行し続ける
// =========================================================
void EnemyJumpNode::OnEnter() {
    m_JumpExecuted = false;

    if (!m_Enemy)
        return;

    // ★地面にいる場合のみジャンプ実行
    if (m_Enemy->GetIsGrounded()) {
        // ジャンプ力を設定
        m_Enemy->SetVerticalVelocity(m_JumpPower);
        m_Enemy->SetIsGrounded(false);

        // ★重要: 重力加速度を負の値に設定（下向きの加速度で落下させる）
        float fallSpeed = m_Enemy->GetFallSpeed();
        m_Enemy->SetVerticalAcceleration(fallSpeed > 0.0f ? -fallSpeed : fallSpeed);

        m_JumpExecuted = true; // ジャンプ成功
    }
}

NodeStatus EnemyJumpNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // ジャンプが実行されなかった場合（空中にいた場合など）は失敗
    if (!m_JumpExecuted) {
        return NodeStatus::Failure;
    }

    // ★修正: 着地するまでRunningを返し続ける
    // 地面に着いたら成功を返す
    if (m_Enemy->GetIsGrounded()) {
        return NodeStatus::Success;
    }

    // まだ空中にいる場合はRunningを返す
    return NodeStatus::Running;
}

// =========================================================
// ジャンプから飛行状態への遷移ノード
// ジャンプ→自前の減速で最高到達点→そのまま飛行停止
// =========================================================
void EnemyJumpToFlyNode::OnEnter() {
    m_ElapsedTime = 0.0f;

    if (!m_Enemy)
        return;

    // ジャンプ速度を設定
    m_Enemy->SetVerticalVelocity(m_JumpPower);
    m_Enemy->SetIsGrounded(false);

    // 即座に飛行フラグON → Enemy::Update()の重力加算を無効化
    // 代わりにOnUpdate()内で自前で減速させる
    m_Enemy->SetIsFlying(true);
    m_Enemy->SetVerticalAcceleration(0.0f);
}

NodeStatus EnemyJumpToFlyNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    m_Enemy->Move();
    m_Enemy->DirectionUpdate();

    float velY = m_Enemy->GetVerticalVelocity();

    if (velY > 0.0f) {
        // 上昇中：重力相当の値で自前減速（Enemy::Updateの重力はisFlying_でブロック済み）
        float fallSpeed = m_Enemy->GetFallSpeed();
        float newVelY = velY - fallSpeed * (1.0f / 60.0f);
        if (newVelY < 0.0f)
            newVelY = 0.0f; // 0より下には行かせない
        m_Enemy->SetVerticalVelocity(newVelY);
        return NodeStatus::Running;
    }

    // velocity.y == 0 = 最高到達点に到達 → 完全停止して飛行待機
    m_Enemy->SetVerticalVelocity(0.0f);

    m_ElapsedTime += 1.0f / 60.0f;
    if (m_ElapsedTime >= kFlyTransitionTime) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyJumpToFlyNode::OnExit() {
    // 飛行状態は維持したまま次のノードへ
}

// =========================================================
// 飛行状態 - 上昇動作
// =========================================================
void EnemyFlyAscendNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);

    if (!m_Enemy)
        return;

    // 飛行フラグを確実にON（重力無効）
    m_Enemy->SetIsFlying(true);
    m_Enemy->SetVerticalAcceleration(0.0f);
    // 上昇速度を直接設定
    m_Enemy->SetVerticalVelocity(m_Speed);
}

NodeStatus EnemyFlyAscendNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // 飛行中は加速度ではなく速度を直接維持（重力無効なので加速不要）
    m_Enemy->SetVerticalVelocity(m_Speed);

    m_Enemy->Move();
    m_Enemy->DirectionUpdate();

    m_CurrentTimer += 1.0f / 60.0f;
    if (m_CurrentTimer >= m_TargetDuration) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyFlyAscendNode::OnExit() {
    if (!m_Enemy)
        return;
    // 上昇終了：速度を0にして空中停止（飛行フラグは維持）
    m_Enemy->SetVerticalVelocity(0.0f);
    m_Enemy->SetVerticalAcceleration(0.0f);
}

// =========================================================
// 飛行状態 - 下降動作
// =========================================================
void EnemyFlyDescendNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);

    if (!m_Enemy)
        return;

    // 飛行フラグを維持（重力無効のまま下降速度を直接設定）
    m_Enemy->SetIsFlying(true);
    m_Enemy->SetVerticalAcceleration(0.0f);
    m_Enemy->SetVerticalVelocity(-m_Speed);
}

NodeStatus EnemyFlyDescendNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // 飛行中は速度を直接維持
    m_Enemy->SetVerticalVelocity(-m_Speed);

    m_Enemy->Move();
    m_Enemy->DirectionUpdate();

    m_CurrentTimer += 1.0f / 60.0f;
    if (m_CurrentTimer >= m_TargetDuration) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyFlyDescendNode::OnExit() {
    if (!m_Enemy)
        return;
    // 下降終了：速度を0にして空中停止（飛行フラグは維持）
    m_Enemy->SetVerticalVelocity(0.0f);
    m_Enemy->SetVerticalAcceleration(0.0f);
}

// =========================================================
// 飛行から地上への遷移ノード (着地)
// =========================================================
void EnemyFlyToGroundNode::OnEnter() {
    if (!m_Enemy)
        return;

    // 飛行フラグをOFF → 重力が復活する
    m_Enemy->SetIsFlying(false);
    float fallSpeed = m_Enemy->GetFallSpeed();
    m_Enemy->SetVerticalAcceleration(fallSpeed > 0.0f ? -fallSpeed : fallSpeed);
}

NodeStatus EnemyFlyToGroundNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // Enemy::Update()の重力処理で自然落下する
    // 地面に着いたら（CollisionGroundで isGrounded_=true になる）
    if (m_Enemy->GetIsGrounded()) {
        m_Enemy->SetVerticalVelocity(0.0f);
        m_Enemy->SetVerticalAcceleration(0.0f);
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

// =========================================================
// EnemyShootNode: 弾発射アクション
// =========================================================
void EnemyShootNode::OnEnter() {
    m_Timer = 0.0f;
    m_HasShot = false;
}

NodeStatus EnemyShootNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 発射はまだの場合、即座に1発撃つ
    if (!m_HasShot) {
        m_Enemy->Shot();
        m_HasShot = true;
    }

    // クールダウン待機
    m_Timer += 1.0f / 60.0f;
    if (m_Timer >= m_Cooldown) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

// =========================================================
// EnemyLockOnNode: ロックオン切り替えアクション
// =========================================================
NodeStatus EnemyLockOnNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    m_Enemy->SetIsLockOn(m_LockOn);
    return NodeStatus::Success; // 即座に完了
}

// =========================================================
// IsEnemyLockOnNode: ロックオン中か条件チェック
// =========================================================
NodeStatus IsEnemyLockOnNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    return m_Enemy->GetIsLockOn() ? NodeStatus::Success : NodeStatus::Failure;
}

// =========================================================
// =========================================================
// EnemyComboStepNode: コンボ1段階実行
// =========================================================
void EnemyComboStepNode::OnEnter() {
    m_Timer = 0.0f;
    m_HasStep = false;
}

NodeStatus EnemyComboStepNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    if (!m_HasStep) {
        m_Enemy->SetComboAttack(true);
        m_HasStep = true;
    }

    // コンボ間隔オーバーライドがあればそちら、なければモーション待機時間
    float waitTime = (m_ComboInterval > 0.0f) ? m_ComboInterval : m_StepDuration;
    m_Timer += 1.0f / 60.0f;
    if (m_Timer >= waitTime)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

// =========================================================
// EnemyComboFullNode: コンボ全段実行
// =========================================================
void EnemyComboFullNode::OnEnter() {
    m_Timer = 0.0f;
    m_StepCount = 0;
    m_WaitingStep = false;
}

NodeStatus EnemyComboFullNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 最大段数の上限 (0=コンボ全段数)
    int comboLength = m_Enemy->GetPunchComboLength();
    int limit = (m_MaxSteps > 0) ? m_MaxSteps : comboLength;

    if (m_StepCount >= limit)
        return NodeStatus::Success;

    m_Timer += 1.0f / 60.0f;

    if (!m_WaitingStep) {
        m_Enemy->SetComboAttack(true);
        m_StepCount++;
        m_WaitingStep = true;
        m_Timer = 0.0f;
    }

    float waitTime = (m_ComboInterval > 0.0f) ? m_ComboInterval : m_StepDuration;
    if (m_Timer >= waitTime) {
        m_WaitingStep = false;
        // IsComboActive()でコンボがリセットされていたら終了
        if (!m_Enemy->IsPunchComboActive())
            return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

// =========================================================
// EnemyBurstShootNode: 連射（拡散・ホーミング対応）
// =========================================================

void EnemyBurstShootNode::FireOneBullet() {
    if (!m_Enemy)
        return;

    if (m_HomingMode) {
        // ホーミング：既存Shot()（ロックオン自動判定）をそのまま使う
        m_Enemy->Shot();
        return;
    }

    // 拡散なし → 敵の正面方向に直進
    if (m_SpreadAngle <= 0.0f) {
        m_Enemy->ShotWithDirection(m_Enemy->GetForward(), false);
        return;
    }

    // ±SpreadAngle/2 の範囲でYaw(水平)・Pitch(垂直)をランダムに振る
    float halfAngle = m_SpreadAngle * 0.5f;
    float yawDeg = Random::Range(-halfAngle, halfAngle);
    float pitchDeg = Random::Range(-halfAngle * 0.3f, halfAngle * 0.3f); // 垂直は少し抑える

    float yawRad = yawDeg * (3.14159265f / 180.0f);
    float pitchRad = pitchDeg * (3.14159265f / 180.0f);

    Vector3 forward = m_Enemy->GetForward();
    Vector3 up = m_Enemy->GetUp();
    Vector3 right = m_Enemy->GetRight();

    // Yaw → Pitch の順に回転してバラけた方向を計算
    Vector3 yawed = forward * std::cos(yawRad) + right * std::sin(yawRad);
    Vector3 dir = yawed * std::cos(pitchRad) + up * std::sin(pitchRad);

    float len = dir.Length();
    if (len > 0.001f)
        dir = dir / len;

    m_Enemy->ShotWithDirection(dir, false);
}

void EnemyBurstShootNode::OnEnter() {
    m_Timer = 0.0f;
    m_ShotsFired = 0;
    m_Phase = Phase::Shooting;

    // 最初の1発を即座に発射
    if (m_Enemy && m_BurstCount > 0) {
        FireOneBullet();
        m_ShotsFired = 1;

        if (m_ShotsFired >= m_BurstCount) {
            m_Phase = Phase::Cooldown;
        }
    }
}

NodeStatus EnemyBurstShootNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    m_Timer += 1.0f / 60.0f;

    if (m_Phase == Phase::Shooting) {
        if (m_Timer >= m_Interval) {
            m_Timer = 0.0f;

            if (m_ShotsFired < m_BurstCount) {
                FireOneBullet();
                m_ShotsFired++;
            }

            if (m_ShotsFired >= m_BurstCount) {
                m_Phase = Phase::Cooldown;
            }
        }
        return NodeStatus::Running;
    }

    // クールダウン待機
    if (m_Timer >= m_Cooldown)
        return NodeStatus::Success;

    return NodeStatus::Running;
}
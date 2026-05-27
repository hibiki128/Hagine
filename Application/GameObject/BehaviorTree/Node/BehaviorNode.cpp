#define NOMINMAX
#include "BehaviorNode.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include <iostream>
#include"Frame.h"

// ---------------------------------------------------------
// BTNode / CompositeNode 基底
// ---------------------------------------------------------
NodeStatus BTNode::Tick() {
    // 初回実行時、または Running 以外から再開された時に OnEnter を呼ぶ
    if (m_Status != NodeStatus::Running)
        OnEnter();

    // 更新処理を実行しステータスを更新
    m_Status = OnUpdate();

    // 終了（Success または Failure）した時に OnExit を呼ぶ
    if (m_Status != NodeStatus::Running)
        OnExit();

    return m_Status;
}
void BTNode::AddChild(std::shared_ptr<BTNode> child) {}
void BTNode::SetContext(Enemy *enemy, Player *player) {}
void BTNode::OnEnter() {}
void BTNode::OnExit() {}

void CompositeNode::AddChild(std::shared_ptr<BTNode> child) {
    // 子ノードリストに追加
    m_Children.push_back(child);
}

void CompositeNode::SetContext(Enemy *enemy, Player *player) {
    BTNode::SetContext(enemy, player);
    // 全ての子ノードにコンテキストを伝播
    for (auto &child : m_Children)
        child->SetContext(enemy, player);
}

void CompositeNode::OnEnter() {
    // 最初の子から実行するようにインデックスをリセット
    m_CurrentChildIndex = 0;
}

// ---------------------------------------------------------
// SequenceNode (Reactive)
// ---------------------------------------------------------
NodeStatus SequenceNode::OnUpdate() {
    if (m_Children.empty()) {
        return NodeStatus::Failure;
    }

    // 毎フレーム先頭から順に子ノードを評価する (Reactive)
    for (int i = 0; i < m_Children.size(); ++i) {
        NodeStatus childStatus = m_Children[i]->Tick();

        if (childStatus == NodeStatus::Running) {
            // Running 中のノードより後のノードはリセットしておく
            for (int j = i + 1; j < m_Children.size(); ++j) {
                m_Children[j]->Reset();
            }
            return NodeStatus::Running;
        }

        if (childStatus == NodeStatus::Failure) {
            // 一つでも失敗した時点で以降のノードをリセットして失敗を返す
            for (int j = i + 1; j < m_Children.size(); ++j) {
                m_Children[j]->Reset();
            }
            return NodeStatus::Failure;
        }
        // Success の場合は次のループで次の子ノードを評価
    }

    // 全ての子ノードが Success だった場合のみ Success を返す
    return NodeStatus::Success;
}

// ---------------------------------------------------------
// SequenceOnceNode (Non-Reactive)
// ---------------------------------------------------------
NodeStatus SequenceOnceNode::OnUpdate() {
    if (m_Children.empty())
        return NodeStatus::Failure;

    // 前回の続きから子ノードを順次実行する
    while (m_CurrentChildIndex < (int)m_Children.size()) {
        NodeStatus status = m_Children[m_CurrentChildIndex]->Tick();

        // 実行中の場合はそのノードで止まる
        if (status == NodeStatus::Running)
            return NodeStatus::Running;

        // 失敗した場合は即座に終了
        if (status == NodeStatus::Failure)
            return NodeStatus::Failure;

        // 成功した場合は次の子ノードへ進む
        m_CurrentChildIndex++;
    }

    return NodeStatus::Success;
}

// ---------------------------------------------------------
// SelectorNode (Reactive)
// ---------------------------------------------------------
NodeStatus SelectorNode::OnUpdate() {
    if (m_Children.empty()) {
        return NodeStatus::Failure;
    }

    // 毎フレーム先頭から順に子ノードを評価する (Reactive)
    for (int i = 0; i < m_Children.size(); ++i) {
        NodeStatus childStatus = m_Children[i]->Tick();

        if (childStatus == NodeStatus::Running) {
            // 実行中のノード以降はリセット
            for (int j = i + 1; j < m_Children.size(); ++j) {
                m_Children[j]->Reset();
            }
            return NodeStatus::Running;
        }

        if (childStatus == NodeStatus::Success) {
            // 一つでも成功した時点で以降のノードをリセットして成功を返す
            for (int j = i + 1; j < m_Children.size(); ++j) {
                m_Children[j]->Reset();
            }
            return NodeStatus::Success;
        }
        // Failure の場合は次のループで次の子ノードを試す
    }

    return NodeStatus::Failure;
}

// ---------------------------------------------------------
// RunActionNode
// ---------------------------------------------------------
RunActionNode::RunActionNode() : m_Counter(0), m_Duration(180) {}

void RunActionNode::OnEnter() {
    // カウンターをリセット
    m_Counter = 0;
}

NodeStatus RunActionNode::OnUpdate() {
    m_Counter++;
    // 指定されたデュレーションが経過するまで Running を返す
    if (m_Counter < m_Duration)
        return NodeStatus::Running;
    return NodeStatus::Success;
}

void TimedActionNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    // 設定された最小時間と最大時間の範囲からランダムに実行時間を決定
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);

    if (m_Enemy) {
        // 敵の移動速度を設定
        m_Enemy->SetMoveSpeed(m_Speed);
        // 各派生クラス固有の初期化（方向決定など）を実行
        SetupAction();
    }
}

NodeStatus TimedActionNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // 各派生クラスの具体的な行動を実行
    ExecuteAction();

    // 経過時間を加算
    m_CurrentTimer += 1.0f / 60.0f;
    // 目標時間に達したら成功を返す
    if (m_CurrentTimer >= m_TargetDuration) {
        return NodeStatus::Success;
    }
    return NodeStatus::Running;
}

void TimedActionNode::OnExit() {
    if (m_Enemy) {
        // アクション終了時に移動を停止
        m_Enemy->StopMovement();
    }
}

void EnemyApproachNode::ExecuteAction() {
    // プレイヤーの位置に向かって移動
    m_Enemy->MoveToTarget(m_Player->GetWorldPosition());
}

void EnemyDashNode::ExecuteAction() {
    // 高速でプレイヤーの位置に向かって移動（速度は OnEnter で設定済み）
    m_Enemy->MoveToTarget(m_Player->GetWorldPosition());
}

void EnemyStrafeNode::SetupAction() {
    // 開始時に左右どちらに回り込むかをランダムに決定
    int dir = (Random::Range(0, 1) == 0) ? -1 : 1;
    m_Enemy->SetStrafeDirection(dir);
}

void EnemyStrafeNode::ExecuteAction() {
    // 設定された方向に回り込み移動を実行
    m_Enemy->MoveStrafe();
}

void EnemyRetreatNode::ExecuteAction() {
    // プレイヤーから離れる方向に移動
    m_Enemy->MoveRetreat();
}

NodeStatus EnemyAttackNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 攻撃動作を実行
    m_Enemy->PerformAttack();
    // 攻撃演出時間を加算（固定1秒）
    m_Timer += 1.0f / 60.0f;
    if (m_Timer >= 1.0f)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

// ---------------------------------------------------------
// IsPlayerCloseNode
// ---------------------------------------------------------
NodeStatus IsPlayerCloseNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // 敵とプレイヤーの距離を算出
    float dist = (m_Enemy->GetWorldPosition() - m_Player->GetWorldPosition()).Length();

    // ヒステリシスを考慮した判定用距離を設定
    float effectiveMinDist = m_MinDist;
    float effectiveMaxDist = m_MaxDist;

    if (m_LastResult == NodeStatus::Success) {
        // 前回成功していたらマージンを広げて状態を維持しやすくする
        effectiveMinDist -= kHysteresisMargin * 2.0f;
        effectiveMaxDist += kHysteresisMargin * 2.0f;
    } else if (m_LastResult == NodeStatus::Failure) {
        // 前回失敗していたらマージンを狭めて判定を厳しくする
        effectiveMinDist += kHysteresisMargin;
        effectiveMaxDist -= kHysteresisMargin;
    }

    // 判定範囲内にいるかチェック
    bool isInRange = (dist >= effectiveMinDist && dist <= effectiveMaxDist);
    NodeStatus newResult = isInRange ? NodeStatus::Success : NodeStatus::Failure;

    // 判定結果が変わった場合の処理
    if (newResult != m_LastResult) {
        // 成功から失敗に変わる際、保持時間をチェックしてチャタリングを防止
        if (m_LastResult == NodeStatus::Success && m_StableTimer < kSuccessHoldTime) {
            m_StableTimer += 1.0f / 60.0f;
            return NodeStatus::Success;
        }

        m_StableTimer = 0.0f;
        m_LastResult = newResult;
    } else {
        m_StableTimer += 1.0f / 60.0f;
    }

    // 最小安定時間を経過するまでは前回の結果を維持
    if (m_StableTimer < kMinStableTime && m_LastResult != NodeStatus::Idle) {
        return m_LastResult;
    }

    return newResult;
}

NodeStatus IsHealthLowNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // HP比率を計算して閾値以下か判定
    float maxHP = m_Enemy->GetMaxHP();
    float currentHP = m_Enemy->GetHP();
    if (maxHP > 0 && (currentHP / maxHP) <= m_ThresholdPercentage)
        return NodeStatus::Success;

    return NodeStatus::Failure;
}

NodeStatus IsEnergyLowNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // エネルギー比率を計算して閾値以下か判定
    float maxEnergy = m_Enemy->GetMaxEnergy();
    float currentEnergy = m_Enemy->GetEnergy();
    if (maxEnergy > 0.0f && (currentEnergy / maxEnergy) <= m_ThresholdPercentage)
        return NodeStatus::Success;

    return NodeStatus::Failure;
}

NodeStatus IsGroundedNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 接地フラグをそのまま返す
    return m_Enemy->GetIsGrounded() ? NodeStatus::Success : NodeStatus::Failure;
}

NodeStatus IsAirborneNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 非接地（空中）状態か判定
    return !m_Enemy->GetIsGrounded() ? NodeStatus::Success : NodeStatus::Failure;
}

NodeStatus IsPlayerStateNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // プレイヤーの現在のステート名を取得して一致するか判定
    std::string currentState = m_Player->GetCurrentStateName();
    return (currentState == m_StateName) ? NodeStatus::Success : NodeStatus::Failure;
}

void RandomSelectorNode::OnEnter() {
    m_SelectedChildIndex = -1;
    if (m_Children.empty())
        return;

    // 重みの合計を算出
    float totalWeight = 0.0f;
    std::vector<float> weights;

    for (const auto &child : m_Children) {
        float w = 1.0f;
        auto weightNode = std::dynamic_pointer_cast<WeightDecoratorNode>(child);
        if (weightNode) {
            w = weightNode->GetWeight();
        }
        if (w < 0.0f) w = 0.0f;

        weights.push_back(w);
        totalWeight += w;
    }

    // 算出した重みに基づいてランダムに子ノードを選択
    float randomValue = Random::Range(0.0f, totalWeight);
    float currentSum = 0.0f;
    for (int i = 0; i < weights.size(); ++i) {
        currentSum += weights[i];
        if (randomValue <= currentSum) {
            m_SelectedChildIndex = i;
            break;
        }
    }

    // 誤差等で決まらなかった場合は最後の要素を選択
    if (m_SelectedChildIndex == -1 && !m_Children.empty()) {
        m_SelectedChildIndex = (int)m_Children.size() - 1;
    }
}

NodeStatus RandomSelectorNode::OnUpdate() {
    if (m_Children.empty()) {
        return NodeStatus::Failure;
    }

    if (m_SelectedChildIndex < 0 || m_SelectedChildIndex >= m_Children.size()) {
        return NodeStatus::Failure;
    }

    // 選択された子ノードのみを実行
    return m_Children[m_SelectedChildIndex]->Tick();
}

void EnemyIdleNode::Reset() {
    BTNode::Reset();
    m_Timer = 0.0f;
}

void EnemyIdleNode::OnEnter() {
    BTNode::Reset();
    m_Timer = 0.0f;
}

void EnemyJumpNode::OnEnter() {
    m_JumpExecuted = false;

    if (!m_Enemy)
        return;

    // 地面にいる時のみジャンプを実行
    if (m_Enemy->GetIsGrounded()) {
        // 上向きの初速を与える
        m_Enemy->SetVerticalVelocity(m_JumpPower);
        m_Enemy->SetIsGrounded(false);

        // 重力を下向きに設定
        float fallSpeed = m_Enemy->GetFallSpeed();
        m_Enemy->SetVerticalAcceleration(fallSpeed > 0.0f ? -fallSpeed : fallSpeed);

        m_JumpExecuted = true;
    }
}

NodeStatus EnemyJumpNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    if (!m_JumpExecuted) {
        return NodeStatus::Failure;
    }

    // 着地するまで Running を返し、着地したら Success
    if (m_Enemy->GetIsGrounded()) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyJumpToFlyNode::OnEnter() {
    m_ElapsedTime = 0.0f;

    if (!m_Enemy)
        return;

    // 上向きの速度を与えてジャンプ開始
    m_Enemy->SetVerticalVelocity(m_JumpPower);
    m_Enemy->SetIsGrounded(false);

    // 飛行状態へ移行（重力を自前で制御するためフラグを立てる）
    m_Enemy->SetIsFlying(true);
    m_Enemy->SetVerticalAcceleration(0.0f);
}

NodeStatus EnemyJumpToFlyNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // 移動と方向更新を実行
    m_Enemy->Move();
    m_Enemy->DirectionUpdate();

    float velY = m_Enemy->GetVerticalVelocity();

    // 上昇中であれば減速させていく
    if (velY > 0.0f) {
        float fallSpeed = m_Enemy->GetFallSpeed();
        float newVelY = velY - fallSpeed * (1.0f / 60.0f);
        if (newVelY < 0.0f)
            newVelY = 0.0f;
        m_Enemy->SetVerticalVelocity(newVelY);
        return NodeStatus::Running;
    }

    // 最高到達点で停止
    m_Enemy->SetVerticalVelocity(0.0f);

    // 遷移時間を待機
    m_ElapsedTime += 1.0f / 60.0f;
    if (m_ElapsedTime >= kFlyTransitionTime) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyJumpToFlyNode::OnExit() {
    // 飛行状態のまま終了
}

void EnemyFlyAscendNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);

    if (!m_Enemy)
        return;

    // 飛行状態を有効化し、重力を切る
    m_Enemy->SetIsFlying(true);
    m_Enemy->SetVerticalAcceleration(0.0f);
    // 上昇速度を設定
    m_Enemy->SetVerticalVelocity(m_Speed);
}

NodeStatus EnemyFlyAscendNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // 上昇速度を維持
    m_Enemy->SetVerticalVelocity(m_Speed);
    m_Enemy->Move();
    m_Enemy->DirectionUpdate();

    m_CurrentTimer += 1.0f / 60.0f;
    // 規定時間経過したら終了
    if (m_CurrentTimer >= m_TargetDuration) {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyFlyAscendNode::OnExit() {
    if (!m_Enemy)
        return;
    // 上昇を停止し、空中停止状態へ
    m_Enemy->SetVerticalVelocity(0.0f);
    m_Enemy->SetVerticalAcceleration(0.0f);
}

void EnemyFlyDescendNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);

    if (!m_Enemy)
        return;

    // 飛行状態を維持し、下降速度を設定
    m_Enemy->SetIsFlying(true);
    m_Enemy->SetVerticalAcceleration(0.0f);
    m_Enemy->SetVerticalVelocity(-m_Speed);
}

NodeStatus EnemyFlyDescendNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    Vector3 pos = m_Enemy->GetLocalPosition();
    float dt = Frame::DeltaTime();

    // 垂直座標を更新
    const float kDescendSpeed = 10.0f;
    pos.y -= kDescendSpeed * dt;

    // 着地判定
    if (pos.y <= 0.0f) {
        pos.y = 0.0f;
        m_Enemy->SetLocalPosition(pos);

        // 地上状態へ切り替え
        m_Enemy->SetIsFlying(false);
        m_Enemy->SetIsGrounded(true);
        m_Enemy->SetVelocity({0.0f, 0.0f, 0.0f});

        return NodeStatus::Success;
    }

    m_Enemy->SetLocalPosition(pos);
    return NodeStatus::Running;
}

void EnemyFlyDescendNode::OnExit() {
    if (!m_Enemy)
        return;
    // 下降を停止
    m_Enemy->SetVerticalVelocity(0.0f);
    m_Enemy->SetVerticalAcceleration(0.0f);
}

void EnemyFlyApproachNode::OnEnter() {
    m_CurrentTimer = 0.0f;
    m_TargetDuration = Random::Range(m_MinTime, m_MaxTime);

    if (!m_Enemy)
        return;

    // 飛行状態で水平移動のみ行う
    m_Enemy->SetIsFlying(true);
    m_Enemy->SetVerticalAcceleration(0.0f);
    m_Enemy->SetVerticalVelocity(0.0f);
    m_Enemy->GetMoveSpeed() = m_Speed;
}

NodeStatus EnemyFlyApproachNode::OnUpdate() {
    if (!m_Enemy || !m_Player)
        return NodeStatus::Failure;

    // プレイヤーに向かって水平移動
    m_Enemy->MoveToTarget(m_Player->GetWorldPosition());
    m_Enemy->DirectionUpdate();

    m_CurrentTimer += 1.0f / 60.0f;
    if (m_CurrentTimer >= m_TargetDuration)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

void EnemyFlyApproachNode::OnExit() {
    if (!m_Enemy)
        return;
    // 水平移動を停止
    m_Enemy->StopMovement();
}

void EnemyFlyToGroundNode::OnEnter() {
    if (!m_Enemy)
        return;

    // 飛行状態を解除し、重力を再開させて落下させる
    m_Enemy->SetIsFlying(false);
    float fallSpeed = m_Enemy->GetFallSpeed();
    m_Enemy->SetVerticalAcceleration(fallSpeed > 0.0f ? -fallSpeed : fallSpeed);
}

NodeStatus EnemyFlyToGroundNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 着地するまで Running
    if (m_Enemy->GetIsGrounded()) {
        m_Enemy->SetVerticalVelocity(0.0f);
        m_Enemy->SetVerticalAcceleration(0.0f);
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyShootNode::OnEnter() {
    m_Timer = 0.0f;
    m_HasShot = false;
}

NodeStatus EnemyShootNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 初回更新時に弾を発射
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

NodeStatus EnemyLockOnNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // ロックオン状態を更新
    m_Enemy->SetIsLockOn(m_LockOn);
    return NodeStatus::Success;
}

NodeStatus IsEnemyLockOnNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // 現在のロックオン状態を返す
    return m_Enemy->GetIsLockOn() ? NodeStatus::Success : NodeStatus::Failure;
}

void EnemyComboStepNode::OnEnter() {
    m_Timer = 0.0f;
    m_HasStep = false;
}

NodeStatus EnemyComboStepNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    // コンボステップを開始
    if (!m_HasStep) {
        m_Enemy->SetComboAttack(true);
        m_HasStep = true;
    }

    // 指定された待機時間が経過するまで Running
    float waitTime = (m_ComboInterval > 0.0f) ? m_ComboInterval : m_StepDuration;
    m_Timer += 1.0f / 60.0f;
    if (m_Timer >= waitTime)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

void EnemyComboFullNode::OnEnter() {
    m_Timer = 0.0f;
    m_StepCount = 0;
    m_WaitingStep = false;
}

NodeStatus EnemyComboFullNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    int comboLength = m_Enemy->GetPunchComboLength();
    int limit = (m_MaxSteps > 0) ? m_MaxSteps : comboLength;

    // 全てのステップが完了したか判定
    if (m_StepCount >= limit)
        return NodeStatus::Success;

    m_Timer += 1.0f / 60.0f;

    // 各ステップを順次実行
    if (!m_WaitingStep) {
        m_Enemy->SetComboAttack(true);
        m_StepCount++;
        m_WaitingStep = true;
        m_Timer = 0.0f;
    }

    float waitTime = (m_ComboInterval > 0.0f) ? m_ComboInterval : m_StepDuration;
    if (m_Timer >= waitTime) {
        m_WaitingStep = false;
        // 途中でコンボがリセットされたら終了
        if (!m_Enemy->IsPunchComboActive())
            return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyBurstShootNode::FireOneBullet() {
    if (!m_Enemy)
        return;

    // ホーミング弾の場合
    if (m_HomingMode) {
        m_Enemy->Shot();
        return;
    }

    // 通常弾または拡散弾の場合
    if (m_SpreadAngle <= 0.0f) {
        m_Enemy->ShotWithDirection(m_Enemy->GetForward(), false);
        return;
    }

    // 指定された角度内でランダムな方向に発射
    float halfAngle = m_SpreadAngle * 0.5f;
    float yawDeg = Random::Range(-halfAngle, halfAngle);
    float pitchDeg = Random::Range(-halfAngle * 0.3f, halfAngle * 0.3f);

    float yawRad = yawDeg * (3.14159265f / 180.0f);
    float pitchRad = pitchDeg * (3.14159265f / 180.0f);

    Vector3 forward = m_Enemy->GetForward();
    Vector3 up = m_Enemy->GetUp();
    Vector3 right = m_Enemy->GetRight();

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

    // 最初の弾を発射
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
        // 発射間隔を待機して次弾を発射
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

    // 全弾発射後のクールダウン待機
    if (m_Timer >= m_Cooldown)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

void EnemyEnergyChargeNode::OnEnter() {
    if (!m_Enemy)
        return;

    // チャージ開始時に移動を停止
    m_Enemy->SetVelocity({0.0f, 0.0f, 0.0f});
    m_Enemy->StopMovement();

    // 元のエネルギー回復レートを保存し、チャージ用の高速レートを設定
    m_OriginalRecoveryRate = m_Enemy->GetEnergyRecoveryRate();
    float chargeRate = 15.0f * m_ChargeRateMultiplier;
    m_Enemy->SetEnergyRecoveryRate(chargeRate);
}

NodeStatus EnemyEnergyChargeNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    float maxEnergy = m_Enemy->GetMaxEnergy();
    float currentEnergy = m_Enemy->GetEnergy();
    float targetEnergy = (m_TargetRatio > 0.0f) ? maxEnergy * m_TargetRatio : maxEnergy;

    // 目標エネルギーに達するまで回復を継続
    if (currentEnergy >= targetEnergy) {
        return NodeStatus::Success;
    }

    // チャージ中は移動不可
    m_Enemy->SetVelocity({0.0f, 0.0f, 0.0f});

    float dt = Frame::DeltaTime();
    float newEnergy = currentEnergy + (m_Enemy->GetEnergyRecoveryRate() * dt);
    m_Enemy->SetEnergy(std::min(newEnergy, maxEnergy));

    return NodeStatus::Running;
}

void EnemyEnergyChargeNode::OnExit() {
    if (m_Enemy) {
        // 保存しておいた元の回復レートに戻す
        m_Enemy->SetEnergyRecoveryRate(m_OriginalRecoveryRate);
    }
}

// ---------------------------------------------------------
// IsPlayerHPLowNode
// ---------------------------------------------------------
NodeStatus IsPlayerHPLowNode::OnUpdate() {
    if (!m_Player)
        return NodeStatus::Failure;

    float maxHP = m_Player->GetMaxHP();
    if (maxHP <= 0.0f)
        return NodeStatus::Failure;

    return (m_Player->GetHP() / maxHP) <= m_Threshold ? NodeStatus::Success : NodeStatus::Failure;
}

// ---------------------------------------------------------
// IsEnergyHighNode
// ---------------------------------------------------------
NodeStatus IsEnergyHighNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    float maxEnergy = m_Enemy->GetMaxEnergy();
    if (maxEnergy <= 0.0f)
        return NodeStatus::Failure;

    return (m_Enemy->GetEnergy() / maxEnergy) >= m_Threshold ? NodeStatus::Success : NodeStatus::Failure;
}

// ---------------------------------------------------------
// EnemyChargeAttackNode
// ---------------------------------------------------------
void EnemyChargeAttackNode::OnEnter() {
    m_Timer      = 0.0f;
    m_ShotsFired = 0;
    m_Phase      = Phase::Charge;

    if (!m_Enemy)
        return;

    // 溜め中は停止する（弾発射時にShot()内でエネルギーを消費）
    m_Enemy->SetVelocity({0.0f, 0.0f, 0.0f});
    m_Enemy->StopMovement();
}

NodeStatus EnemyChargeAttackNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    m_Timer += 1.0f / 60.0f;

    if (m_Phase == Phase::Charge) {
        m_Enemy->SetVelocity({0.0f, 0.0f, 0.0f});
        if (m_Timer >= m_ChargeDuration) {
            // 溜め完了 → 即1発目を発射してShootフェーズへ
            m_Enemy->Shot();
            m_ShotsFired = 1;
            m_Timer      = 0.0f;
            m_Phase      = (m_ShotsFired >= m_BurstCount) ? Phase::Cooldown : Phase::Shoot;
        }
        return NodeStatus::Running;
    }

    if (m_Phase == Phase::Shoot) {
        if (m_Timer >= kShootInterval) {
            m_Timer = 0.0f;
            m_Enemy->Shot();
            ++m_ShotsFired;
            if (m_ShotsFired >= m_BurstCount) {
                m_Phase = Phase::Cooldown;
                m_Timer = 0.0f;
            }
        }
        return NodeStatus::Running;
    }

    // Phase::Cooldown
    if (m_Timer >= kCooldown)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

// ---------------------------------------------------------
// EnemyUltimateNode
// ---------------------------------------------------------
void EnemyUltimateNode::OnEnter() {
    m_Timer       = 0.0f;
    m_StepCount   = 0;
    m_ShotsFired  = 0;
    m_WaitingStep = false;
    m_Phase       = Phase::Combo;

    if (!m_Enemy)
        return;

    // 停止してコンボ開始（弾発射時にShot()内でエネルギーを消費）
    m_Enemy->SetVelocity({0.0f, 0.0f, 0.0f});
    m_Enemy->StopMovement();
}

NodeStatus EnemyUltimateNode::OnUpdate() {
    if (!m_Enemy)
        return NodeStatus::Failure;

    m_Timer += 1.0f / 60.0f;

    if (m_Phase == Phase::Combo) {
        if (m_StepCount >= kMaxComboSteps) {
            // コンボ完了 → 即1発目発射してShootフェーズへ
            m_Enemy->Shot();
            m_ShotsFired = 1;
            m_Timer      = 0.0f;
            m_Phase      = (m_ShotsFired >= m_ShotCount) ? Phase::Cooldown : Phase::Shoot;
            return NodeStatus::Running;
        }

        if (!m_WaitingStep) {
            m_Enemy->SetComboAttack(true);
            ++m_StepCount;
            m_WaitingStep = true;
            m_Timer       = 0.0f;
        }

        if (m_Timer >= m_StepDuration) {
            m_WaitingStep = false;
            if (!m_Enemy->IsPunchComboActive())
                m_StepCount = kMaxComboSteps; // 強制終了
        }
        return NodeStatus::Running;
    }

    if (m_Phase == Phase::Shoot) {
        if (m_Timer >= kShootInterval) {
            m_Timer = 0.0f;
            m_Enemy->Shot();
            ++m_ShotsFired;
            if (m_ShotsFired >= m_ShotCount) {
                m_Phase = Phase::Cooldown;
                m_Timer = 0.0f;
            }
        }
        return NodeStatus::Running;
    }

    // Phase::Cooldown
    if (m_Timer >= kCooldown)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

void EnemyUltimateNode::OnExit() {
    if (m_Enemy)
        m_Enemy->StopMovement();
}
#define NOMINMAX
#include "BehaviorNode.h"
#include "Application/entity/enemy/Enemy.h"
#include "Application/entity/player/Player.h"
#include "Frame.h"
#include <iostream>

// ---------------------------------------------------------
// BTNode / CompositeNode 基底
// ---------------------------------------------------------
using namespace Hagine;
NodeStatus BTNode::Tick()
{
    // 初回実行時、または Running 以外から再開された時に OnEnter を呼ぶ
    if (status_ != NodeStatus::Running)
        OnEnter();

    // 更新処理を実行しステータスを更新
    status_ = OnUpdate();

    // 終了（Success または Failure）した時に OnExit を呼ぶ
    if (status_ != NodeStatus::Running)
        OnExit();

    return status_;
}
void BTNode::AddChild(std::shared_ptr<BTNode> child) {}
void BTNode::SetContext(Enemy *enemy, Player *player) {}
void BTNode::OnEnter() {}
void BTNode::OnExit() {}

void CompositeNode::AddChild(std::shared_ptr<BTNode> child)
{
    // 子ノードリストに追加
    children_.push_back(child);
}

void CompositeNode::SetContext(Enemy *enemy, Player *player)
{
    BTNode::SetContext(enemy, player);
    // 全ての子ノードにコンテキストを伝播
    for (auto &child : children_)
        child->SetContext(enemy, player);
}

void CompositeNode::OnEnter()
{
    // 最初の子から実行するようにインデックスをリセット
    currentChildIndex_ = 0;
}

// ---------------------------------------------------------
// SequenceNode (Reactive)
// ---------------------------------------------------------
NodeStatus SequenceNode::OnUpdate()
{
    if (children_.empty())
    {
        return NodeStatus::Failure;
    }

    // 毎フレーム先頭から順に子ノードを評価する (Reactive)
    for (int i = 0; i < children_.size(); ++i)
    {
        NodeStatus childStatus = children_[i]->Tick();

        if (childStatus == NodeStatus::Running)
        {
            // Running 中のノードより後のノードはリセットしておく
            for (int j = i + 1; j < children_.size(); ++j)
            {
                children_[j]->Reset();
            }
            return NodeStatus::Running;
        }

        if (childStatus == NodeStatus::Failure)
        {
            // 一つでも失敗した時点で以降のノードをリセットして失敗を返す
            for (int j = i + 1; j < children_.size(); ++j)
            {
                children_[j]->Reset();
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
NodeStatus SequenceOnceNode::OnUpdate()
{
    if (children_.empty())
        return NodeStatus::Failure;

    // 前回の続きから子ノードを順次実行する
    while (currentChildIndex_ < static_cast<int>(children_.size()))
    {
        NodeStatus status = children_[currentChildIndex_]->Tick();

        // 実行中の場合はそのノードで止まる
        if (status == NodeStatus::Running)
            return NodeStatus::Running;

        // 失敗した場合は即座に終了
        if (status == NodeStatus::Failure)
            return NodeStatus::Failure;

        // 成功した場合は次の子ノードへ進む
        currentChildIndex_++;
    }

    return NodeStatus::Success;
}

// ---------------------------------------------------------
// SelectorNode (Reactive)
// ---------------------------------------------------------
NodeStatus SelectorNode::OnUpdate()
{
    if (children_.empty())
    {
        return NodeStatus::Failure;
    }

    // 毎フレーム先頭から順に子ノードを評価する (Reactive)
    for (int i = 0; i < children_.size(); ++i)
    {
        NodeStatus childStatus = children_[i]->Tick();

        if (childStatus == NodeStatus::Running)
        {
            // 実行中のノード以降はリセット
            for (int j = i + 1; j < children_.size(); ++j)
            {
                children_[j]->Reset();
            }
            return NodeStatus::Running;
        }

        if (childStatus == NodeStatus::Success)
        {
            // 一つでも成功した時点で以降のノードをリセットして成功を返す
            for (int j = i + 1; j < children_.size(); ++j)
            {
                children_[j]->Reset();
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
RunActionNode::RunActionNode() : counter_(0), duration_(180) {}

void RunActionNode::OnEnter()
{
    // カウンターをリセット
    counter_ = 0;
}

NodeStatus RunActionNode::OnUpdate()
{
    counter_++;
    // 指定されたデュレーションが経過するまで Running を返す
    if (counter_ < duration_)
        return NodeStatus::Running;
    return NodeStatus::Success;
}

void TimedActionNode::OnEnter()
{
    currentTimer_ = 0.0f;
    // 設定された最小時間と最大時間の範囲からランダムに実行時間を決定
    targetDuration_ = Random::Range(minTime_, maxTime_);

    if (pEnemy_)
    {
        // 敵の移動速度を設定
        pEnemy_->SetMoveSpeed(speed_);
        // 各派生クラス固有の初期化（方向決定など）を実行
        SetupAction();
    }
}

NodeStatus TimedActionNode::OnUpdate()
{
    if (!pEnemy_ || !pPlayer_)
        return NodeStatus::Failure;

    // 各派生クラスの具体的な行動を実行
    ExecuteAction();

    // 経過時間を加算
    currentTimer_ += 1.0f / 60.0f;
    // 目標時間に達したら成功を返す
    if (currentTimer_ >= targetDuration_)
    {
        return NodeStatus::Success;
    }
    return NodeStatus::Running;
}

void TimedActionNode::OnExit()
{
    if (pEnemy_)
    {
        // アクション終了時に移動を停止
        pEnemy_->StopMovement();
    }
}

void EnemyApproachNode::ExecuteAction()
{
    // プレイヤーの位置に向かって移動
    pEnemy_->MoveToTarget(pPlayer_->GetWorldPosition());
}

void EnemyDashNode::ExecuteAction()
{
    // 高速でプレイヤーの位置に向かって移動（速度は OnEnter で設定済み）
    pEnemy_->MoveToTarget(pPlayer_->GetWorldPosition());
}

void EnemyStrafeNode::SetupAction()
{
    // 開始時に左右どちらに回り込むかをランダムに決定
    int dir = (Random::Range(0, 1) == 0) ? -1 : 1;
    pEnemy_->SetStrafeDirection(dir);
}

void EnemyStrafeNode::ExecuteAction()
{
    // 設定された方向に回り込み移動を実行
    pEnemy_->MoveStrafe();
}

void EnemyRetreatNode::ExecuteAction()
{
    // プレイヤーから離れる方向に移動
    pEnemy_->MoveRetreat();
}

NodeStatus EnemyAttackNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // 攻撃動作を実行
    pEnemy_->PerformAttack();
    // 攻撃演出時間を加算（固定1秒）
    timer_ += 1.0f / 60.0f;
    if (timer_ >= 1.0f)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

// ---------------------------------------------------------
// IsPlayerCloseNode
// ---------------------------------------------------------
NodeStatus IsPlayerCloseNode::OnUpdate()
{
    if (!pEnemy_ || !pPlayer_)
        return NodeStatus::Failure;

    // 敵とプレイヤーの距離を算出
    float dist = (pEnemy_->GetWorldPosition() - pPlayer_->GetWorldPosition()).Length();

    // ヒステリシスを考慮した判定用距離を設定
    float effectiveMinDist = minDist_;
    float effectiveMaxDist = maxDist_;

    if (lastResult_ == NodeStatus::Success)
    {
        // 前回成功していたらマージンを広げて状態を維持しやすくする
        effectiveMinDist -= kHysteresisMargin * 2.0f;
        effectiveMaxDist += kHysteresisMargin * 2.0f;
    }
    else if (lastResult_ == NodeStatus::Failure)
    {
        // 前回失敗していたらマージンを狭めて判定を厳しくする
        effectiveMinDist += kHysteresisMargin;
        effectiveMaxDist -= kHysteresisMargin;
    }

    // 判定範囲内にいるかチェック
    bool isInRange = (dist >= effectiveMinDist && dist <= effectiveMaxDist);
    NodeStatus newResult = isInRange ? NodeStatus::Success : NodeStatus::Failure;

    // 判定結果が変わった場合の処理
    if (newResult != lastResult_)
    {
        // 成功から失敗に変わる際、保持時間をチェックしてチャタリングを防止
        if (lastResult_ == NodeStatus::Success && stableTimer_ < kSuccessHoldTime)
        {
            stableTimer_ += 1.0f / 60.0f;
            return NodeStatus::Success;
        }

        stableTimer_ = 0.0f;
        lastResult_ = newResult;
    }
    else
    {
        stableTimer_ += 1.0f / 60.0f;
    }

    // 最小安定時間を経過するまでは前回の結果を維持
    if (stableTimer_ < kMinStableTime && lastResult_ != NodeStatus::Idle)
    {
        return lastResult_;
    }

    return newResult;
}

NodeStatus IsHealthLowNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // HP比率を計算して閾値以下か判定
    float maxHP = pEnemy_->GetMaxHP();
    float currentHP = pEnemy_->GetHP();
    if (maxHP > 0 && (currentHP / maxHP) <= thresholdPercentage_)
        return NodeStatus::Success;

    return NodeStatus::Failure;
}

NodeStatus IsEnergyLowNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // エネルギー比率を計算して閾値以下か判定
    float maxEnergy = pEnemy_->GetMaxEnergy();
    float currentEnergy = pEnemy_->GetEnergy();
    if (maxEnergy > 0.0f && (currentEnergy / maxEnergy) <= thresholdPercentage_)
        return NodeStatus::Success;

    return NodeStatus::Failure;
}

NodeStatus IsGroundedNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // 接地フラグをそのまま返す
    return pEnemy_->GetIsGrounded() ? NodeStatus::Success : NodeStatus::Failure;
}

NodeStatus IsAirborneNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // 非接地（空中）状態か判定
    return !pEnemy_->GetIsGrounded() ? NodeStatus::Success : NodeStatus::Failure;
}

NodeStatus IsPlayerStateNode::OnUpdate()
{
    if (!pEnemy_ || !pPlayer_)
        return NodeStatus::Failure;

    // プレイヤーの現在のステート名を取得して一致するか判定
    std::string currentState = pPlayer_->GetCurrentStateName();
    return (currentState == stateName_) ? NodeStatus::Success : NodeStatus::Failure;
}

void RandomSelectorNode::OnEnter()
{
    selectedChildIndex_ = -1;
    if (children_.empty())
        return;

    // 重みの合計を算出
    float totalWeight = 0.0f;
    std::vector<float> weights;

    for (const auto &child : children_)
    {
        float w = 1.0f;
        auto weightNode = std::dynamic_pointer_cast<WeightDecoratorNode>(child);
        if (weightNode)
        {
            w = weightNode->GetWeight();
        }
        if (w < 0.0f)
            w = 0.0f;

        weights.push_back(w);
        totalWeight += w;
    }

    // 算出した重みに基づいてランダムに子ノードを選択
    float randomValue = Random::Range(0.0f, totalWeight);
    float currentSum = 0.0f;
    for (int i = 0; i < weights.size(); ++i)
    {
        currentSum += weights[i];
        if (randomValue <= currentSum)
        {
            selectedChildIndex_ = i;
            break;
        }
    }

    // 誤差等で決まらなかった場合は最後の要素を選択
    if (selectedChildIndex_ == -1 && !children_.empty())
    {
        selectedChildIndex_ = static_cast<int>(children_.size()) - 1;
    }
}

NodeStatus RandomSelectorNode::OnUpdate()
{
    if (children_.empty())
    {
        return NodeStatus::Failure;
    }

    if (selectedChildIndex_ < 0 || selectedChildIndex_ >= children_.size())
    {
        return NodeStatus::Failure;
    }

    // 選択された子ノードのみを実行
    return children_[selectedChildIndex_]->Tick();
}

void EnemyIdleNode::Reset()
{
    BTNode::Reset();
    timer_ = 0.0f;
}

void EnemyIdleNode::OnEnter()
{
    BTNode::Reset();
    timer_ = 0.0f;
}

void EnemyJumpNode::OnEnter()
{
    jumpExecuted_ = false;

    if (!pEnemy_)
        return;

    // 地面にいる時のみジャンプを実行
    if (pEnemy_->GetIsGrounded())
    {
        // 上向きの初速を与える
        pEnemy_->SetVerticalVelocity(jumpPower_);
        pEnemy_->SetIsGrounded(false);

        // 重力を下向きに設定
        float fallSpeed = pEnemy_->GetFallSpeed();
        pEnemy_->SetVerticalAcceleration(fallSpeed > 0.0f ? -fallSpeed : fallSpeed);

        jumpExecuted_ = true;
    }
}

NodeStatus EnemyJumpNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    if (!jumpExecuted_)
    {
        return NodeStatus::Failure;
    }

    // 着地するまで Running を返し、着地したら Success
    if (pEnemy_->GetIsGrounded())
    {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyJumpToFlyNode::OnEnter()
{
    elapsedTime_ = 0.0f;

    if (!pEnemy_)
        return;

    // 上向きの速度を与えてジャンプ開始
    pEnemy_->SetVerticalVelocity(jumpPower_);
    pEnemy_->SetIsGrounded(false);

    // 飛行状態へ移行（重力を自前で制御するためフラグを立てる）
    pEnemy_->SetIsFlying(true);
    pEnemy_->SetVerticalAcceleration(0.0f);
}

NodeStatus EnemyJumpToFlyNode::OnUpdate()
{
    if (!pEnemy_ || !pPlayer_)
        return NodeStatus::Failure;

    // 移動と方向更新を実行
    pEnemy_->Move();
    pEnemy_->DirectionUpdate();

    float velY = pEnemy_->GetVerticalVelocity();

    // 上昇中であれば減速させていく
    if (velY > 0.0f)
    {
        float fallSpeed = pEnemy_->GetFallSpeed();
        float newVelY = velY - fallSpeed * (1.0f / 60.0f);
        if (newVelY < 0.0f)
            newVelY = 0.0f;
        pEnemy_->SetVerticalVelocity(newVelY);
        return NodeStatus::Running;
    }

    // 最高到達点で停止
    pEnemy_->SetVerticalVelocity(0.0f);

    // 遷移時間を待機
    elapsedTime_ += 1.0f / 60.0f;
    if (elapsedTime_ >= kFlyTransitionTime)
    {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyJumpToFlyNode::OnExit()
{
    // 飛行状態のまま終了
}

void EnemyFlyAscendNode::OnEnter()
{
    currentTimer_ = 0.0f;
    targetDuration_ = Random::Range(minTime_, maxTime_);

    if (!pEnemy_)
        return;

    // 飛行状態を有効化し、重力を切る
    pEnemy_->SetIsFlying(true);
    pEnemy_->SetVerticalAcceleration(0.0f);
    // 上昇速度を設定
    pEnemy_->SetVerticalVelocity(speed_);
}

NodeStatus EnemyFlyAscendNode::OnUpdate()
{
    if (!pEnemy_ || !pPlayer_)
        return NodeStatus::Failure;

    // 上昇速度を維持
    pEnemy_->SetVerticalVelocity(speed_);
    pEnemy_->Move();
    pEnemy_->DirectionUpdate();

    currentTimer_ += 1.0f / 60.0f;
    // 規定時間経過したら終了
    if (currentTimer_ >= targetDuration_)
    {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyFlyAscendNode::OnExit()
{
    if (!pEnemy_)
        return;
    // 上昇を停止し、空中停止状態へ
    pEnemy_->SetVerticalVelocity(0.0f);
    pEnemy_->SetVerticalAcceleration(0.0f);
}

void EnemyFlyDescendNode::OnEnter()
{
    currentTimer_ = 0.0f;
    targetDuration_ = Random::Range(minTime_, maxTime_);

    if (!pEnemy_)
        return;

    // 飛行状態を維持し、下降速度を設定
    pEnemy_->SetIsFlying(true);
    pEnemy_->SetVerticalAcceleration(0.0f);
    pEnemy_->SetVerticalVelocity(-speed_);
}

NodeStatus EnemyFlyDescendNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    Vector3 pos = pEnemy_->GetLocalPosition();
    float dt = Frame::DeltaTime();

    // 垂直座標を更新
    const float kDescendSpeed = 10.0f;
    pos.y -= kDescendSpeed * dt;

    // 着地判定
    if (pos.y <= 0.0f)
    {
        pos.y = 0.0f;
        pEnemy_->SetLocalPosition(pos);

        // 地上状態へ切り替え
        pEnemy_->SetIsFlying(false);
        pEnemy_->SetIsGrounded(true);
        pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});

        return NodeStatus::Success;
    }

    pEnemy_->SetLocalPosition(pos);
    return NodeStatus::Running;
}

void EnemyFlyDescendNode::OnExit()
{
    if (!pEnemy_)
        return;
    // 下降を停止
    pEnemy_->SetVerticalVelocity(0.0f);
    pEnemy_->SetVerticalAcceleration(0.0f);
}

void EnemyFlyApproachNode::OnEnter()
{
    currentTimer_ = 0.0f;
    targetDuration_ = Random::Range(minTime_, maxTime_);

    if (!pEnemy_)
        return;

    // 飛行状態で水平移動のみ行う
    pEnemy_->SetIsFlying(true);
    pEnemy_->SetVerticalAcceleration(0.0f);
    pEnemy_->SetVerticalVelocity(0.0f);
    pEnemy_->GetMoveSpeed() = speed_;
}

NodeStatus EnemyFlyApproachNode::OnUpdate()
{
    if (!pEnemy_ || !pPlayer_)
        return NodeStatus::Failure;

    // プレイヤーに向かって水平移動
    pEnemy_->MoveToTarget(pPlayer_->GetWorldPosition());
    pEnemy_->DirectionUpdate();

    currentTimer_ += 1.0f / 60.0f;
    if (currentTimer_ >= targetDuration_)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

void EnemyFlyApproachNode::OnExit()
{
    if (!pEnemy_)
        return;
    // 水平移動を停止
    pEnemy_->StopMovement();
}

void EnemyFlyToGroundNode::OnEnter()
{
    if (!pEnemy_)
        return;

    // 飛行状態を解除し、重力を再開させて落下させる
    pEnemy_->SetIsFlying(false);
    float fallSpeed = pEnemy_->GetFallSpeed();
    pEnemy_->SetVerticalAcceleration(fallSpeed > 0.0f ? -fallSpeed : fallSpeed);
}

NodeStatus EnemyFlyToGroundNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // 着地するまで Running
    if (pEnemy_->GetIsGrounded())
    {
        pEnemy_->SetVerticalVelocity(0.0f);
        pEnemy_->SetVerticalAcceleration(0.0f);
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyShootNode::OnEnter()
{
    timer_ = 0.0f;
    hasShot_ = false;
}

NodeStatus EnemyShootNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // 初回更新時に弾を発射
    if (!hasShot_)
    {
        pEnemy_->Shot();
        hasShot_ = true;
    }

    // クールダウン待機
    timer_ += 1.0f / 60.0f;
    if (timer_ >= cooldown_)
    {
        return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

NodeStatus IsEnemyLockOnNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // 現在のロックオン状態を返す
    return pEnemy_->GetIsLockOn() ? NodeStatus::Success : NodeStatus::Failure;
}

void EnemyComboStepNode::OnEnter()
{
    timer_ = 0.0f;
    hasStep_ = false;
}

NodeStatus EnemyComboStepNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // コンボステップを開始
    if (!hasStep_)
    {
        pEnemy_->SetComboAttack(true);
        hasStep_ = true;
    }

    // 指定された待機時間が経過するまで Running
    float waitTime = (comboInterval_ > 0.0f) ? comboInterval_ : stepDuration_;
    timer_ += 1.0f / 60.0f;
    if (timer_ >= waitTime)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

void EnemyComboFullNode::OnEnter()
{
    timer_ = 0.0f;
    stepCount_ = 0;
    waitingStep_ = false;
}

NodeStatus EnemyComboFullNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    int comboLength = pEnemy_->GetPunchComboLength();
    int limit = (maxSteps_ > 0) ? maxSteps_ : comboLength;

    // 全てのステップが完了したか判定
    if (stepCount_ >= limit)
        return NodeStatus::Success;

    timer_ += 1.0f / 60.0f;

    // 各ステップを順次実行
    if (!waitingStep_)
    {
        pEnemy_->SetComboAttack(true);
        stepCount_++;
        waitingStep_ = true;
        timer_ = 0.0f;
    }

    float waitTime = (comboInterval_ > 0.0f) ? comboInterval_ : stepDuration_;
    if (timer_ >= waitTime)
    {
        waitingStep_ = false;
        // 途中でコンボがリセットされたら終了
        if (!pEnemy_->IsPunchComboActive())
            return NodeStatus::Success;
    }

    return NodeStatus::Running;
}

void EnemyBurstShootNode::FireOneBullet()
{
    if (!pEnemy_)
        return;

    // ホーミング弾の場合
    if (homingMode_)
    {
        pEnemy_->Shot();
        return;
    }

    // 通常弾または拡散弾の場合
    if (spreadAngle_ <= 0.0f)
    {
        pEnemy_->ShotWithDirection(pEnemy_->GetForward(), false);
        return;
    }

    // 指定された角度内でランダムな方向に発射
    float halfAngle = spreadAngle_ * 0.5f;
    float yawDeg = Random::Range(-halfAngle, halfAngle);
    float pitchDeg = Random::Range(-halfAngle * 0.3f, halfAngle * 0.3f);

    float yawRad = yawDeg * (3.14159265f / 180.0f);
    float pitchRad = pitchDeg * (3.14159265f / 180.0f);

    Vector3 forward = pEnemy_->GetForward();
    Vector3 up = pEnemy_->GetUp();
    Vector3 right = pEnemy_->GetRight();

    Vector3 yawed = forward * std::cos(yawRad) + right * std::sin(yawRad);
    Vector3 dir = yawed * std::cos(pitchRad) + up * std::sin(pitchRad);

    float len = dir.Length();
    if (len > 0.001f)
        dir = dir / len;

    pEnemy_->ShotWithDirection(dir, false);
}

void EnemyBurstShootNode::OnEnter()
{
    timer_ = 0.0f;
    shotsFired_ = 0;
    phase_ = Phase::Shooting;

    // 最初の弾を発射
    if (pEnemy_ && burstCount_ > 0)
    {
        FireOneBullet();
        shotsFired_ = 1;

        if (shotsFired_ >= burstCount_)
        {
            phase_ = Phase::Cooldown;
        }
    }
}

NodeStatus EnemyBurstShootNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    timer_ += 1.0f / 60.0f;

    if (phase_ == Phase::Shooting)
    {
        // 発射間隔を待機して次弾を発射
        if (timer_ >= interval_)
        {
            timer_ = 0.0f;

            if (shotsFired_ < burstCount_)
            {
                FireOneBullet();
                shotsFired_++;
            }

            if (shotsFired_ >= burstCount_)
            {
                phase_ = Phase::Cooldown;
            }
        }
        return NodeStatus::Running;
    }

    // 全弾発射後のクールダウン待機
    if (timer_ >= cooldown_)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

void EnemyEnergyChargeNode::OnEnter()
{
    if (!pEnemy_)
        return;

    // チャージ開始時に移動を停止
    pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
    pEnemy_->StopMovement();

    // 元のエネルギー回復レートを保存し、チャージ用の高速レートを設定
    originalRecoveryRate_ = pEnemy_->GetEnergyRecoveryRate();
    float chargeRate = 15.0f * chargeRateMultiplier_;
    pEnemy_->SetEnergyRecoveryRate(chargeRate);
}

NodeStatus EnemyEnergyChargeNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    float maxEnergy = pEnemy_->GetMaxEnergy();
    float currentEnergy = pEnemy_->GetEnergy();
    float targetEnergy = (targetRatio_ > 0.0f) ? maxEnergy * targetRatio_ : maxEnergy;

    // 目標エネルギーに達するまで回復を継続
    if (currentEnergy >= targetEnergy)
    {
        return NodeStatus::Success;
    }

    // チャージ中は移動不可
    pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});

    float dt = Frame::DeltaTime();
    float newEnergy = currentEnergy + (pEnemy_->GetEnergyRecoveryRate() * dt);
    pEnemy_->SetEnergy(std::min(newEnergy, maxEnergy));

    return NodeStatus::Running;
}

void EnemyEnergyChargeNode::OnExit()
{
    if (pEnemy_)
    {
        // 保存しておいた元の回復レートに戻す
        pEnemy_->SetEnergyRecoveryRate(originalRecoveryRate_);
    }
}

// ---------------------------------------------------------
// IsPlayerHPLowNode
// ---------------------------------------------------------
NodeStatus IsPlayerHPLowNode::OnUpdate()
{
    if (!pPlayer_)
        return NodeStatus::Failure;

    float maxHP = pPlayer_->GetMaxHP();
    if (maxHP <= 0.0f)
        return NodeStatus::Failure;

    return (pPlayer_->GetHP() / maxHP) <= threshold_ ? NodeStatus::Success : NodeStatus::Failure;
}

// ---------------------------------------------------------
// IsEnergyHighNode
// ---------------------------------------------------------
NodeStatus IsEnergyHighNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    float maxEnergy = pEnemy_->GetMaxEnergy();
    if (maxEnergy <= 0.0f)
        return NodeStatus::Failure;

    return (pEnemy_->GetEnergy() / maxEnergy) >= threshold_ ? NodeStatus::Success : NodeStatus::Failure;
}

// ---------------------------------------------------------
// EnemyChargeAttackNode
// プレイヤーの ChargeShot 相当: 溜め演出 → 強化ホーミング弾を1発放つ
// ---------------------------------------------------------
void EnemyChargeAttackNode::OnEnter()
{
    timer_ = 0.0f;
    shotsFired_ = 0;
    phase_ = Phase::Charge;

    if (!pEnemy_)
        return;

    // 溜め中は停止し、チャージ演出（魔法陣＋魂の渦）を開始
    pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
    pEnemy_->StopMovement();
    pEnemy_->StartChargeAura();
}

NodeStatus EnemyChargeAttackNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    timer_ += 1.0f / 60.0f;

    if (phase_ == Phase::Charge)
    {
        pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
        if (timer_ >= chargeDuration_)
        {
            // 溜め完了 → 閃光＋強化ホーミング弾を放つ
            pEnemy_->FireChargeBlast();
            timer_ = 0.0f;
            phase_ = Phase::Cooldown;
        }
        return NodeStatus::Running;
    }

    // Phase::Cooldown（発射後の硬直）
    if (timer_ >= kCooldown)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

void EnemyChargeAttackNode::OnExit()
{
    // 中断された場合でも溜め演出を確実に止める
    if (pEnemy_)
        pEnemy_->StopChargeAura();
}

// ---------------------------------------------------------
// EnemyUltimateNode
// 重スキル: フルコンボ → ホーミング連射（必殺技より格下の強攻撃として運用）
// ---------------------------------------------------------
void EnemyUltimateNode::OnEnter()
{
    timer_ = 0.0f;
    stepCount_ = 0;
    shotsFired_ = 0;
    waitingStep_ = false;
    phase_ = Phase::Combo;

    if (!pEnemy_)
        return;

    pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
    pEnemy_->StopMovement();
}

NodeStatus EnemyUltimateNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    timer_ += 1.0f / 60.0f;

    if (phase_ == Phase::Combo)
    {
        if (stepCount_ >= kMaxComboSteps)
        {
            // コンボ完了 → 連射フェーズへ
            pEnemy_->Shot();
            shotsFired_ = 1;
            timer_ = 0.0f;
            phase_ = (shotsFired_ >= shotCount_) ? Phase::Cooldown : Phase::Shoot;
            return NodeStatus::Running;
        }

        if (!waitingStep_)
        {
            pEnemy_->SetComboAttack(true);
            ++stepCount_;
            waitingStep_ = true;
            timer_ = 0.0f;
        }

        if (timer_ >= stepDuration_)
        {
            waitingStep_ = false;
            if (!pEnemy_->IsPunchComboActive())
                stepCount_ = kMaxComboSteps;
        }
        return NodeStatus::Running;
    }

    if (phase_ == Phase::Shoot)
    {
        if (timer_ >= kShootInterval)
        {
            timer_ = 0.0f;
            pEnemy_->Shot();
            ++shotsFired_;
            if (shotsFired_ >= shotCount_)
            {
                phase_ = Phase::Cooldown;
                timer_ = 0.0f;
            }
        }
        return NodeStatus::Running;
    }

    if (timer_ >= kCooldown)
        return NodeStatus::Success;
    return NodeStatus::Running;
}

void EnemyUltimateNode::OnExit()
{
    if (pEnemy_)
        pEnemy_->StopMovement();
}

// ---------------------------------------------------------
// EnemyBeamUltimateNode
// MakanAttackSkill 相当のビーム必殺技
// 溜め(ロックオン+オーラ) → ビーム発射 → 硬直
// ---------------------------------------------------------
void EnemyBeamUltimateNode::OnEnter()
{
    timer_ = 0.0f;
    phase_ = Phase::Windup;

    if (!pEnemy_)
        return;

    pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
    pEnemy_->StopMovement();
    pEnemy_->SetIsLockOn(true);
    pEnemy_->StartChargeAura(); // 溜め中はチャージオーラを発光
}

NodeStatus EnemyBeamUltimateNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    timer_ += 1.0f / 60.0f;

    if (phase_ == Phase::Windup)
    {
        pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
        if (timer_ >= windupDuration_)
        {
            // 溜め完了 → 発動前演出（カメラ顔アップ→通常カメラ復帰→遅延→発動）へ。
            // 演出・遅延中はロックオン（照準追従）を維持し、発動の瞬間に
            // Enemy 側のコールバックがロックオン解除＋向き固定＋発射を行う。
            // これにより無予備動作の即発射ではなくなり、プレイヤーが回避できる
            pEnemy_->StartBeamStaging();
            timer_ = 0.0f;
            phase_ = Phase::Staging;
        }
        return NodeStatus::Running;
    }

    if (phase_ == Phase::Staging)
    {
        pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
        // 演出＋遅延が終わり実際にビームが発射されたら Beam フェーズへ
        if (pEnemy_->IsBeamActive())
        {
            timer_ = 0.0f;
            phase_ = Phase::Beam;
        }
        return NodeStatus::Running;
    }

    if (phase_ == Phase::Beam)
    {
        pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
        // ビームが自然終了するまで待つ
        if (!pEnemy_->IsBeamActive())
        {
            timer_ = 0.0f;
            phase_ = Phase::Cooldown;
        }
        return NodeStatus::Running;
    }

    // Phase::Cooldown
    if (timer_ >= kCooldown)
        return NodeStatus::Success;
    return NodeStatus::Running;
}

void EnemyBeamUltimateNode::OnExit()
{
    if (pEnemy_)
    {
        pEnemy_->CancelBeamStaging();
        pEnemy_->StopChargeAura();
        pEnemy_->DeactivateBeam();
        pEnemy_->StopMovement();
    }
}

// ---------------------------------------------------------
// EnemyGuardNode
// 一定時間ガード状態に入り、被ダメージを軽減する
// ---------------------------------------------------------
void EnemyGuardNode::OnEnter()
{
    timer_ = 0.0f;
    if (!pEnemy_)
        return;
    // エネルギーが被弾消費量未満ならガードしない（OnUpdateでFailureを返す）
    if (!pEnemy_->CanGuard())
        return;
    pEnemy_->SetGuarding(true);
    pEnemy_->SetVelocity({0.0f, 0.0f, 0.0f});
    pEnemy_->StopMovement();
}

NodeStatus EnemyGuardNode::OnUpdate()
{
    if (!pEnemy_)
        return NodeStatus::Failure;

    // エネルギーが被弾消費量未満になったらガードできない（無くなったら維持もできない）
    if (!pEnemy_->CanGuard())
    {
        pEnemy_->SetGuarding(false);
        return NodeStatus::Failure;
    }

    timer_ += 1.0f / 60.0f;
    pEnemy_->SetVelocity({0.0f, pEnemy_->GetVelocity().y, 0.0f});

    if (timer_ >= duration_)
        return NodeStatus::Success;
    return NodeStatus::Running;
}

void EnemyGuardNode::OnExit()
{
    if (pEnemy_)
        pEnemy_->SetGuarding(false);
}

void EnemyGuardNode::Reset()
{
    BTNode::Reset();
    timer_ = 0.0f;
    // 別の枝に切り替わって Reset された場合でもガード状態を確実に解除する
    if (pEnemy_)
        pEnemy_->SetGuarding(false);
}

// ---------------------------------------------------------
// IsPlayerAttackingNode
// プレイヤーが脅威（Rush中・スキル発動中・コンボ中）かを判定する
// ---------------------------------------------------------
NodeStatus IsPlayerAttackingNode::OnUpdate()
{
    if (!pPlayer_)
        return NodeStatus::Failure;

    bool threatening =
        pPlayer_->GetCurrentStateName() == "Rush" ||
        pPlayer_->GetIsSkillActive() ||
        pPlayer_->IsComboActive();

    return threatening ? NodeStatus::Success : NodeStatus::Failure;
}
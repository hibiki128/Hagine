#pragma once
#include "../BehaviorNode/BehaviorNode.h"

// 移動速度の種類
enum class MoveSpeedType {
    Fast, // 速い
    Slow  // 遅い
};

// 近距離行動の種類
enum class CloseRangeAction {
    Approach, // さらに近づく
    Strafe,   // 横に移動
    Retreat   // 後退
};

/// <summary>
/// プレイヤーに近づく行動ノード
/// </summary>
class ApproachNode : public BehaviorNode {
  public:
    ApproachNode(MoveSpeedType speedType = MoveSpeedType::Fast);
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Approach"; }

    // 速度設定
    void SetSpeedType(MoveSpeedType type) { speedType_ = type; }
    MoveSpeedType GetSpeedType() const { return speedType_; }
    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }

  private:
    MoveSpeedType speedType_;
    float speed_ = 10.0f;
};

/// <summary>
/// 停止行動ノード
/// </summary>
class StopNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Stop"; }
};

/// <summary>
/// 近距離行動ノード(さらに近づく)
/// </summary>
class CloseApproachNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "CloseApproach"; }

    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }
    float GetTargetDistance() const { return targetDistance_; }
    void SetTargetDistance(float distance) { targetDistance_ = distance; }

  private:
    float speed_ = 5.0f;
    float targetDistance_ = 2.0f;
};

/// <summary>
/// 横移動行動ノード
/// </summary>
class StrafeNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Strafe"; }

    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }

  private:
    float speed_ = 8.0f;
    float strafeTime_ = 0.0f;
    float maxStrafeTime_ = 1.0f;
    int strafeDirection_ = 1; // 1: 右, -1: 左
};

/// <summary>
/// 後退行動ノード
/// </summary>
class RetreatNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Retreat"; }

    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }
    float GetRetreatDistance() const { return retreatDistance_; }
    void SetRetreatDistance(float distance) { retreatDistance_ = distance; }

  private:
    float speed_ = 7.0f;
    float retreatDistance_ = 5.0f;
};

/// <summary>
/// ガード行動ノード
/// 一定時間ガード状態を維持し、ダメージを軽減する
/// </summary>
class GuardNode : public BehaviorNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Guard"; }

    float GetGuardDuration() const { return guardDuration_; }
    void SetGuardDuration(float duration) { guardDuration_ = duration; }
    float GetDamageReduction() const { return damageReduction_; }
    void SetDamageReduction(float reduction) { damageReduction_ = reduction; }

  private:
    float guardDuration_ = 2.0f;    // ガード継続時間(秒)
    float damageReduction_ = 0.85f; // ダメージ軽減率(85%)
    float guardTimer_ = 0.0f;       // ガード経過時間
    bool guardStarted_ = false;     // ガード開始フラグ
};
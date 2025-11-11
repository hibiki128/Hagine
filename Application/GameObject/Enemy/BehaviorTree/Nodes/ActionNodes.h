#pragma once
#include "../BehaviorNode/BehaviorNode.h"
#include "../Nodes/InterruptNode.h"
#include <type/Vector3.h>

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
/// プレイヤーに近づく行動ノード(割り込み対応)
/// </summary>
class ApproachNode : public InterruptableNode {
  public:
    ApproachNode(MoveSpeedType speedType = MoveSpeedType::Fast);
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Approach"; }

    bool CheckInterruptCondition(Enemy &enemy) override;
    InterruptPriority GetInterruptPriority() const override;
    void Reset() override;

    // 速度設定
    void SetSpeedType(MoveSpeedType type) { speedType_ = type; }
    MoveSpeedType GetSpeedType() const { return speedType_; }
    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }

    // 割り込み条件設定
    float GetInterruptDistance() const { return interruptDistance_; }
    void SetInterruptDistance(float distance) { interruptDistance_ = distance; }
    float GetInterruptChance() const { return interruptChance_; }
    void SetInterruptChance(float chance) { interruptChance_ = chance; }

  private:
    MoveSpeedType speedType_;
    float speed_ = 10.0f;
    float interruptDistance_ = 15.0f; // この距離以上離れたら割り込み判定
    float interruptChance_ = 0.8f;    // 割り込み発動確率(80%)
    bool interruptCheckDone_ = false; // 割り込みチェック済みフラグ
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
class CloseApproachNode : public InterruptableNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "CloseApproach"; }

    bool CheckInterruptCondition(Enemy &enemy) override;
    InterruptPriority GetInterruptPriority() const override { return InterruptPriority::Low; }
    void Reset() override;

    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }
    float GetMinTargetDistance() const { return minTargetDistance_; }
    float GetMaxTargetDistance() const { return maxTargetDistance_; }
    void SetTargetDistanceRange(float min, float max) {
        minTargetDistance_ = min;
        maxTargetDistance_ = max;
    }

  private:
    float speed_ = 5.0f;
    float minTargetDistance_ = 1.5f;
    float maxTargetDistance_ = 3.0f;
    float currentTargetDistance_ = 2.0f;
    bool distanceSet_ = false;
};

/// <summary>
/// 横移動行動ノード
/// </summary>
class StrafeNode : public InterruptableNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Strafe"; }

    bool CheckInterruptCondition(Enemy &enemy) override;
    InterruptPriority GetInterruptPriority() const override { return InterruptPriority::Low; }
    void Reset() override;

    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }
    float GetMinStrafeTime() const { return minStrafeTime_; }
    float GetMaxStrafeTime() const { return maxStrafeTime_; }
    void SetStrafeTimeRange(float min, float max) {
        minStrafeTime_ = min;
        maxStrafeTime_ = max;
    }

  private:
    float speed_ = 8.0f;
    float strafeTimer_ = 0.0f;
    float minStrafeTime_ = 0.5f;
    float maxStrafeTime_ = 2.0f;
    float currentStrafeTime_ = 1.0f;
    int strafeDirection_ = 1;
    bool timeSet_ = false;
};

/// <summary>
/// 後退行動ノード
/// </summary>
class RetreatNode : public InterruptableNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Retreat"; }

    bool CheckInterruptCondition(Enemy &enemy) override;
    InterruptPriority GetInterruptPriority() const override { return InterruptPriority::Medium; }
    void Reset() override;

    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed) { speed_ = speed; }
    float GetMinRetreatDistance() const { return minRetreatDistance_; }
    float GetMaxRetreatDistance() const { return maxRetreatDistance_; }
    void SetRetreatDistanceRange(float min, float max) {
        minRetreatDistance_ = min;
        maxRetreatDistance_ = max;
    }

  private:
    float speed_ = 7.0f;
    float minRetreatDistance_ = 3.0f;
    float maxRetreatDistance_ = 7.0f;
    float currentRetreatDistance_ = 5.0f;
    Vector3 retreatStartPos_{};
    bool distanceSet_ = false;
};

/// <summary>
/// ガード行動ノード
/// </summary>
class GuardNode : public InterruptableNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "Guard"; }

    bool CheckInterruptCondition(Enemy &enemy) override;
    InterruptPriority GetInterruptPriority() const override { return InterruptPriority::Low; }
    void Reset() override;

    float GetMinGuardDuration() const { return minGuardDuration_; }
    float GetMaxGuardDuration() const { return maxGuardDuration_; }
    void SetGuardDurationRange(float min, float max) {
        minGuardDuration_ = min;
        maxGuardDuration_ = max;
    }
    float GetDamageReduction() const { return damageReduction_; }
    void SetDamageReduction(float reduction) { damageReduction_ = reduction; }

  private:
    float minGuardDuration_ = 1.0f;
    float maxGuardDuration_ = 3.0f;
    float currentGuardDuration_ = 2.0f;
    float damageReduction_ = 0.85f;
    float guardTimer_ = 0.0f;
    bool guardStarted_ = false;
    bool durationSet_ = false;
};

/// <summary>
/// 弾回避行動ノード(将来の実装用)
/// 最高優先度で割り込む
/// </summary>
class DodgeBulletNode : public InterruptableNode {
  public:
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;
    const char *GetNodeName() const override { return "DodgeBullet"; }

    bool CheckInterruptCondition(Enemy &enemy) override;
    InterruptPriority GetInterruptPriority() const override { return InterruptPriority::Critical; }
    void Reset() override;
    bool CanBeInterrupted() const override { return false; } // 回避中は割り込み不可

  private:
    float dodgeTimer_ = 0.0f;
    float dodgeDuration_ = 0.5f;
    Vector3 dodgeDirection_{};
    bool dodgeStarted_ = false;
};
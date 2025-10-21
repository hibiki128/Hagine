#pragma once
#include "Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "application/GameObject/Enemy/Enemy.h"

/// <summary>
/// 敵が地面に接地しているかを判定するビヘイビアノード
/// </summary>
class IsGroundedNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// ノードの実行処理
    /// </summary>
    /// <param name="enemy">対象のEnemy参照</param>
    /// <param name="deltaTime">フレームの経過時間</param>
    /// <returns>ノードの実行結果</returns>
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    /// <summary>
    /// ノード名取得
    /// </summary>
    /// <returns>ノードの名前</returns>
    const char *GetNodeName() const override { return "IsGrounded"; }
};

/// <summary>
/// 敵が空中にいるかを判定するビヘイビアノード
/// </summary>
class IsInAirNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// ノードの実行処理
    /// </summary>
    /// <param name="enemy">対象のEnemy参照</param>
    /// <param name="deltaTime">フレームの経過時間</param>
    /// <returns>ノードの実行結果</returns>
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    /// <summary>
    /// ノード名取得
    /// </summary>
    /// <returns>ノードの名前</returns>
    const char *GetNodeName() const override { return "IsInAir"; }
};

/// <summary>
/// 敵がターゲットを持っているかを判定するビヘイビアノード
/// </summary>
class HasTargetNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// ノードの実行処理
    /// </summary>
    /// <param name="enemy">対象のEnemy参照</param>
    /// <param name="deltaTime">フレームの経過時間</param>
    /// <returns>ノードの実行結果</returns>
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    /// <summary>
    /// ノード名取得
    /// </summary>
    /// <returns>ノードの名前</returns>
    const char *GetNodeName() const override { return "HasTarget"; }
};

/// <summary>
/// ターゲットまでの距離が指定範囲内かを判定するビヘイビアノード
/// </summary>
class DistanceToTargetNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="minDist">最小距離</param>
    /// <param name="maxDist">最大距離</param>
    DistanceToTargetNode(float minDist, float maxDist)
        : minDistance_(minDist), maxDistance_(maxDist) {}

    /// <summary>
    /// ノードの実行処理
    /// </summary>
    /// <param name="enemy">対象のEnemy参照</param>
    /// <param name="deltaTime">フレームの経過時間</param>
    /// <returns>ノードの実行結果</returns>
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    /// <summary>
    /// ノード名取得
    /// </summary>
    /// <returns>ノードの名前</returns>
    const char *GetNodeName() const override { return "DistanceCheck"; }

    /// <summary>
    /// 最小距離を取得
    /// </summary>
    /// <returns>現在の最小距離</returns>
    float GetMinDistance() const { return minDistance_; }

    /// <summary>
    /// 最大距離を取得
    /// </summary>
    /// <returns>現在の最大距離</returns>
    float GetMaxDistance() const { return maxDistance_; }

    /// <summary>
    /// 距離の範囲を設定
    /// </summary>
    /// <param name="minDist">設定する最小距離</param>
    /// <param name="maxDist">設定する最大距離</param>
    void SetDistances(float minDist, float maxDist) {
        minDistance_ = minDist;
        maxDistance_ = maxDist;
    }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    float minDistance_; // 最小距離
    float maxDistance_; // 最大距離
};

/// <summary>
/// プレイヤーが空中にいるかを判定するビヘイビアノード
/// </summary>
class IsPlayerAirborneNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// ノードの実行処理
    /// </summary>
    /// <param name="enemy">対象のEnemy参照</param>
    /// <param name="deltaTime">フレームの経過時間</param>
    /// <returns>ノードの実行結果</returns>
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    /// <summary>
    /// ノード名取得
    /// </summary>
    /// <returns>ノードの名前</returns>
    const char *GetNodeName() const override { return "IsPlayerAirborne"; }
};

/// <summary>
/// ターゲットまでの距離が閾値を超えているかを判定するビヘイビアノード
/// </summary>
class DistanceThresholdNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="threshold">距離の閾値</param>
    DistanceThresholdNode(float threshold) : threshold_(threshold) {}

    /// <summary>
    /// ノードの実行処理
    /// </summary>
    /// <param name="enemy">対象のEnemy参照</param>
    /// <param name="deltaTime">フレームの経過時間</param>
    /// <returns>ノードの実行結果</returns>
    NodeStatus Execute(Enemy &enemy, float deltaTime) override;

    /// <summary>
    /// ノード名取得
    /// </summary>
    /// <returns>ノードの名前</returns>
    const char *GetNodeName() const override { return "DistanceThreshold"; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    float threshold_; // 距離の閾値
};
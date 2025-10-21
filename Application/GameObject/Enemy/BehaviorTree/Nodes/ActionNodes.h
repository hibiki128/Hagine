#pragma once
#include "Application/GameObject/Enemy/BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Input.h"

/// <summary>
/// ターゲットに移動するビヘイビアノード
/// </summary>
class MoveToTargetNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="speed">移動速度</param>
    MoveToTargetNode(float speed) : moveSpeed_(speed) {}

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
    const char *GetNodeName() const override { return "MoveToTarget"; }

    /// <summary>
    /// 移動速度を取得
    /// </summary>
    /// <returns>現在の移動速度</returns>
    float GetSpeed() const { return moveSpeed_; }

    /// <summary>
    /// 移動速度を設定
    /// </summary>
    /// <param name="speed">設定する移動速度</param>
    void SetSpeed(float speed) { moveSpeed_ = speed; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    float moveSpeed_; // 移動速度
};

/// <summary>
/// ジャンプアクションを実行するビヘイビアノード
/// </summary>
class JumpNode : public BehaviorNode {
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
    const char *GetNodeName() const override { return "Jump"; }
};

/// <summary>
/// 飛行中のアイドル状態を実行するビヘイビアノード
/// </summary>
class FlyIdleNode : public BehaviorNode {
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
    const char *GetNodeName() const override { return "FlyIdle"; }
};

/// <summary>
/// 重力を適用するビヘイビアノード
/// </summary>
class ApplyGravityNode : public BehaviorNode {
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
    const char *GetNodeName() const override { return "ApplyGravity"; }
};

/// <summary>
/// ターゲットに向かって飛行するビヘイビアノード
/// </summary>
class FlyToTargetNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="speed">飛行速度</param>
    FlyToTargetNode(float speed) : moveSpeed_(speed) {}

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
    const char *GetNodeName() const override { return "FlyToTarget"; }

    /// <summary>
    /// 飛行速度を取得
    /// </summary>
    /// <returns>現在の飛行速度</returns>
    float GetSpeed() const { return moveSpeed_; }

    /// <summary>
    /// 飛行速度を設定
    /// </summary>
    /// <param name="speed">設定する飛行速度</param>
    void SetSpeed(float speed) { moveSpeed_ = speed; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    float moveSpeed_; // 飛行速度
};

/// <summary>
/// 突撃攻撃を実行するビヘイビアノード
/// </summary>
class RushAttackNode : public BehaviorNode {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="speed">突撃速度</param>
    /// <param name="minDistance">最小距離</param>
    RushAttackNode(float speed, float minDistance)
        : rushSpeed_(speed), minDistance_(minDistance) {}

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
    const char *GetNodeName() const override { return "RushAttack"; }

    /// <summary>
    /// 突撃速度を取得
    /// </summary>
    /// <returns>現在の突撃速度</returns>
    float GetRushSpeed() const { return rushSpeed_; }

    /// <summary>
    /// 突撃速度を設定
    /// </summary>
    /// <param name="speed">設定する突撃速度</param>
    void SetRushSpeed(float speed) { rushSpeed_ = speed; }

    /// <summary>
    /// 最小距離を取得
    /// </summary>
    /// <returns>現在の最小距離</returns>
    float GetMinDistance() const { return minDistance_; }

    /// <summary>
    /// 最小距離を設定
    /// </summary>
    /// <param name="dist">設定する最小距離</param>
    void SetMinDistance(float dist) { minDistance_ = dist; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    float rushSpeed_;                            // 突撃速度
    float minDistance_;                          // 最小距離
    Vector3 rushDirection_ = {0.0f, 0.0f, 0.0f}; // 突撃方向
    float rushElapsedTime_ = 0.0f;               // 突撃経過時間
};
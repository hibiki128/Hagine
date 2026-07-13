#pragma once
#include "Easing.h"
#include <application/entity/player/PlayerData.h>
#include <type/Vector3.h>

class Enemy;

/// <summary>
/// 敵の移動パーツクラス
/// 速度・加速度・イージング移動・回転（プレイヤー追従）・接地判定と、
/// それらのパラメータを担当する。実際の駆動はビヘイビアツリーから行われる
/// </summary>
class EnemyMovement
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（移動パラメータの初期値設定）
    /// </summary>
    /// <param name="owner">所有者の敵</param>
    void Init(Enemy *owner);

    /// <summary>
    /// ターゲットへ向かって移動を開始する（速度イージング）
    /// </summary>
    /// <param name="targetPos">目標位置</param>
    void MoveToTarget(const Hagine::Vector3 &targetPos);

    /// <summary>
    /// 横移動（ストレイフ）を開始する
    /// </summary>
    void MoveStrafe();

    /// <summary>
    /// ターゲットから離れる方向へ後退を開始する
    /// </summary>
    void MoveRetreat();

    /// <summary>
    /// 停止（水平速度を0へイージング）
    /// </summary>
    void StopMovement();

    /// <summary>
    /// 移動処理（現状ターゲット有無のみ判定するプレースホルダ）
    /// </summary>
    void Move();

    /// <summary>
    /// 方向情報の更新（現状未使用のプレースホルダ）
    /// </summary>
    void DirectionUpdate();

    /// <summary>
    /// 回転を更新（ロックオン中はプレイヤーへ向ける）
    /// </summary>
    void RotateUpdate();

    /// <summary>
    /// 地面との衝突判定・位置更新
    /// </summary>
    void CollisionGround();

    /// <summary>
    /// 正規化した移動方向を取得（プレースホルダ）
    /// </summary>
    Hagine::Vector3 GetMovementDirection() const;

    /// <summary>
    /// 速度の大きさを取得（プレースホルダ）
    /// </summary>
    float GetVelocityMagnitude() const;

    /// ===================================================
    /// Update から呼ぶ移動状態の小ステップ
    /// ===================================================

    /// <summary>移動・重力ごと完全停止させる（必殺技カメラワーク中）</summary>
    void Freeze();

    /// <summary>水平速度を0にする（ガード中）</summary>
    void StopHorizontal();

    /// <summary>速度イージングが有効なら水平速度へ適用する</summary>
    void UpdateVelocityEase(float deltaTime);

    /// <summary>重力を適用する（非接地かつ非飛行のときのみ）</summary>
    void ApplyGravity(float deltaTime);

    /// <summary>ダミーモードの摩擦減衰＋重力を適用する</summary>
    void ApplyDummyFriction(float deltaTime);

    /// <summary>ルートノードが無いときの停止処理</summary>
    void StopAll();

    /// <summary>速度イージングをキャンセルする（ノックバックが上書きされるのを防ぐ）</summary>
    void CancelVelocityEase() { velocityEase_.isActive = false; }

    /// <summary>速度・加速度・接地状態を初期化する（復活用）</summary>
    void ResetMotion();

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

    /// ===================================================
    /// Getter
    /// ===================================================
    Hagine::Vector3 &GetVelocity() { return velocity_; }
    Hagine::Vector3 &GetAcceleration() { return acceleration_; }
    float &GetFallSpeed() { return fallSpeed_; }
    float &GetMoveSpeed() { return moveSpeed_; }
    float &GetJumpSpeed() { return jumpSpeed_; }
    float &GetMaxSpeed() { return maxSpeed_; }
    float &GetAccelRate() { return accelRate_; }
    bool &GetCanJump() { return canJump_; }
    bool &GetIsGrounded() { return isGrounded_; }
    bool GetIsFlying() const { return isFlying_; }
    float GetVerticalVelocity() const { return velocity_.y; }
    float GetVerticalAcceleration() const { return acceleration_.y; }
    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetVelocity(const Hagine::Vector3 &vel) { velocity_ = vel; }
    void SetMoveSpeed(float speed) { moveSpeed_ = speed; }
    void SetStrafeDirection(int dir) { strafeDirection_ = dir; }
    void SetVerticalVelocity(float velocity) { velocity_.y = velocity; }
    void SetVerticalAcceleration(float accel) { acceleration_.y = accel; }
    void SetIsGrounded(bool grounded) { isGrounded_ = grounded; }
    void SetIsFlying(bool flying) { isFlying_ = flying; }

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 回転・ベクトル定数
    static constexpr float kRightVectorX = 1.0f;
    static constexpr float kRightVectorY = 0.0f;
    static constexpr float kRightVectorZ = 0.0f;
    static constexpr float kUpVectorX = 0.0f;
    static constexpr float kUpVectorY = 1.0f;
    static constexpr float kUpVectorZ = 0.0f;

    // 距離・閾値定数
    static constexpr float kMinRotationDistance = 0.001f;
    static constexpr float kParallelThreshold = 0.999f;
    static constexpr float kRotationSpeed = 8.0f;
    static constexpr float kGroundLevel = 0.0f;
    static constexpr float kVelocityZero = 0.0f;

    // イージング関連定数
    static constexpr float kVelocityEaseTime = 0.15f;
    static constexpr float kStopEaseTime = 0.2f;

    // ダミーモード(トレーニング)関連
    static constexpr float kDummyGroundFriction = 0.8f; // ノックバック後の水平減衰率(毎フレーム)

    Enemy *pOwner_ = nullptr; ///< 所有者の敵

    Direction dir_;         ///< 現在の方向
    MoveDirection moveDir_; ///< 移動方向

    Hagine::Vector3 velocity_{};     ///< 速度
    Hagine::Vector3 acceleration_{}; ///< 加速度

    int strafeDirection_ = 1; ///< 横移動方向

    float moveSpeed_ = 0.0f; ///< 移動速度
    float fallSpeed_ = 0.0f; ///< 落下速度
    float jumpSpeed_ = 0.0f; ///< ジャンプ速度
    float maxSpeed_ = 0.0f;  ///< 最大速度
    float accelRate_ = 0.0f; ///< 加速レート

    Hagine::Vector3 velocityTarget_{};                 ///< 目標速度
    Hagine::EasingData<Hagine::Vector3> velocityEase_; ///< 速度補間用イージング

    bool canJump_ = false;   ///< ジャンプ可能フラグ
    bool isGrounded_ = true; ///< 接地フラグ
    bool isFlying_ = false;  ///< 飛行中フラグ
};

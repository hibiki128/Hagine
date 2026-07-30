#define NOMINMAX
#include "EnemyMovement.h"
#include "Application/entity/enemy/Enemy.h"
#include "Application/entity/field/ground/Ground.h"
#include "Application/entity/player/Player.h"
#include <Frame.h>
#include <utility/debug/param/GameParamHub.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;

void EnemyMovement::Init(Enemy *pOwner)
{
    pOwner_ = pOwner;

    // 移動パラメータ
    moveSpeed_ = 5.0f;
    jumpSpeed_ = 15.0f;
    maxSpeed_ = 10.0f;
    accelRate_ = 1.0f;

    isGrounded_ = true;
    velocity_ = Vector3(0.0f, 0.0f, 0.0f);
    acceleration_ = Vector3(0.0f, 0.0f, 0.0f);
}

void EnemyMovement::MoveToTarget(const Vector3 &targetPos)
{
    if (!pOwner_->GetTarget())
        return;
    // ターゲットへの方向を計算
    Vector3 direction = targetPos - pOwner_->GetWorldTransform()->translation_;
    direction.y = 0;
    direction = direction.Normalize();
    // 目標速度を設定し、イージングを開始
    velocityTarget_ = direction * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void EnemyMovement::MoveStrafe()
{
    if (!pOwner_->GetTarget())
        return;
    // 横移動方向へ速度を設定
    Vector3 right = pOwner_->GetRight();
    velocityTarget_ = right * static_cast<float>(strafeDirection_) * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void EnemyMovement::MoveRetreat()
{
    if (!pOwner_->GetTarget())
        return;
    // ターゲットから離れる方向へ速度を設定
    Vector3 direction = pOwner_->GetWorldTransform()->translation_ - pOwner_->GetTarget()->GetWorldPosition();
    direction.y = 0;
    direction = direction.Normalize();
    velocityTarget_ = direction * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void EnemyMovement::StopMovement()
{
    // 停止目標速度を設定
    Vector3 zeroVel(0.0f, velocity_.y, 0.0f);
    velocityTarget_ = zeroVel;
    velocityEase_.Reset(velocity_, velocityTarget_, kStopEaseTime, EasingType::OutQuad);
}

void EnemyMovement::Move()
{
    if (!pOwner_->GetTarget())
        return;
}

void EnemyMovement::DirectionUpdate() {}

void EnemyMovement::RotateUpdate()
{
    if (!pOwner_->GetIsLockOn() || !pOwner_->GetTarget())
        return;

    WorldTransform *pTransform = pOwner_->GetWorldTransform();
    Vector3 toTarget = pOwner_->GetTarget()->GetWorldPosition() - pOwner_->GetWorldPosition();
    if (toTarget.Length() < kMinRotationDistance)
        return;

    toTarget = toTarget.Normalize();
    Vector3 forward = toTarget;
    Vector3 worldUp = kWorldUp;
    Vector3 right;

    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold)
    {
        right = kWorldRight;
    }
    else
    {
        right = (worldUp.Cross(forward)).Normalize();
    }

    Vector3 up = (forward.Cross(right)).Normalize();
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    pTransform->quaternionRotation_ = Quaternion::Slerp(
        pTransform->quaternionRotation_, targetRot, kRotationSpeed * Frame::DeltaTime());
}

void EnemyMovement::CollisionGround()
{
    WorldTransform *pTransform = pOwner_->GetWorldTransform();
    float nextY = pTransform->translation_.y + velocity_.y * Frame::DeltaTime();

    pTransform->translation_.x += velocity_.x * Frame::DeltaTime();
    pTransform->translation_.z += velocity_.z * Frame::DeltaTime();

    // 移動後のXZ位置における接地レベル（地形メッシュの表面高さ＋立ちオフセット）
    const float groundLevel = Ground::GetStandingY(pTransform->translation_.x, pTransform->translation_.z);

    if (isFlying_)
    {
        // 飛行中でも地形には潜らないよう接地レベルでクランプする
        pTransform->translation_.y = (std::max)(nextY, groundLevel);
        return;
    }

    if (nextY <= groundLevel)
    {
        pTransform->translation_.y = groundLevel;
        if (!isGrounded_)
        {
            velocity_.y = kVelocityZero;
            isGrounded_ = true;
        }
    }
    else if (isGrounded_ && velocity_.y <= kVelocityZero && nextY - groundLevel <= kGroundSnapDistance)
    {
        // 下り坂で毎フレーム接地が外れてガタつかないよう、僅かな段差は地面に吸着させる
        pTransform->translation_.y = groundLevel;
        velocity_.y = kVelocityZero;
    }
    else
    {
        pTransform->translation_.y = nextY;
        isGrounded_ = false;
    }
}

Vector3 EnemyMovement::GetMovementDirection() const { return Vector3(); }
float EnemyMovement::GetVelocityMagnitude() const { return kVelocityZero; }

void EnemyMovement::Freeze()
{
    // プレイヤーの必殺技カメラワーク中は移動・重力ごと完全停止させ、その場に固定する
    velocity_ = {0.0f, 0.0f, 0.0f};
    if (isGrounded_)
    {
        acceleration_.y = 0.0f;
    }
}

void EnemyMovement::StopHorizontal()
{
    // ガード中は移動させない（EnemyGuardNode が毎フレーム速度を0にしているため、
    // ここで移動イージングを適用すると追跡速度で上書きされて動いてしまう）
    velocity_.x = 0.0f;
    velocity_.z = 0.0f;
}

void EnemyMovement::UpdateVelocityEase(float deltaTime)
{
    // 速度イージングの更新（ガード中は適用しない）
    if (velocityEase_.isActive)
    {
        Vector3 easedVelocity = velocityEase_.Update(deltaTime);
        velocity_.x = easedVelocity.x;
        velocity_.z = easedVelocity.z;
    }
}

void EnemyMovement::ApplyGravity(float deltaTime)
{
    // 重力処理
    if (!isGrounded_ && !isFlying_)
    {
        velocity_.y += acceleration_.y * deltaTime;
    }
    else if (isGrounded_)
    {
        acceleration_.y = 0.0f;
    }
}

void EnemyMovement::ApplyPinchApproach(float /*deltaTime*/)
{
    if (!pinchAggressionEnabled_)
    {
        return;
    }
    Player *pTarget = pOwner_->GetTarget();
    if (!pTarget || !pTarget->GetAlive())
    {
        return;
    }

    // プレイヤーHP比率がピンチ閾値未満のときだけ発動する
    const float maxHP = pTarget->GetMaxHP();
    if (maxHP <= 0.0f)
    {
        return;
    }
    const float hpRatio = pTarget->GetHP() / maxHP;
    if (hpRatio >= pinchThreshold_)
    {
        return;
    }

    // XZ平面上の距離。近づき切っている（攻撃レンジ内）なら詰めを止めてBTに任せる
    Vector3 toTarget = pTarget->GetWorldPosition() - pOwner_->GetWorldTransform()->translation_;
    toTarget.y = 0.0f;
    const float distance = toTarget.Length();
    if (distance <= pinchCloseDistance_)
    {
        return;
    }

    // BTが後退・回り込みを選んでいても、プレイヤーへ向かう水平速度で上書きして強制接近する
    Vector3 dir = toTarget.Normalize();
    const float speed = moveSpeed_ * pinchSpeedMul_;
    velocity_.x = dir.x * speed;
    velocity_.z = dir.z * speed;
    // 追従が上書きされ続けないよう、速度イージングは畳んでおく
    velocityEase_.isActive = false;
}

void EnemyMovement::ApplyDummyFriction(float deltaTime)
{
    // ダミー: AIは動かさないが、被弾ノックバックは残す。
    // 水平速度に摩擦をかけて徐々に停止させ、重力だけ適用する。
    velocity_.x *= kDummyGroundFriction;
    velocity_.z *= kDummyGroundFriction;
    if (!isGrounded_)
    {
        velocity_.y += acceleration_.y * deltaTime;
    }
    else
    {
        acceleration_.y = 0.0f;
    }
}

void EnemyMovement::StopAll()
{
    // ルートノードがなければ停止
    velocity_.x = 0.0f;
    velocity_.z = 0.0f;
    if (isGrounded_)
    {
        velocity_.y = 0.0f;
        acceleration_.y = 0.0f;
    }
}

void EnemyMovement::ResetMotion()
{
    velocity_ = {0.0f, 0.0f, 0.0f};
    acceleration_ = {0.0f, 0.0f, 0.0f};
    isGrounded_ = true;
}

void EnemyMovement::RegisterParams()
{
    auto *pHub = GameParamHub::GetInstance();
    pHub->Register("Enemy", "移動速度", &moveSpeed_, {0.1f, 0.0f, 50.0f});
    pHub->Register("Enemy", "最大速度", &maxSpeed_, {0.1f, 0.0f, 50.0f});
    pHub->Register("Enemy", "加速率", &accelRate_, {0.1f, 0.0f, 50.0f});
    pHub->Register("Enemy", "ジャンプ速度", &jumpSpeed_, {0.1f, 0.0f, 50.0f});

    // ピンチ時の積極接近
    pHub->Register("Enemy", "ピンチ接近有効", &pinchAggressionEnabled_);
    pHub->Register("Enemy", "ピンチ判定HP比率", &pinchThreshold_, {0.01f, 0.0f, 1.0f});
    pHub->Register("Enemy", "ピンチ接近停止距離", &pinchCloseDistance_, {0.1f, 0.0f, 30.0f});
    pHub->Register("Enemy", "ピンチ接近速度倍率", &pinchSpeedMul_, {0.05f, 0.5f, 5.0f});
}

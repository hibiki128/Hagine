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

void EnemyMovement::Init(Enemy *owner)
{
    pOwner_ = owner;

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

    WorldTransform *transform = pOwner_->GetWorldTransform();
    Vector3 toTarget = pOwner_->GetTarget()->GetWorldPosition() - pOwner_->GetWorldPosition();
    if (toTarget.Length() < kMinRotationDistance)
        return;

    toTarget = toTarget.Normalize();
    Vector3 forward = toTarget;
    Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ};
    Vector3 right;

    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold)
    {
        right = {kRightVectorX, kRightVectorY, kRightVectorZ};
    }
    else
    {
        right = (worldUp.Cross(forward)).Normalize();
    }

    Vector3 up = (forward.Cross(right)).Normalize();
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    transform->quateRotation_ = Quaternion::Slerp(
        transform->quateRotation_, targetRot, kRotationSpeed * Frame::DeltaTime());
}

void EnemyMovement::CollisionGround()
{
    WorldTransform *transform = pOwner_->GetWorldTransform();
    float nextY = transform->translation_.y + velocity_.y * Frame::DeltaTime();

    transform->translation_.x += velocity_.x * Frame::DeltaTime();
    transform->translation_.z += velocity_.z * Frame::DeltaTime();

    // 移動後のXZ位置における接地レベル（地形メッシュの表面高さ＋立ちオフセット）
    const float groundLevel = Ground::GetStandingY(transform->translation_.x, transform->translation_.z);

    if (isFlying_)
    {
        // 飛行中でも地形には潜らないよう接地レベルでクランプする
        transform->translation_.y = (std::max)(nextY, groundLevel);
        return;
    }

    if (nextY <= groundLevel)
    {
        transform->translation_.y = groundLevel;
        if (!isGrounded_)
        {
            velocity_.y = kVelocityZero;
            isGrounded_ = true;
        }
    }
    else if (isGrounded_ && velocity_.y <= kVelocityZero && nextY - groundLevel <= kGroundSnapDistance)
    {
        // 下り坂で毎フレーム接地が外れてガタつかないよう、僅かな段差は地面に吸着させる
        transform->translation_.y = groundLevel;
        velocity_.y = kVelocityZero;
    }
    else
    {
        transform->translation_.y = nextY;
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
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Enemy", "移動速度", &moveSpeed_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "最大速度", &maxSpeed_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "加速率", &accelRate_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "ジャンプ速度", &jumpSpeed_, {0.1f, 0.0f, 50.0f});
}

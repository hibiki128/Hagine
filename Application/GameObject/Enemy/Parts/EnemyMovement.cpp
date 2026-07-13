#define NOMINMAX
#include "EnemyMovement.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include <Frame.h>
#include <Utility/Debug/GameParam/GameParamHub.h>
#include <cmath>

using namespace Hagine;

void EnemyMovement::Init(Enemy *owner) {
    owner_ = owner;

    // 移動パラメータ
    moveSpeed_ = 5.0f;
    jumpSpeed_ = 15.0f;
    maxSpeed_ = 10.0f;
    accelRate_ = 1.0f;

    isGrounded_ = true;
    velocity_ = Vector3(0.0f, 0.0f, 0.0f);
    acceleration_ = Vector3(0.0f, 0.0f, 0.0f);
}

void EnemyMovement::MoveToTarget(const Vector3 &targetPos) {
    if (!owner_->GetTarget())
        return;
    // ターゲットへの方向を計算
    Vector3 direction = targetPos - owner_->GetWorldTransform()->translation_;
    direction.y = 0;
    direction = direction.Normalize();
    // 目標速度を設定し、イージングを開始
    velocityTarget_ = direction * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void EnemyMovement::MoveStrafe() {
    if (!owner_->GetTarget())
        return;
    // 横移動方向へ速度を設定
    Vector3 right = owner_->GetRight();
    velocityTarget_ = right * (float)strafeDirection_ * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void EnemyMovement::MoveRetreat() {
    if (!owner_->GetTarget())
        return;
    // ターゲットから離れる方向へ速度を設定
    Vector3 direction = owner_->GetWorldTransform()->translation_ - owner_->GetTarget()->GetWorldPosition();
    direction.y = 0;
    direction = direction.Normalize();
    velocityTarget_ = direction * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void EnemyMovement::StopMovement() {
    // 停止目標速度を設定
    Vector3 zeroVel(0.0f, velocity_.y, 0.0f);
    velocityTarget_ = zeroVel;
    velocityEase_.Reset(velocity_, velocityTarget_, kStopEaseTime, EasingType::OutQuad);
}

void EnemyMovement::Move() {
    if (!owner_->GetTarget())
        return;
}

void EnemyMovement::DirectionUpdate() {}

void EnemyMovement::RotateUpdate() {
    if (!owner_->GetIsLockOn() || !owner_->GetTarget())
        return;

    WorldTransform *transform = owner_->GetWorldTransform();
    Vector3 toTarget = owner_->GetTarget()->GetWorldPosition() - owner_->GetWorldPosition();
    if (toTarget.Length() < kMinRotationDistance)
        return;

    toTarget = toTarget.Normalize();
    Vector3 forward = toTarget;
    Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ};
    Vector3 right;

    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
        right = {kRightVectorX, kRightVectorY, kRightVectorZ};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }

    Vector3 up = (forward.Cross(right)).Normalize();
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    transform->quateRotation_ = Quaternion::Slerp(
        transform->quateRotation_, targetRot, kRotationSpeed * Frame::DeltaTime());
}

void EnemyMovement::CollisionGround() {
    WorldTransform *transform = owner_->GetWorldTransform();
    float nextY = transform->translation_.y + velocity_.y * Frame::DeltaTime();

    transform->translation_.x += velocity_.x * Frame::DeltaTime();
    transform->translation_.z += velocity_.z * Frame::DeltaTime();

    if (isFlying_) {
        transform->translation_.y = nextY;
        return;
    }

    if (nextY <= kGroundLevel) {
        transform->translation_.y = kGroundLevel;
        if (!isGrounded_) {
            velocity_.y = kVelocityZero;
            isGrounded_ = true;
        }
    } else {
        transform->translation_.y = nextY;
        isGrounded_ = false;
    }
}

Vector3 EnemyMovement::GetMovementDirection() const { return Vector3(); }
float EnemyMovement::GetVelocityMagnitude() const { return kVelocityZero; }

void EnemyMovement::Freeze() {
    // プレイヤーの必殺技カメラワーク中は移動・重力ごと完全停止させ、その場に固定する
    velocity_ = {0.0f, 0.0f, 0.0f};
    if (isGrounded_) {
        acceleration_.y = 0.0f;
    }
}

void EnemyMovement::StopHorizontal() {
    // ガード中は移動させない（EnemyGuardNode が毎フレーム速度を0にしているため、
    // ここで移動イージングを適用すると追跡速度で上書きされて動いてしまう）
    velocity_.x = 0.0f;
    velocity_.z = 0.0f;
}

void EnemyMovement::UpdateVelocityEase(float deltaTime) {
    // 速度イージングの更新（ガード中は適用しない）
    if (velocityEase_.isActive) {
        Vector3 easedVelocity = velocityEase_.Update(deltaTime);
        velocity_.x = easedVelocity.x;
        velocity_.z = easedVelocity.z;
    }
}

void EnemyMovement::ApplyGravity(float deltaTime) {
    // 重力処理
    if (!isGrounded_ && !isFlying_) {
        velocity_.y += acceleration_.y * deltaTime;
    } else if (isGrounded_) {
        acceleration_.y = 0.0f;
    }
}

void EnemyMovement::ApplyDummyFriction(float deltaTime) {
    // ダミー: AIは動かさないが、被弾ノックバックは残す。
    // 水平速度に摩擦をかけて徐々に停止させ、重力だけ適用する。
    velocity_.x *= kDummyGroundFriction;
    velocity_.z *= kDummyGroundFriction;
    if (!isGrounded_) {
        velocity_.y += acceleration_.y * deltaTime;
    } else {
        acceleration_.y = 0.0f;
    }
}

void EnemyMovement::StopAll() {
    // ルートノードがなければ停止
    velocity_.x = 0.0f;
    velocity_.z = 0.0f;
    if (isGrounded_) {
        velocity_.y = 0.0f;
        acceleration_.y = 0.0f;
    }
}

void EnemyMovement::ResetMotion() {
    velocity_ = {0.0f, 0.0f, 0.0f};
    acceleration_ = {0.0f, 0.0f, 0.0f};
    isGrounded_ = true;
}

void EnemyMovement::RegisterParams() {
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Enemy", "移動速度", &moveSpeed_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "最大速度", &maxSpeed_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "加速率", &accelRate_, {0.1f, 0.0f, 50.0f});
    hub->Register("Enemy", "ジャンプ速度", &jumpSpeed_, {0.1f, 0.0f, 50.0f});
}

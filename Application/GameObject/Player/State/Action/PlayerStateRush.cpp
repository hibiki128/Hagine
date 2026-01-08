#include "PlayerStateRush.h"
#include "Input.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <Particle/ParticleEditor.h>

void PlayerStateRush::Enter(Player &player) {
    Enemy *enemy = player.GetEnemy();
    if (enemy && player.GetIsLockOn()) {
        targetPosition_ = enemy->GetPositionBehind(distance_);
        startPosition_ = player.GetTransform().translation_;

        CalculateArcPath(startPosition_, targetPosition_, player);

        isRushing_ = true;
        elapsedTime_ = kVelocityZero;

        player.GetVelocity().y = kVelocityZero;
    }
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(&player.GetViewProjection(), "RushShake");
    shake_->StartShake();
    rushEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("RushEmitter");

    rushEmitter_->SetStartRotate("Burst", player.GetWorldRotation().ToEulerDegrees());
    rushEmitter_->SetEndRotate("Burst", player.GetWorldRotation().ToEulerDegrees());
    rushEmitter_->SetPosition(player.GetWorldPosition());
    rushEmitter_->UpdateOnce();
}

void PlayerStateRush::Update(Player &player) {
    if (!isRushing_) {
        player.ChangeState("FlyIdle");
        return;
    }

    if (CheckExitInput()) {
        player.ChangeState("FlyIdle");
        return;
    }

    elapsedTime_ += player.GetDt();

    if (CheckReachedTarget(player)) {
        FinishRush(player);
        return;
    }

    UpdateMovement(player);
    UpdateRotation(player);
    shake_->Update();
}

void PlayerStateRush::Exit(Player &player) {
    isRushing_ = false;
    elapsedTime_ = kVelocityZero;
    player.GetVelocity() = {kVelocityZero, kVelocityZero, kVelocityZero};
    player.ResetControlCount();
}

void PlayerStateRush::DrawParticle(Player &player, const ViewProjection &viewProjection) {
    if (rushEmitter_) {
        rushEmitter_->Draw(viewProjection);
    }
}

Quaternion PlayerStateRush::LookRotation(const Vector3 &forward, const Vector3 &up) {
    Vector3 f = forward.Normalize();
    Vector3 u = up.Normalize();
    Vector3 r = u.Cross(f);
    u = f.Cross(r);

    Matrix4x4 rotationMatrix = MakeIdentity4x4();
    rotationMatrix.m[kMatrixRow0][kMatrixCol0] = r.x;
    rotationMatrix.m[kMatrixRow0][kMatrixCol1] = r.y;
    rotationMatrix.m[kMatrixRow0][kMatrixCol2] = r.z;
    rotationMatrix.m[kMatrixRow1][kMatrixCol0] = u.x;
    rotationMatrix.m[kMatrixRow1][kMatrixCol1] = u.y;
    rotationMatrix.m[kMatrixRow1][kMatrixCol2] = u.z;
    rotationMatrix.m[kMatrixRow2][kMatrixCol0] = f.x;
    rotationMatrix.m[kMatrixRow2][kMatrixCol1] = f.y;
    rotationMatrix.m[kMatrixRow2][kMatrixCol2] = f.z;

    return Quaternion::FromMatrix(rotationMatrix);
}

void PlayerStateRush::CalculateArcPath(const Vector3 &startPos, const Vector3 &targetPos, Player &player) {
    Enemy *enemy = player.GetEnemy();
    Vector3 enemyPos = enemy->GetTransform().translation_;

    Vector3 toTarget = targetPos - startPos;
    float distance = toTarget.Length();

    Vector3 midPoint = (startPos + targetPos) * kMidPointFactor;

    Vector3 enemyRight = enemy->GetRight();
    Vector3 enemyUp = enemy->GetUp();
    Vector3 enemyForward = enemy->GetForward();

    Vector3 playerToEnemy = (enemyPos - startPos).Normalize();

    float rightDot = playerToEnemy.Dot(enemyRight);
    float upDot = playerToEnemy.Dot(enemyUp);

    float arcHeight = distance * kArcHeightFactor;
    float sideOffset = distance * kSideOffsetFactor;

    Vector3 controlDirection = Vector3(kVectorZero, kVectorZero, kVectorZero);

    controlDirection += enemyRight * (rightDot > kVectorZero ? -sideOffset : sideOffset);

    if (upDot > kUpDotHighThreshold) {
        controlDirection += enemyUp * (arcHeight * kUpHeightLowMultiplier);
    } else if (upDot < kUpDotLowThreshold) {
        controlDirection += enemyUp * (arcHeight * kUpHeightHighMultiplier);
    } else {
        controlDirection += enemyUp * arcHeight;
    }

    Vector3 startToTarget = (targetPos - startPos).Normalize();
    Vector3 forwardComponent = enemyForward * (startToTarget.Dot(enemyForward));
    controlDirection += forwardComponent * (distance * kForwardOffsetFactor);

    arcControlPoint_ = midPoint + controlDirection;

    arcLength_ = (startPos - arcControlPoint_).Length() + (arcControlPoint_ - targetPos).Length();
}

Vector3 PlayerStateRush::GetArcPosition(float progress) {
    float t = progress;
    float oneMinusT = kMaxProgress - t;

    Vector3 result = oneMinusT * oneMinusT * startPosition_ +
                     kTwoFactor * oneMinusT * t * arcControlPoint_ +
                     t * t * targetPosition_;

    return result;
}

bool PlayerStateRush::CheckExitInput() {
    return Input::GetInstance()->TriggerKey(DIK_X);
}

bool PlayerStateRush::CheckReachedTarget(Player &player) {
    Vector3 currentPos = player.GetTransform().translation_;
    float distanceToTarget = (targetPosition_ - currentPos).Length();
    return distanceToTarget <= arrivalDistance_;
}

void PlayerStateRush::FinishRush(Player &player) {
    isRushing_ = false;
    player.GetVelocity() = {kVelocityZero, kVelocityZero, kVelocityZero};
    player.GetWorldPosition() = targetPosition_;
    player.ChangeState("FlyIdle");
}

void PlayerStateRush::UpdateMovement(Player &player) {
    float progress = (elapsedTime_ * rushSpeed_) / arcLength_;
    if (progress > kMaxProgress)
        progress = kMaxProgress;

    rushDirection_ = CalculateMovementDirection(progress, player);

    player.GetVelocity() = rushDirection_ * rushSpeed_;
}

Vector3 PlayerStateRush::CalculateMovementDirection(float progress, Player &player) {
    Vector3 arcPos = GetArcPosition(progress);

    float nextProgress = progress + kProgressIncrement;
    if (nextProgress > kMaxProgress)
        nextProgress = kMaxProgress;
    Vector3 nextPos = GetArcPosition(nextProgress);

    Vector3 direction = (nextPos - arcPos).Normalize();

    if (progress > blendStartProgress_) {
        Vector3 currentPos = player.GetTransform().translation_;
        Vector3 directDirection = (targetPosition_ - currentPos).Normalize();
        float blendFactor = (progress - blendStartProgress_) / (kMaxProgress - blendStartProgress_);
        direction = ((kMaxProgress - blendFactor) * direction + blendFactor * directDirection).Normalize();
    }

    return direction;
}

void PlayerStateRush::UpdateRotation(Player &player) {
    if (rushDirection_.Length() > kVectorZero) {
        Vector3 forward = rushDirection_;
        Vector3 up = {kVectorZero, kUpVectorY, kVectorZero};

        Quaternion targetRotation = LookRotation(forward, up);
        player.GetWorldRotation() = Slerp(
            player.GetTransform().quateRotation_,
            targetRotation,
            rotationSpeed_ * player.GetDt());
    }
}
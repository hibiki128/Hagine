#include "PlayerStateRush.h"
#include "Input.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"

void PlayerStateRush::Enter(Player &player) {
    Enemy *enemy = player.GetEnemy();
    if (enemy && player.GetIsLockOn()) {
        // ターゲットの後ろの位置を取得
        targetPosition_ = enemy->GetPositionBehind(distance_);
        startPosition_ = player.GetTransform().translation_;

        CalculateArcPath(startPosition_, targetPosition_, player);

        isRushing_ = true;
        elapsedTime_ = 0.0f;

        player.GetVelocity().y = 0.0f;
    }
    shake_ = std::make_unique<Shake>();
    shake_->Initialize(&player.GetViewProjection(), "RushShake");
    shake_->StartShake();
}

void PlayerStateRush::Update(Player &player) {
    if (!isRushing_) {
        player.ChangeState("FlyIdle");
        return;
    }

    // 離脱ボタンチェック
    if (CheckExitInput()) {
        player.ChangeState("FlyIdle");
        return;
    }

    elapsedTime_ += player.GetDt();

    // 目標地点に到達したかチェック
    if (CheckReachedTarget(player)) {
        FinishRush(player);
        return;
    }

    // 移動処理
    UpdateMovement(player);

    // 回転処理
    UpdateRotation(player);

    shake_->Update();
}

void PlayerStateRush::Exit(Player &player) {
    isRushing_ = false;
    elapsedTime_ = 0.0f;
    player.GetVelocity() = {0.0f, 0.0f, 0.0f};
    player.ResetControlCount();
}

Quaternion PlayerStateRush::LookRotation(const Vector3 &forward, const Vector3 &up) {
    Vector3 f = forward.Normalize();
    Vector3 u = up.Normalize();
    Vector3 r = u.Cross(f);
    u = f.Cross(r);

    // 回転行列から四元数を計算
    Matrix4x4 rotationMatrix = MakeIdentity4x4();
    rotationMatrix.m[0][0] = r.x;
    rotationMatrix.m[0][1] = r.y;
    rotationMatrix.m[0][2] = r.z;
    rotationMatrix.m[1][0] = u.x;
    rotationMatrix.m[1][1] = u.y;
    rotationMatrix.m[1][2] = u.z;
    rotationMatrix.m[2][0] = f.x;
    rotationMatrix.m[2][1] = f.y;
    rotationMatrix.m[2][2] = f.z;

    return Quaternion::FromMatrix(rotationMatrix);
}

void PlayerStateRush::CalculateArcPath(const Vector3 &startPos, const Vector3 &targetPos, Player &player) {
    Enemy *enemy = player.GetEnemy();
    Vector3 enemyPos = enemy->GetTransform().translation_;

    // スタートからターゲットへの直線距離
    Vector3 toTarget = targetPos - startPos;
    float distance = toTarget.Length();

    // 弧の制御点を計算
    Vector3 midPoint = (startPos + targetPos) * 0.5f;

    // 敵の方向ベクトルを取得
    Vector3 enemyRight = enemy->GetRight();
    Vector3 enemyUp = enemy->GetUp();
    Vector3 enemyForward = enemy->GetForward();

    // プレイヤーの開始位置から敵への方向を分析
    Vector3 playerToEnemy = (enemyPos - startPos).Normalize();

    // 敵の右方向との内積で、プレイヤーが敵のどちら側にいるかを判定
    float rightDot = playerToEnemy.Dot(enemyRight);
    // 敵の上方向との内積で、プレイヤーが敵の上下どちらにいるかを判定
    float upDot = playerToEnemy.Dot(enemyUp);

    // 弧の高さと横方向のオフセットを設定
    float arcHeight = distance * 0.25f;
    float sideOffset = distance * 0.4f;

    // プレイヤーの位置に応じて制御点の方向を決定
    Vector3 controlDirection = Vector3(0.0f, 0.0f, 0.0f);

    // 横方向のオフセット（プレイヤーが左にいるなら右に、右にいるなら左に）
    controlDirection += enemyRight * (rightDot > 0 ? -sideOffset : sideOffset);

    // 縦方向のオフセット
    if (upDot > 0.3f) {
        // プレイヤーが敵の上方にいる場合は、あまり上に行かない
        controlDirection += enemyUp * (arcHeight * 0.3f);
    } else if (upDot < -0.3f) {
        // プレイヤーが敵の下方にいる場合は、上に大きく弧を描く
        controlDirection += enemyUp * (arcHeight * 1.2f);
    } else {
        // プレイヤーが敵と同じ高さにいる場合は、適度に上に
        controlDirection += enemyUp * arcHeight;
    }

    // 前後方向にも少しオフセットを追加
    Vector3 startToTarget = (targetPos - startPos).Normalize();
    Vector3 forwardComponent = enemyForward * (startToTarget.Dot(enemyForward));
    controlDirection += forwardComponent * (distance * 0.2f);

    // 制御点を設定
    arcControlPoint_ = midPoint + controlDirection;

    // 弧の総長を概算
    arcLength_ = (startPos - arcControlPoint_).Length() + (arcControlPoint_ - targetPos).Length();
}

Vector3 PlayerStateRush::GetArcPosition(float progress) {
  float t = progress;
    float oneMinusT = 1.0f - t;

    Vector3 result = oneMinusT * oneMinusT * startPosition_ +
                     2.0f * oneMinusT * t * arcControlPoint_ +
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
    player.GetVelocity() = {0.0f, 0.0f, 0.0f};
    player.GetWorldPosition() = targetPosition_;
    player.ChangeState("FlyIdle");
}

void PlayerStateRush::UpdateMovement(Player &player) {
    // 弧の進行を時間ベースで計算
    float progress = (elapsedTime_ * rushSpeed_) / arcLength_;
    if (progress > 1.0f)
        progress = 1.0f;

    // 移動方向を計算
    rushDirection_ = CalculateMovementDirection(progress, player);

    // 高速移動
    player.GetVelocity() = rushDirection_ * rushSpeed_;
}

Vector3 PlayerStateRush::CalculateMovementDirection(float progress, Player &player) {
    // 弧上の現在位置と次の位置を計算
    Vector3 arcPos = GetArcPosition(progress);

    // 次の位置を計算して移動方向を求める
    float nextProgress = progress + 0.01f;
    if (nextProgress > 1.0f)
        nextProgress = 1.0f;
    Vector3 nextPos = GetArcPosition(nextProgress);

    // 移動方向を計算
    Vector3 direction = (nextPos - arcPos).Normalize();

    if (progress > blendStartProgress_) {
        Vector3 currentPos = player.GetTransform().translation_;
        Vector3 directDirection = (targetPosition_ - currentPos).Normalize();
        float blendFactor = (progress - blendStartProgress_) / (1.0f - blendStartProgress_);
        direction = ((1.0f - blendFactor) * direction + blendFactor * directDirection).Normalize();
    }

    return direction;
}

void PlayerStateRush::UpdateRotation(Player &player) {
    if (rushDirection_.Length() > 0.0f) {
        Vector3 forward = rushDirection_;
        Vector3 up = {0.0f, 1.0f, 0.0f};

        Quaternion targetRotation = LookRotation(forward, up);
        player.GetWorldRotation() = Slerp(
            player.GetTransform().quateRotation_,
            targetRotation,
            rotationSpeed_ * player.GetDt());
    }
}
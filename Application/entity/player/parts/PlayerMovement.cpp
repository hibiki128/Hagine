#define NOMINMAX
#include "PlayerMovement.h"
#include <Application/entity/enemy/Enemy.h>
#include <Application/entity/field/ground/Ground.h>
#include <Application/entity/player/Player.h>
#include <Application/camera/follow/FollowCamera.h>
#include <data/DataHandler.h>
#include <utility/debug/param/GameParamHub.h>
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Hagine;

void PlayerMovement::Init(Player *pOwner)
{
    pOwner_ = pOwner;
    isGrounded_ = true; // 初期状態は地面にいる
}

void PlayerMovement::Move()
{
    float xInput = kInputZero;
    float zInput = kInputZero;

    GamePad *pGamePad = pOwner_->GetGamePad();
    Input *pInput = pOwner_->GetInput();
    const float dt = pOwner_->GetDt();

    // ─── 近接コンボ中はその場で攻撃する（移動入力を受け付けない）───
    // 射撃は移動しながら撃てるが、近接は踏み込んで殴る動きのため移動と噛み合わない。
    // 入力を無視するだけだと直前の速度で滑り続けるので、水平速度も減衰させる
    if (pOwner_->Combat().GetPunchCombo().IsComboActive())
    {
        velocity_.x *= kDecelerationFactor;
        velocity_.z *= kDecelerationFactor;
        if (std::abs(velocity_.x) < kVelocityStopThreshold)
            velocity_.x = kVelocityZero;
        if (std::abs(velocity_.z) < kVelocityStopThreshold)
            velocity_.z = kVelocityZero;

        // ダッシュ中に殴り始めたとき、攻撃後もダッシュ状態が残らないよう解除する
        ClearDashState();
        return;
    }

    if (!pGamePad->IsConnected())
    {
        // キーボード入力
        if (pInput->PushKey(DIK_A))
            xInput += kInputValue;
        if (pInput->PushKey(DIK_D))
            xInput -= kInputValue;
        if (pInput->PushKey(DIK_W))
            zInput += kInputValue;
        if (pInput->PushKey(DIK_S))
            zInput -= kInputValue;
        // ダッシュは「Ctrl を押しながら移動しているとき」だけ有効。
        // Ctrl を放した時点、または移動入力が無くなった時点でダッシュ解除する
        // （その場に立ち止まったまま Ctrl 押しっぱなしで演出が続くのを防ぐ）。
        const bool hasMoveInput = (xInput != kInputZero || zInput != kInputZero);
        isDashing_ = pInput->PushKey(DIK_LCONTROL) && hasMoveInput;
        if (isDashing_ && !wasDashing_)
        {
            pOwner_->EmitAction(Player::ActionKind::Dash); // 入力表示UI用：ダッシュ開始を通知
        }
        wasDashing_ = isDashing_; // キーボードは UpdateDashState が早期returnするためここで更新
    }
    else
    {
        // ゲームパッド入力
        xInput = -pGamePad->GetLeftStickX(); // 左スティックX軸
        zInput = pGamePad->GetLeftStickY();  // 左スティックY軸

        // 左スティックが倒れているか（ダッシュ維持の判定に使う）
        bool hasStickInput = (xInput != kInputZero || zInput != kInputZero);

        // Aボタンのトリガーでダッシュ開始（LTは不要）。
        // すでにダッシュ中の場合は再始動しないため、ダッシュ中のA入力は
        // Rush 側のトリガーとして扱われる（PlayerStateFly* の TryChangeToRush）。
        // 開始判定は Idle 系ステートと共通化するため TryStartGamepadDash に集約している。
        TryStartGamepadDash();

        // 猶予中にスティックを倒したらダッシュを確定（以降は通常の維持判定に従う）。
        // ニュートラルのままなら猶予時間をカウントダウンする。
        if (isDashing_ && hasStickInput)
        {
            dashGraceTimer_ = 0.0f;
        }
        else if (isDashing_ && dashGraceTimer_ > 0.0f)
        {
            dashGraceTimer_ -= dt;
        }

        // スティックをニュートラルに戻したらダッシュ解除。
        // ただし開始フレームと猶予時間中（A押下→移動待ち）はニュートラルでも維持する。
        if (isDashing_ && !hasStickInput && !dashStartedThisFrame_ && dashGraceTimer_ <= 0.0f)
        {
            isDashing_ = false;
            dashInputX_ = kInputZero;
            dashInputZ_ = kInputZero;
            dashDuration_ = 0.0f;
            dashGraceTimer_ = 0.0f;
        }
    }

    // 入力がない場合の減速処理
    if (xInput == kInputZero && zInput == kInputZero && !isDashing_)
    {
        velocity_.x *= kDecelerationFactor;
        velocity_.z *= kDecelerationFactor;
        if (std::abs(velocity_.x) < kVelocityStopThreshold)
            velocity_.x = kVelocityZero;
        if (std::abs(velocity_.z) < kVelocityStopThreshold)
            velocity_.z = kVelocityZero;
        return;
    }

    // カメラの方向ベクトル取得
    FollowCamera *pCamera = pOwner_->GetCamera();
    if (!pCamera)
        return;

    float yaw = pCamera->GetYaw();
    Vector3 cameraForward = {std::sin(yaw), kYComponentZero, std::cos(yaw)};
    Vector3 cameraRight = {-std::cos(yaw), kYComponentZero, std::sin(yaw)};

    Vector3 moveDir = cameraRight * xInput + cameraForward * zInput;

    // ダッシュ開始時のスティック入力がない場合、敵または自機正面方向へ移動
    if (dashStartedThisFrame_ && dashInputX_ == kInputZero && dashInputZ_ == kInputZero)
    {
        if (pOwner_->GetEnemy())
        {
            Vector3 toEnemy = pOwner_->GetEnemy()->GetWorldPosition() - pOwner_->GetWorldPosition();
            toEnemy.y = 0;
            if (toEnemy.Length() > 0.001f)
            {
                moveDir = toEnemy.Normalize();
            }
        }
        else
        {
            moveDir = pOwner_->GetForward();
            moveDir.y = 0;
            moveDir = moveDir.Normalize();
        }
    }
    else if (moveDir.Length() > 0.001f)
    {
        moveDir = moveDir.Normalize();
    }

    // ロックオン中でない場合、移動方向に向けて回転
    if (!pOwner_->GetIsLockOn() && moveDir.Length() > 0.001f)
    {
        float targetYaw = std::atan2(-moveDir.x, moveDir.z);
        Quaternion targetRot = Quaternion::FromEulerAngles({0.0f, targetYaw, 0.0f});
        float rotateSpeed = kPlayerRotationSpeed;
        pOwner_->GetLocalRotation() = Quaternion::Slerp(pOwner_->GetLocalRotation(), targetRot, rotateSpeed * dt);
    }

    // ダッシュ中は最大速度を倍増
    float currentMaxSpeed = isDashing_ ? maxSpeed_ * kDashSpeedMultiplier : maxSpeed_;

    velocity_.x += moveDir.x * accelRate_ * dt;
    velocity_.z += moveDir.z * accelRate_ * dt;

    // 最高速度制限
    float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
    if (speed > currentMaxSpeed)
    {
        float scale = currentMaxSpeed / speed;
        velocity_.x *= scale;
        velocity_.z *= scale;
    }
}

bool PlayerMovement::TryStartGamepadDash()
{
    GamePad *pGamePad = pOwner_->GetGamePad();

    // ゲームパッド未接続、既にダッシュ中、または近接コンボ中は開始しない
    // （Move() 側もコンボ中はダッシュを開始せず ClearDashState する挙動に合わせる）。
    if (!pGamePad || !pGamePad->IsConnected() || isDashing_)
    {
        return false;
    }
    if (pOwner_->Combat().GetPunchCombo().IsComboActive())
    {
        return false;
    }
    if (!pGamePad->IsTrigger(XINPUT_GAMEPAD_A))
    {
        return false;
    }

    const float xInput = -pGamePad->GetLeftStickX();
    const float zInput = pGamePad->GetLeftStickY();
    const bool hasStickInput = (xInput != kInputZero || zInput != kInputZero);

    dashInputX_ = xInput;
    dashInputZ_ = zInput;
    isDashing_ = true;
    dashStartedThisFrame_ = true;
    dashDuration_ = 0.0f;
    // A押下時にスティックがニュートラルなら、猶予時間内に倒せばダッシュ継続を許可。
    // 既にスティックを倒していれば従来通り即ダッシュ確定（猶予不要）。
    dashGraceTimer_ = hasStickInput ? 0.0f : kDashGraceTime;
    pOwner_->EmitAction(Player::ActionKind::Dash); // 入力表示UI用：ダッシュ開始を通知
    return true;
}

void PlayerMovement::DirectionUpdate()
{
    GamePad *pGamePad = pOwner_->GetGamePad();
    Input *pInput = pOwner_->GetInput();

    if (!pGamePad->IsConnected())
    {
        // キーボード入力
        if (pInput->PushKey(DIK_D))
        {
            moveDir_ = MoveDirection::Right;
        }
        else if (pInput->PushKey(DIK_A))
        {
            moveDir_ = MoveDirection::Left;
        }
        else if (pInput->PushKey(DIK_W))
        {
            moveDir_ = MoveDirection::Forward;
        }
        else if (pInput->PushKey(DIK_S))
        {
            moveDir_ = MoveDirection::Behind;
        }
    }
    else
    {
        // ゲームパッド入力 - 左スティック
        // 方向分類用の X はスティックの符号そのままを使う（右スティック→右アニメ）。
        // Move() の移動計算では cameraRight が -X 基準のため符号を反転しているが、
        // ここは方向分類なので反転しないことでキーボード(D=Right)と左右を一致させる
        float xInput = pGamePad->GetLeftStickX(); // 左スティックX軸
        float zInput = pGamePad->GetLeftStickY(); // 左スティックY軸

        // スティック入力から方向を判定
        if (xInput != 0.0f || zInput != 0.0f)
        {
            float angle = std::atan2(xInput, zInput);

            const float kPi = std::numbers::pi_v<float>;
            const float segment = kPi / 4.0f; // 45度

            if (angle >= -segment && angle < segment)
            {
                moveDir_ = MoveDirection::Forward;
            }
            else if (angle >= segment && angle < segment * 3.0f)
            {
                moveDir_ = MoveDirection::Right;
            }
            else if (angle >= segment * 3.0f || angle < -segment * 3.0f)
            {
                moveDir_ = MoveDirection::Behind;
            }
            else if (angle >= -segment * 3.0f && angle < -segment)
            {
                moveDir_ = MoveDirection::Left;
            }
        }
    }

    // 向いてる方向は回転値から計算(ロックオン時以外)
    if (!pOwner_->GetIsLockOn())
    {
        dir_ = CalculateDirectionFromRotation();
    }
    else
    {
        dir_ = Direction::Forward;
    }
}

void PlayerMovement::RotateUpdate()
{
    GamePad *pGamePad = pOwner_->GetGamePad();
    Input *pInput = pOwner_->GetInput();
    const float dt = pOwner_->GetDt();

    if (pOwner_->GetIsLockOn() && pOwner_->GetEnemy())
    {
        Vector3 toEnemy = pOwner_->GetEnemy()->GetWorldPosition() - pOwner_->GetWorldPosition();
        if (toEnemy.Length() > kMinRotationDistance)
        {
            toEnemy = toEnemy.Normalize();

            // プレイヤーの正面方向(+Z方向)を敵の方向に向ける
            Vector3 forward = toEnemy;
            Vector3 worldUp = kWorldUp;

            // forwardとworldUpが平行になる場合の対処
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

            // 回転行列から目標クォータニオンを作成
            Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
            Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

            float rotateSpeed = kPlayerRotationSpeed;
            pOwner_->GetLocalRotation() = Quaternion::Slerp(pOwner_->GetLocalRotation(), targetRot, rotateSpeed * dt);
        }
    }
    else
    {
        if (!pGamePad->IsConnected())
        {
            // キーボード入力（左右矢印キーで手動回転）
            Vector3 euler = pOwner_->GetLocalRotation().ToEulerAngles();
            bool rotationChanged = false;
            if (pInput->PushKey(DIK_RIGHT))
            {
                euler.y -= kManualRotationSpeed;
                rotationChanged = true;
            }
            if (pInput->PushKey(DIK_LEFT))
            {
                euler.y += kManualRotationSpeed;
                rotationChanged = true;
            }
            if (rotationChanged)
            {
                pOwner_->GetLocalRotation() = Quaternion::FromEulerAngles(euler);
            }
        }
        else
        {
            // ゲームパッド入力（右スティックX軸で左右回転）
            float rightStickX = pGamePad->GetRightStickX();
            float rightStickY = pGamePad->GetRightStickY();

            if (rightStickX != 0.0f || rightStickY != 0.0f)
            {
                Vector3 euler = pOwner_->GetLocalRotation().ToEulerAngles();
                euler.y += rightStickX * kManualRotationSpeed * 2.0f;
                pOwner_->GetLocalRotation() = Quaternion::FromEulerAngles(euler);
            }
        }
    }
}

void PlayerMovement::FaceTargetInstant(const Vector3 &targetPos)
{
    // RotateUpdate のロックオン追従と同じ計算を、補間なし（即時）で行う
    Vector3 toTarget = targetPos - pOwner_->GetWorldPosition();
    if (toTarget.Length() < kMinRotationDistance)
    {
        return;
    }
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
    pOwner_->GetLocalRotation() = Quaternion::FromMatrix(rotMatrix);
}

void PlayerMovement::StartMeleeLunge()
{
    // 既定は自分の正面方向（敵がいない・ロックオンしていない場合）
    Vector3 dir = pOwner_->GetForward();
    dir.y = kYComponentZero;

    if (Enemy *pEnemy = pOwner_->GetEnemy())
    {
        Vector3 toEnemy = pEnemy->GetWorldPosition() - pOwner_->GetWorldPosition();
        toEnemy.y = kYComponentZero;
        const float distance = toEnemy.Length();

        // 密着状態でさらに踏み込むと相手を押し込んだりすり抜けたりするので何もしない
        if (distance <= meleeLungeMinDistance_)
        {
            return;
        }
        dir = toEnemy / distance;
    }

    if (dir.Length() < kMinRotationDistance)
    {
        return;
    }
    dir = dir.Normalize();

    // 水平速度を踏み込みの初速で上書きする。
    // このあとは Move()（コンボ中）や Idle ステートの減衰で数フレームかけて止まる
    velocity_.x = dir.x * meleeLungeSpeed_;
    velocity_.z = dir.z * meleeLungeSpeed_;
}

void PlayerMovement::CollisionGround()
{
    const float dt = pOwner_->GetDt();

    // 下方向の速度を制限
    if (velocity_.y < kMaxFallVelocity)
    {
        velocity_.y = kMaxFallVelocity;
    }

    // 位置更新前に次フレームのY座標を計算
    float nextY = pOwner_->GetLocalPosition().y + velocity_.y * dt;

    pOwner_->GetLocalPosition().x += velocity_.x * dt;
    pOwner_->GetLocalPosition().z += velocity_.z * dt;

    // 移動後のXZ位置における接地レベル（地形メッシュの表面高さ＋立ちオフセット）
    const float groundLevel = Ground::GetStandingY(pOwner_->GetLocalPosition().x, pOwner_->GetLocalPosition().z);

    if (nextY <= groundLevel)
    {
        // Rush状態の場合は地面から押し戻す（地面に埋まらないよう浮かせる）
        if (pOwner_->GetCurrentStateName() == "Rush")
        {
            pOwner_->GetLocalPosition().y = groundLevel + kRushGroundOffset;
            velocity_.y = kVelocityZero;
            return;
        }

        // 地面に接地（Rush 以外）
        pOwner_->GetLocalPosition().y = groundLevel;
        if (!isGrounded_)
        {
            velocity_.y = kVelocityZero;
            isGrounded_ = true;
            // 空中からの着地で水平速度に応じて状態遷移
            if (pOwner_->GetCurrentStateName() == "Air")
            {
                float horizontalSpeed = sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
                if (horizontalSpeed > kLandingSpeedThreshold)
                {
                    pOwner_->ChangeState("Move");
                }
                else
                {
                    pOwner_->ChangeState("Idle");
                }
            }
        }
    }
    else if (isGrounded_ && velocity_.y <= kVelocityZero && nextY - groundLevel <= kGroundSnapDistance)
    {
        // 下り坂で毎フレーム接地が外れてガタつかないよう、僅かな段差は地面に吸着させる
        pOwner_->GetLocalPosition().y = groundLevel;
        velocity_.y = kVelocityZero;
    }
    else
    {
        pOwner_->GetLocalPosition().y = nextY;
        isGrounded_ = false;
    }
}

void PlayerMovement::UpdateDashState()
{
    if (!pOwner_->GetGamePad()->IsConnected())
    {
        return; // キーボードの場合はダッシュ継続時間を管理しない
    }

    if (isDashing_)
    {
        dashDuration_ += pOwner_->GetDt(); // ダッシュ継続時間を更新（A＋スティックで維持）
    }

    // dashStartedThisFrame_ は1フレームだけ true になるフラグ
    // wasDashing_ で前フレームのダッシュ状態を保持し、次フレームでリセットする
    if (dashStartedThisFrame_ && wasDashing_)
    {
        dashStartedThisFrame_ = false;
    }

    wasDashing_ = isDashing_;
}

Direction PlayerMovement::CalculateDirectionFromRotation()
{
    // クォータニオンからオイラー角（Yaw）を取得し、8方向に分類
    float yaw = pOwner_->GetLocalRotation().ToEulerAngles().y;
    float angle = NormalizeAngle(yaw);

    if (angle >= 7.0f * std::numbers::pi_v<float> / 4.0f || angle < std::numbers::pi_v<float> / 4.0f)
    {
        return Direction::Forward;
    }
    else if (angle >= std::numbers::pi_v<float> / 4.0f && angle < 2.0f * std::numbers::pi_v<float> / 4.0f)
    {
        return Direction::ForwardRight;
    }
    else if (angle >= 2.0f * std::numbers::pi_v<float> / 4.0f && angle < 3.0f * std::numbers::pi_v<float> / 4.0f)
    {
        return Direction::Right;
    }
    else if (angle >= 3.0f * std::numbers::pi_v<float> / 4.0f && angle < 4.0f * std::numbers::pi_v<float> / 4.0f)
    {
        return Direction::BackwardRight;
    }
    else if (angle >= 4.0f * std::numbers::pi_v<float> / 4.0f && angle < 5.0f * std::numbers::pi_v<float> / 4.0f)
    {
        return Direction::Behind;
    }
    else if (angle >= 5.0f * std::numbers::pi_v<float> / 4.0f && angle < 6.0f * std::numbers::pi_v<float> / 4.0f)
    {
        return Direction::BackwardLeft;
    }
    else if (angle >= 6.0f * std::numbers::pi_v<float> / 4.0f && angle < 7.0f * std::numbers::pi_v<float> / 4.0f)
    {
        return Direction::Left;
    }
    else if (angle >= 7.0f * std::numbers::pi_v<float> / 4.0f && angle < 8.0f * std::numbers::pi_v<float> / 4.0f)
    {
        return Direction::ForwardLeft;
    }
    return Direction::Forward;
}

const char *PlayerMovement::GetDirectionName(Direction dir)
{
    switch (dir)
    {
    case Direction::Forward:
        return "前";
    case Direction::ForwardRight:
        return "右前";
    case Direction::Right:
        return "右";
    case Direction::BackwardRight:
        return "右後ろ";
    case Direction::Behind:
        return "後ろ";
    case Direction::BackwardLeft:
        return "左後ろ";
    case Direction::Left:
        return "左";
    case Direction::ForwardLeft:
        return "左前";
    default:
        return "不明";
    }
}

float PlayerMovement::NormalizeAngle(float angle)
{
    const float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    while (angle < 0.0f)
        angle += kTwoPi;
    while (angle >= kTwoPi)
        angle -= kTwoPi;
    return angle;
}

float PlayerMovement::CalculateShortestRotation(float from, float to)
{
    float diff = to - from;
    const float kPi = std::numbers::pi_v<float>;

    while (diff > kPi)
        diff -= 2.0f * kPi;
    while (diff < -kPi)
        diff += 2.0f * kPi;

    return diff;
}

Vector3 PlayerMovement::GetMovementDirection() const
{
    Vector3 dir = velocity_;
    float len = GetVelocityMagnitude();

    if (len > 0.001f)
    {
        dir.x /= len;
        dir.y /= len;
        dir.z /= len;
    }
    else
    {
        dir = {0.0f, 0.0f, 0.0f};
    }

    return dir;
}

float PlayerMovement::GetVelocityMagnitude() const
{
    return std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
}

void PlayerMovement::Save(DataHandler *pData)
{
    pData->Save("fallSpeed", fallSpeed_);
    pData->Save("moveSpeed", moveSpeed_);
    pData->Save("jumpSpeed", jumpSpeed_);
    pData->Save("maxSpeed", maxSpeed_);
    pData->Save("accelRate", accelRate_);
    pData->Save("meleeLungeSpeed", meleeLungeSpeed_);
    pData->Save("meleeLungeMinDistance", meleeLungeMinDistance_);
}

void PlayerMovement::Load(DataHandler *pData)
{
    fallSpeed_ = pData->Load<float>("fallSpeed", -9.8f);
    moveSpeed_ = pData->Load<float>("moveSpeed", 0.0f);
    jumpSpeed_ = pData->Load<float>("jumpSpeed", 10.0f);
    maxSpeed_ = pData->Load<float>("maxSpeed", 10.0f);
    accelRate_ = pData->Load<float>("accelRate", 15.0f);
    meleeLungeSpeed_ = pData->Load<float>("meleeLungeSpeed", meleeLungeSpeed_);
    meleeLungeMinDistance_ = pData->Load<float>("meleeLungeMinDistance", meleeLungeMinDistance_);
}

void PlayerMovement::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Text("ダッシュ始めたフレームかどうか: %s", dashStartedThisFrame_ ? "True" : "False");
    ImGui::Text("ダッシュ時間: %f", dashDuration_);
    ImGui::Text("IsGrounded: %s", isGrounded_ ? "True" : "False");
    ImGui::Text("向いている方向: %s", GetDirectionName(dir_));
    ImGui::DragFloat("ジャンプ速度", &jumpSpeed_, 0.1f, 0.0f, 50.0f);
    ImGui::DragFloat("落下速度", &fallSpeed_, 0.1f, -20.0f, 0.0f);
    ImGui::DragFloat("現在速度", &moveSpeed_, 0.1f, 0.0f, maxSpeed_);
    ImGui::DragFloat("最大速度", &maxSpeed_, 0.1f, 0.0f, 50.0f);
    ImGui::DragFloat("加速率", &accelRate_, 0.1f, 0.0f, 50.0f);
    ImGui::DragFloat("近接踏み込み速度", &meleeLungeSpeed_, 0.5f, 0.0f, 60.0f);
    ImGui::DragFloat("近接踏み込み最小距離", &meleeLungeMinDistance_, 0.1f, 0.0f, 20.0f);
    ImGui::Text("現在速度: X=%.2f, Y=%.2f, Z=%.2f",
                velocity_.x, velocity_.y, velocity_.z);
#endif // USE_IMGUI
}

void PlayerMovement::RegisterParams()
{
    auto *pHub = GameParamHub::GetInstance();
    pHub->Register("Player", "最大速度", &maxSpeed_, {0.1f, 0.0f, 50.0f});
    pHub->Register("Player", "加速率", &accelRate_, {0.1f, 0.0f, 50.0f});
    pHub->Register("Player", "近接踏み込み速度", &meleeLungeSpeed_, {0.5f, 0.0f, 60.0f});
    pHub->Register("Player", "近接踏み込み最小距離", &meleeLungeMinDistance_, {0.1f, 0.0f, 20.0f});
    pHub->Register("Player", "ジャンプ速度", &jumpSpeed_, {0.1f, 0.0f, 50.0f});
    pHub->Register("Player", "落下速度", &fallSpeed_, {0.1f, -20.0f, 0.0f});
}

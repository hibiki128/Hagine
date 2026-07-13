#define NOMINMAX
#include "PlayerMovement.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "application/Camera/FollowCamera.h"
#include <Data/DataHandler.h>
#include <Utility/Debug/GameParam/GameParamHub.h>
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Hagine;

void PlayerMovement::Init(Player *owner)
{
    owner_ = owner;
    isGrounded_ = true; // 初期状態は地面にいる
}

void PlayerMovement::Move()
{
    float xInput = kInputZero;
    float zInput = kInputZero;

    GamePad *gamePad = owner_->GetGamePad();
    Input *input = owner_->GetInput();
    const float dt = owner_->GetDt();

    if (!gamePad->IsConnected())
    {
        // キーボード入力
        if (input->PushKey(DIK_A))
            xInput += kInputValue;
        if (input->PushKey(DIK_D))
            xInput -= kInputValue;
        if (input->PushKey(DIK_W))
            zInput += kInputValue;
        if (input->PushKey(DIK_S))
            zInput -= kInputValue;
        isDashing_ = input->PushKey(DIK_LCONTROL);
        if (input->TriggerKey(DIK_LCONTROL))
        {
            owner_->EmitAction(Player::ActionKind::Dash); // 入力表示UI用：ダッシュ開始を通知
        }
    }
    else
    {
        // ゲームパッド入力
        xInput = -gamePad->GetLeftStickX(); // 左スティックX軸
        zInput = gamePad->GetLeftStickY();  // 左スティックY軸

        // 左スティックが倒れているか（ダッシュ維持の判定に使う）
        bool hasStickInput = (xInput != kInputZero || zInput != kInputZero);

        // Aボタンのトリガーでダッシュ開始（LTは不要）。
        // すでにダッシュ中の場合は再始動しないため、ダッシュ中のA入力は
        // Rush 側のトリガーとして扱われる（PlayerStateFly* の TryChangeToRush）。
        if (gamePad->IsTrigger(XINPUT_GAMEPAD_A) && !isDashing_)
        {
            dashInputX_ = xInput;
            dashInputZ_ = zInput;
            isDashing_ = true;
            dashStartedThisFrame_ = true;
            dashDuration_ = 0.0f;
            // A押下時にスティックがニュートラルなら、猶予時間内に倒せばダッシュ継続を許可。
            // 既にスティックを倒していれば従来通り即ダッシュ確定（猶予不要）。
            dashGraceTimer_ = hasStickInput ? 0.0f : kDashGraceTime;
            owner_->EmitAction(Player::ActionKind::Dash); // 入力表示UI用：ダッシュ開始を通知
        }

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
    FollowCamera *camera = owner_->GetCamera();
    if (!camera)
        return;

    float yaw = camera->GetYaw();
    Vector3 cameraForward = {std::sin(yaw), kYComponentZero, std::cos(yaw)};
    Vector3 cameraRight = {-std::cos(yaw), kYComponentZero, std::sin(yaw)};

    Vector3 moveDir = cameraRight * xInput + cameraForward * zInput;

    // ダッシュ開始時のスティック入力がない場合、敵または自機正面方向へ移動
    if (dashStartedThisFrame_ && dashInputX_ == kInputZero && dashInputZ_ == kInputZero)
    {
        if (owner_->GetEnemy())
        {
            Vector3 toEnemy = owner_->GetEnemy()->GetWorldPosition() - owner_->GetWorldPosition();
            toEnemy.y = 0;
            if (toEnemy.Length() > 0.001f)
            {
                moveDir = toEnemy.Normalize();
            }
        }
        else
        {
            moveDir = owner_->GetForward();
            moveDir.y = 0;
            moveDir = moveDir.Normalize();
        }
    }
    else if (moveDir.Length() > 0.001f)
    {
        moveDir = moveDir.Normalize();
    }

    // ロックオン中でない場合、移動方向に向けて回転
    if (!owner_->GetIsLockOn() && moveDir.Length() > 0.001f)
    {
        float targetYaw = std::atan2(-moveDir.x, moveDir.z);
        Quaternion targetRot = Quaternion::FromEulerAngles({kRotationZero, targetYaw, kRotationZero});
        float rotateSpeed = kPlayerRotationSpeed;
        owner_->GetLocalRotation() = Quaternion::Slerp(owner_->GetLocalRotation(), targetRot, rotateSpeed * dt);
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

void PlayerMovement::DirectionUpdate()
{
    GamePad *gamePad = owner_->GetGamePad();
    Input *input = owner_->GetInput();

    if (!gamePad->IsConnected())
    {
        // キーボード入力
        if (input->PushKey(DIK_D))
        {
            moveDir_ = MoveDirection::Right;
        }
        else if (input->PushKey(DIK_A))
        {
            moveDir_ = MoveDirection::Left;
        }
        else if (input->PushKey(DIK_W))
        {
            moveDir_ = MoveDirection::Forward;
        }
        else if (input->PushKey(DIK_S))
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
        float xInput = gamePad->GetLeftStickX(); // 左スティックX軸
        float zInput = gamePad->GetLeftStickY(); // 左スティックY軸

        // スティック入力から方向を判定
        if (xInput != 0.0f || zInput != 0.0f)
        {
            float angle = std::atan2(xInput, zInput);

            const float PI = std::numbers::pi_v<float>;
            const float segment = PI / 4.0f; // 45度

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
    if (!owner_->GetIsLockOn())
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
    GamePad *gamePad = owner_->GetGamePad();
    Input *input = owner_->GetInput();
    const float dt = owner_->GetDt();

    if (owner_->GetIsLockOn() && owner_->GetEnemy())
    {
        Vector3 toEnemy = owner_->GetEnemy()->GetWorldPosition() - owner_->GetWorldPosition();
        if (toEnemy.Length() > kMinRotationDistance)
        {
            toEnemy = toEnemy.Normalize();

            // プレイヤーの正面方向(+Z方向)を敵の方向に向ける
            Vector3 forward = toEnemy;
            Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ};

            // forwardとworldUpが平行になる場合の対処
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

            // 回転行列から目標クォータニオンを作成
            Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
            Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

            float rotateSpeed = kPlayerRotationSpeed;
            owner_->GetLocalRotation() = Quaternion::Slerp(owner_->GetLocalRotation(), targetRot, rotateSpeed * dt);
        }
    }
    else
    {
        if (!gamePad->IsConnected())
        {
            // キーボード入力（左右矢印キーで手動回転）
            Vector3 euler = owner_->GetLocalRotation().ToEulerAngles();
            bool rotationChanged = false;
            if (input->PushKey(DIK_RIGHT))
            {
                euler.y -= kManualRotationSpeed;
                rotationChanged = true;
            }
            if (input->PushKey(DIK_LEFT))
            {
                euler.y += kManualRotationSpeed;
                rotationChanged = true;
            }
            if (rotationChanged)
            {
                owner_->GetLocalRotation() = Quaternion::FromEulerAngles(euler);
            }
        }
        else
        {
            // ゲームパッド入力（右スティックX軸で左右回転）
            float rightStickX = gamePad->GetRightStickX();
            float rightStickY = gamePad->GetRightStickY();

            if (rightStickX != 0.0f || rightStickY != 0.0f)
            {
                Vector3 euler = owner_->GetLocalRotation().ToEulerAngles();
                euler.y += rightStickX * kManualRotationSpeed * 2.0f;
                owner_->GetLocalRotation() = Quaternion::FromEulerAngles(euler);
            }
        }
    }
}

void PlayerMovement::CollisionGround()
{
    const float dt = owner_->GetDt();

    // 下方向の速度を制限
    if (velocity_.y < kMaxFallVelocity)
    {
        velocity_.y = kMaxFallVelocity;
    }

    // 位置更新前に次フレームのY座標を計算
    float nextY = owner_->GetLocalPosition().y + velocity_.y * dt;

    owner_->GetLocalPosition().x += velocity_.x * dt;
    owner_->GetLocalPosition().z += velocity_.z * dt;

    if (nextY <= kGroundLevel)
    {
        // Rush状態の場合は地面から押し戻す（地面に埋まらないよう浮かせる）
        if (owner_->GetCurrentStateName() == "Rush")
        {
            owner_->GetLocalPosition().y = kRushGroundOffset;
            velocity_.y = kVelocityZero;
            return;
        }

        // 地面に接地（Rush 以外）
        owner_->GetLocalPosition().y = kGroundLevel;
        if (!isGrounded_)
        {
            velocity_.y = kVelocityZero;
            isGrounded_ = true;
            // 空中からの着地で水平速度に応じて状態遷移
            if (owner_->GetCurrentStateName() == "Air")
            {
                float horizontalSpeed = sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
                if (horizontalSpeed > kLandingSpeedThreshold)
                {
                    owner_->ChangeState("Move");
                }
                else
                {
                    owner_->ChangeState("Idle");
                }
            }
        }
    }
    else
    {
        owner_->GetLocalPosition().y = nextY;
        isGrounded_ = false;
    }
}

void PlayerMovement::UpdateDashState()
{
    if (!owner_->GetGamePad()->IsConnected())
    {
        return; // キーボードの場合はダッシュ継続時間を管理しない
    }

    if (isDashing_)
    {
        dashDuration_ += owner_->GetDt(); // ダッシュ継続時間を更新（A＋スティックで維持）
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
    float yaw = owner_->GetLocalRotation().ToEulerAngles().y;
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
    const float TWO_PI = 2.0f * std::numbers::pi_v<float>;
    while (angle < 0.0f)
        angle += TWO_PI;
    while (angle >= TWO_PI)
        angle -= TWO_PI;
    return angle;
}

float PlayerMovement::CalculateShortestRotation(float from, float to)
{
    float diff = to - from;
    const float PI = std::numbers::pi_v<float>;

    while (diff > PI)
        diff -= 2.0f * PI;
    while (diff < -PI)
        diff += 2.0f * PI;

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

void PlayerMovement::Save(DataHandler *data)
{
    data->Save("fallSpeed", fallSpeed_);
    data->Save("moveSpeed", moveSpeed_);
    data->Save("jumpSpeed", jumpSpeed_);
    data->Save("maxSpeed", maxSpeed_);
    data->Save("accelRate", accelRate_);
}

void PlayerMovement::Load(DataHandler *data)
{
    fallSpeed_ = data->Load<float>("fallSpeed", -9.8f);
    moveSpeed_ = data->Load<float>("moveSpeed", 0.0f);
    jumpSpeed_ = data->Load<float>("jumpSpeed", 10.0f);
    maxSpeed_ = data->Load<float>("maxSpeed", 10.0f);
    accelRate_ = data->Load<float>("accelRate", 15.0f);
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
    ImGui::Text("現在速度: X=%.2f, Y=%.2f, Z=%.2f",
                velocity_.x, velocity_.y, velocity_.z);
#endif // USE_IMGUI
}

void PlayerMovement::RegisterParams()
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Player", "最大速度", &maxSpeed_, {0.1f, 0.0f, 50.0f});
    hub->Register("Player", "加速率", &accelRate_, {0.1f, 0.0f, 50.0f});
    hub->Register("Player", "ジャンプ速度", &jumpSpeed_, {0.1f, 0.0f, 50.0f});
    hub->Register("Player", "落下速度", &fallSpeed_, {0.1f, -20.0f, 0.0f});
}

#include "Player.h"
#include "Engine/Frame/Frame.h"
#include "State/Air/PlayerStateAir.h"
#include "State/Fly/PlayerStateFlyIdle.h"

#include "Bullet/ChageShot/ChageShot.h"
#include "State/Fly/PlayerStateFlyMove.h"
#include "State/Ground/PlayerStateIdle.h"
#include "State/Ground/PlayerStateJump.h"
#include "State/Ground/PlayerStateMove.h"
#include "application/Camera/FollowCamera.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "numbers"
#include <Input.h>
#include <cmath>
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include"Object/Base/BaseObjectManager.h"

Player::Player() {
}

Player::~Player() {
}

void Player::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cube);
    BaseObject::AddCollider();
    BaseObject::SetCollisionType(CollisionType::OBB);
    states_["Idle"] = std::make_unique<PlayerStateIdle>();
    states_["Move"] = std::make_unique<PlayerStateMove>();
    states_["Jump"] = std::make_unique<PlayerStateJump>();
    states_["Air"] = std::make_unique<PlayerStateAir>();
    states_["FlyIdle"] = std::make_unique<PlayerStateFlyIdle>();
    states_["FlyMove"] = std::make_unique<PlayerStateFlyMove>();
    currentState_ = states_["Idle"].get();
    isGrounded_ = true; // 初期状態は地面にいる

    data_ = std::make_unique<DataHandler>("EntityData", "Player");
    shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetLocalRotation() = Quaternion::FromEulerAngles(Vector3(degreesToRadians(-90.0f), 0.0f, 0.0f));
    shadow_->GetLocalScale() = {1.5f, 1.5f, 1.5f};

    chageShot_ = std::make_unique<ChageShot>();
    chageShot_->Init("chageShot");

    // 手の生成
    leftHand_ = std::make_unique<PlayerHand>();
    leftHand_->Init("leftHand");
    leftHand_->GetLocalScale() = {0.5f, 0.5f, 0.5f};
    leftHand_->GetLocalPosition() = {-2.0f, 0.0f, 0.0f};

    rightHand_ = std::make_unique<PlayerHand>();
    rightHand_->Init("rightHand");
    rightHand_->GetLocalScale() = {0.5f, 0.5f, 0.5f};
    rightHand_->GetLocalPosition() = {2.0f, 0.0f, 0.0f};

    this->AddChild(leftHand_.get());
    this->AddChild(rightHand_.get());

    MotionEditor::GetInstance()->Register(leftHand_.get());
    MotionEditor::GetInstance()->Register(rightHand_.get());

    BaseObjectManager::GetInstance()->AddObject(std::move(leftHand_));
    BaseObjectManager::GetInstance()->AddObject(std::move(rightHand_));

    Load();
}

void Player::Update() {

    dt_ = Frame::DeltaTime();
    shadow_->GetLocalPosition() = {transform_->translation_.x, -0.95f, transform_->translation_.z};
    shadow_->Update();
    if (chageShot_) {
        chageShot_->SetPlayer(this);
        chageShot_->Update();
    }

    if (currentState_) {
        currentState_->Update(*this);
    }

    // 下方向の速度を制限
    if (velocity_.y < -40.0f) {
        velocity_.y = -40.0f;
    }

    if (Input::GetInstance()->TriggerKey(DIK_L)) {
        isLockOn_ = !isLockOn_;
    }

    if (isDashing_) {
        targetFov_ = 55.0f;
    } else {
        targetFov_ = 45.0f;
    }

    // 現在のFOVを滑らかに補間（線形補間）
    currentFov_ += (targetFov_ - currentFov_) * fovLerpSpeed_ * dt_;

    // FOVをカメラに適用
    FollowCamera_->SetCameraFov(currentFov_);

    CollisionGround();

    RotateUpdate();

    BaseObject::Update();

    Shot();

    // 弾の更新と生存チェック
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        (*it)->Update();
        (*it)->SetSpeed(B_speed_);
        (*it)->SetAcce(B_acce_);

        // 弾が生きていない場合は削除
        if (!(*it)->IsAlive()) {
            it = bullets_.erase(it);
        } else {
            ++it;
        }
    }
}

void Player::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    shadow_->Draw(viewProjection, offSet);
    BaseObject::Draw(viewProjection, offSet);
    for (auto &bullet : bullets_) {
        bullet->Draw(viewProjection, offSet);
    }
    chageShot_->Draw(viewProjection, offSet);
}

void Player::DrawParticle(const ViewProjection &viewProjection) {
    chageShot_->DrawParticle(viewProjection);
    for (auto &bullet : bullets_) {
        bullet->DrawParticle(viewProjection);
    }
}

void Player::ChangeState(const std::string &stateName) {
    auto it = states_.find(stateName);
    if (it != states_.end()) {
        if (currentState_) {
            currentState_->Exit(*this);
        }
        currentState_ = it->second.get();
        currentState_->Enter(*this);
    }
}

void Player::DirectionUpdate() {
    if (Input::GetInstance()->PushKey(DIK_D)) {
        // 右
        moveDir_ = MoveDirection::Right;
    } else if (Input::GetInstance()->PushKey(DIK_A)) {
        // 左
        moveDir_ = MoveDirection::Left;
    } else if (Input::GetInstance()->PushKey(DIK_W)) {
        // 前
        moveDir_ = MoveDirection::Forward;
    } else if (Input::GetInstance()->PushKey(DIK_S)) {
        // 後ろ
        moveDir_ = MoveDirection::Behind;
    }
    // 向いてる方向は回転値から計算（ロックオン時以外）
    if (!isLockOn_) {
        dir_ = CalculateDirectionFromRotation();
    } else {
        dir_ = Direction::Forward;
    }
}

void Player::Debug() {
    // 現在のステート名を取得
    const char *currentStateName = "Unknown";
    for (const auto &[named, state] : states_) {
        if (state.get() == currentState_) {
            currentStateName = named.c_str();
            break;
        }
    }
    if (ImGui::BeginTabBar("プレイヤー")) {
        if (ImGui::BeginTabItem("プレイヤー")) {
            ImGui::Text("Current State: %s", currentStateName);
            ImGui::Text("IsGrounded: %s", isGrounded_ ? "True" : "False");
            ImGui::Text("IsLockOn: %s", isLockOn_ ? "True" : "False");
            ImGui::Text("向いている方向: %s", GetDirectionName(dir_));
            ImGui::DragFloat("ジャンプ速度", &jumpSpeed_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("落下速度", &fallSpeed_, 0.1f, -20.0f, 0.0f);
            ImGui::DragFloat("現在速度", &moveSpeed_, 0.1f, 0.0f, maxSpeed_);
            ImGui::DragFloat("最大速度", &maxSpeed_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("加速率", &accelRate_, 0.1f, 0.0f, 50.0f);
            ImGui::Text("現在位置: X=%.2f, Y=%.2f, Z=%.2f",
                        GetLocalPosition().x, GetLocalPosition().y, GetLocalPosition().z);
            ImGui::Text("現在速度: X=%.2f, Y=%.2f, Z=%.2f",
                        velocity_.x, velocity_.y, velocity_.z);

            ImGui::DragFloat("弾の速度", &B_speed_, 0.1f);
            ImGui::DragFloat("弾の加速度", &B_acce_, 0.1f);

            if (ImGui::Button("セーブ")) {

                Save();
            }

            if (ImGui::TreeNode("操作説明")) {
                ImGui::Text("WASD : 移動");
                if (currentState_ != states_["FlyMove"].get() || currentState_ != states_["FlyIdle"].get()) {
                    ImGui::Text("SPACE : ジャンプ");
                    ImGui::Text("空中でSPACE : 浮遊");
                } else {
                    ImGui::Text("SPACE : 上昇");
                    ImGui::Text("LSHIFT : 下降");
                    ImGui::Text("LSHIFT2回押し : 落下");
                    ImGui::Text("Ctrl : ダッシュ");
                }
                ImGui::Text("J : 射撃");
                ImGui::Text("K長押し : チャージショット");
                ImGui::Text("L : ロックオン");

                ImGui::TreePop();
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}


Vector3 Player::GetMovementDirection() const {
    Vector3 dir = velocity_;
    float len = GetVelocityMagnitude();

    // ゼロ除算を防ぐ
    if (len > 0.001f) {
        dir.x /= len;
        dir.y /= len;
        dir.z /= len;
    } else {
        dir = {0.0f, 0.0f, 0.0f};
    }

    return dir;
}

float Player::GetVelocityMagnitude() const {
    return std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
}

std::string Player::GetCurrentStateName() const {
    // 現在のステートを文字列で返す
    for (const auto &pair : states_) {
        if (pair.second.get() == currentState_) {
            return pair.first;
        }
    }
    return "Unknown"; // エラー時のフォールバック
}

void Player::Save() {
    data_->Save("fallSpeed", fallSpeed_);
    data_->Save("moveSpeed", moveSpeed_);
    data_->Save("jumpSpeed", jumpSpeed_);
    data_->Save("maxSpeed", maxSpeed_);
    data_->Save("accelRate", accelRate_);
    data_->Save("bulletSpeed", B_speed_);
    data_->Save("bulletAcce", B_acce_);
}

void Player::Load() {
    fallSpeed_ = data_->Load<float>("fallSpeed", -9.8f);
    moveSpeed_ = data_->Load<float>("moveSpeed", 0.0f);
    jumpSpeed_ = data_->Load<float>("jumpSpeed", 10.0f);
    maxSpeed_ = data_->Load<float>("maxSpeed", 10.0f);
    accelRate_ = data_->Load<float>("accelRate", 15.0f);
    B_speed_ = data_->Load<float>("bulletSpeed", 60.0f);
    B_acce_ = data_->Load<float>("bulletAcce", 5.0f);
}

void Player::Shot() {
    if (Input::GetInstance()->TriggerKey(DIK_J)) {
        // 弾の番号をbullets_のサイズで決定
        std::string bulletName = "PlayerBullet_" + std::to_string(bullets_.size());
        auto bullet = std::make_unique<PlayerBullet>();
        bullet->Init(bulletName);
        bullet->InitTransform(this);
        bullet->GetLocalScale() = {0.5f, 0.5f, 0.5f};
        bullet->SetRadius(0.5f);
        bullets_.push_back(std::move(bullet));
    }
}

void Player::RotateUpdate() {
    Vector3 euler = transform_->rotation_.ToEuler(); // ← クォータニオン → オイラー角に変換

    if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
        euler.y += 0.04f;
    }
    if (Input::GetInstance()->PushKey(DIK_LEFT)) {
        euler.y -= 0.04f;
    }

    transform_->rotation_ = Quaternion::FromEulerAngles(euler); // ← 再度クォータニオンに戻す
}

void Player::CollisionGround() {
    // 位置更新前に次の位置を計算
    float nextY = GetLocalPosition().y + velocity_.y * dt_;

    // 通常の位置更新
    GetLocalPosition().x += velocity_.x * dt_;
    GetLocalPosition().z += velocity_.z * dt_;

    // Y方向の処理（地面判定含む）
    if (nextY <= 0.0f) {
        // 地面に接地する場合
        GetLocalPosition().y = 0.0f;

        // 前のフレームで空中だった場合のみ着地処理
        if (!isGrounded_) {
            velocity_.y = 0.0f; // Y方向の速度をリセット
            isGrounded_ = true;

            // 空中からの着地で状態遷移
            if (currentState_ == states_["Air"].get()) {
                // 水平方向に動いていれば移動状態、そうでなければアイドル状態へ
                float horizontalSpeed = sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
                if (horizontalSpeed > 0.5f) {
                    ChangeState("Move");
                } else {
                    ChangeState("Idle");
                }
            }
        }
    } else {
        // 空中にいる場合
        GetLocalPosition().y = nextY;
        isGrounded_ = false;
    }
}

// 新しく追加するメソッド
Direction Player::CalculateDirectionFromRotation() {
    // プレイヤーの回転角度を [0, 2π) の範囲に正規化
    float angle = NormalizeAngle(transform_->rotation_.y);

    // 8方向の場合の角度範囲（π/4 = 45度ごと）
    // 0度を前方として、時計回りに8方向を判定
    if (angle >= 7.0f * std::numbers::pi_v<float> / 4.0f || angle < std::numbers::pi_v<float> / 4.0f) {

        return Direction::Forward;
    } else if (angle >= std::numbers::pi_v<float> / 4.0f && angle < 2.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::ForwardRight;
    } else if (angle >= 2.0f * std::numbers::pi_v<float> / 4.0f && angle < 3.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::Right;
    } else if (angle >= 3.0f * std::numbers::pi_v<float> / 4.0f && angle < 4.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::BackwardRight;
    } else if (angle >= 4.0f * std::numbers::pi_v<float> / 4.0f && angle < 5.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::Behind;
    } else if (angle >= 5.0f * std::numbers::pi_v<float> / 4.0f && angle < 6.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::BackwardLeft;
    } else if (angle >= 6.0f * std::numbers::pi_v<float> / 4.0f && angle < 7.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::Left;
    } else if (angle >= 7.0f * std::numbers::pi_v<float> / 4.0f && angle < 8.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::ForwardLeft;
    }

    // デフォルト（通常はここに来ないはず）
    return Direction::Forward;
}



// GetDirectionName メソッドを更新
const char *Player::GetDirectionName(Direction dir) {
    switch (dir) {
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
// 角度の正規化関数
float Player::NormalizeAngle(float angle) {
    // 角度を [0, 2π) の範囲に正規化
    const float TWO_PI = 2.0f * std::numbers::pi_v<float>;
    while (angle < 0.0f) {
        angle += TWO_PI;
    }
    while (angle >= TWO_PI) {
        angle -= TWO_PI;
    }
    return angle;
}

// 最短回転経路を計算する関数
float Player::CalculateShortestRotation(float from, float to) {
    float diff = to - from;
    const float PI = std::numbers::pi_v<float>;

    // 差分を [-π, π] の範囲に正規化
    while (diff > PI) {
        diff -= 2.0f * PI;
    }
    while (diff < -PI) {
        diff += 2.0f * PI;
    }

    return diff;
}

void Player::Move() {
    FollowCamera *camera = GetCamera();
    if (!camera) {
        DefaultMovement();
        return;
    }

    // 入力取得
    float xInput = 0.0f;
    float zInput = 0.0f;
    if (Input::GetInstance()->PushKey(DIK_D))
        xInput += 1.0f;
    if (Input::GetInstance()->PushKey(DIK_A))
        xInput -= 1.0f;
    if (Input::GetInstance()->PushKey(DIK_W))
        zInput += 1.0f;
    if (Input::GetInstance()->PushKey(DIK_S))
        zInput -= 1.0f;

    isDashing_ = Input::GetInstance()->PushKey(DIK_LCONTROL) || Input::GetInstance()->PushKey(DIK_RCONTROL);
    float maxSpeedMultiplier = isDashing_ ? 2.5f : 1.0f;

    if (xInput != 0.0f || zInput != 0.0f) {
        // 入力ベクトルを正規化
        Vector3 inputDir = {xInput, 0.0f, zInput};
        float length = std::sqrt(xInput * xInput + zInput * zInput);
        if (length > 0.0f) {
            inputDir.x /= length;
            inputDir.z /= length;
        }

        // カメラのYawで回転
        float cameraYaw = camera->GetYaw();
        Quaternion camRot = Quaternion::FromAxisAngle({0, 1, 0}, cameraYaw);
        Vector3 moveDir = camRot * inputDir;

        // 加速
        GetMoveSpeed() += GetAccelRate() * dt_;
        float maxSpeed = GetMaxSpeed() * (currentState_ == states_["FlyMove"].get() ? maxSpeedMultiplier : 1.0f);
        if (GetMoveSpeed() > maxSpeed)
            GetMoveSpeed() = maxSpeed;

        // 移動速度反映
        GetVelocity().x = moveDir.x * GetMoveSpeed();
        GetVelocity().z = moveDir.z * GetMoveSpeed();

      // 回転処理
        bool isLockOn = GetIsLockOn();
        Enemy *targetEnemy = GetEnemy();

        Quaternion currentRot = GetLocalRotation();
        Quaternion targetRot = currentRot;

        if (isLockOn && targetEnemy) {
            // ロックオン時は敵方向
            Vector3 playerPos = GetLocalPosition();
            Vector3 enemyPos = targetEnemy->GetLocalPosition();
            Vector3 toEnemy = enemyPos - playerPos;
            toEnemy.y = 0.0f;
            if (toEnemy.Length() > 0.001f) {
                targetRot = Quaternion::FromLookRotation(toEnemy.Normalize(), {0, 1, 0});
            }
        } else {
            // 移動方向を向く
            if (moveDir.Length() > 0.001f) {
                targetRot = Quaternion::FromLookRotation(moveDir.Normalize(), {0, 1, 0});
            }
        }

        // 補間率（回転速度）を調整
        float rotationLerp = (isLockOn ? 15.0f : 10.0f) * dt_;
        rotationLerp = std::clamp(rotationLerp, 0.0f, 1.0f);

        // Slerpで補間
        Quaternion newRot = Quaternion::Slerp(currentRot, targetRot, rotationLerp);
        GetLocalRotation() = newRot;
    } else {
        // 減速処理
        GetMoveSpeed() -= GetAccelRate() * 2.0f * dt_;
        if (GetMoveSpeed() < 0.0f)
            GetMoveSpeed() = 0.0f;
        GetVelocity().x *= 0.95f;
        GetVelocity().z *= 0.95f;

        // ロックオン中は敵の方向を向く
        bool isLockOn = GetIsLockOn();
        Enemy *targetEnemy = GetEnemy();
        if (isLockOn && targetEnemy) {
            Vector3 playerPos = GetLocalPosition();
            Vector3 enemyPos = targetEnemy->GetLocalPosition();
            Vector3 toEnemy = enemyPos - playerPos;
            toEnemy.y = 0.0f;
            if (toEnemy.Length() > 0.001f) {
                GetLocalRotation() = Quaternion::FromLookRotation(toEnemy.Normalize(), {0, 1, 0});
            }
        }
    }
}

void Player::DefaultMovement() {
    // 入力取得
    float xInput = 0.0f;
    float zInput = 0.0f;
    if (Input::GetInstance()->PushKey(DIK_D))
        xInput += 1.0f;
    if (Input::GetInstance()->PushKey(DIK_A))
        xInput -= 1.0f;
    if (Input::GetInstance()->PushKey(DIK_W))
        zInput += 1.0f;
    if (Input::GetInstance()->PushKey(DIK_S))
        zInput -= 1.0f;

    // ダッシュ入力
    isDashing_ = Input::GetInstance()->PushKey(DIK_LCONTROL) || Input::GetInstance()->PushKey(DIK_RCONTROL);
    float maxSpeedMultiplier = isDashing_ ? 2.5f : 1.0f;

    if (xInput != 0.0f || zInput != 0.0f) {
        // 入力ベクトルを正規化
        if (xInput != 0.0f && zInput != 0.0f) {
            float length = std::sqrt(xInput * xInput + zInput * zInput);
            xInput /= length;
            zInput /= length;
        }

        // 加速
        GetMoveSpeed() += GetAccelRate() * dt_;
        float maxSpeed = GetMaxSpeed() * (currentState_ == states_["FlyMove"].get() ? maxSpeedMultiplier : 1.0f);
        if (GetMoveSpeed() > maxSpeed) {
            GetMoveSpeed() = maxSpeed;
        }

        // 移動速度反映
        GetVelocity().x = xInput * GetMoveSpeed();
        GetVelocity().z = zInput * GetMoveSpeed();

        // 回転処理（ロックオン対応）
        bool isLockOn = GetIsLockOn();
        Enemy *targetEnemy = GetEnemy();

        Vector3 currentEuler = GetLocalRotation().ToEuler();
        float currentYaw = -currentEuler.y; // 座標系の修正
        float targetYaw = currentYaw;

        if (isLockOn && targetEnemy) {
            Vector3 playerPos = GetLocalPosition();
            Vector3 enemyPos = targetEnemy->GetLocalPosition();
            float dx = enemyPos.x - playerPos.x;
            float dz = enemyPos.z - playerPos.z;
            targetYaw = std::atan2(dx, dz);
        } else {
            // プレイヤーの入力方向に向ける
            targetYaw = std::atan2(xInput, zInput);
        }

        float rotationDiff = CalculateShortestRotation(currentYaw, targetYaw);
        float rotationSpeed = (isLockOn ? 15.0f : 10.0f) * dt_;
        float rotationAmount = std::clamp(rotationDiff, -rotationSpeed, rotationSpeed);

        // 回転適用（座標系を考慮して-yを使用）
        currentEuler.y = -(currentYaw + rotationAmount);
        GetLocalRotation() = Quaternion::FromEulerAngles(currentEuler);

    } else {
        // 減速処理
        GetMoveSpeed() -= GetAccelRate() * 2.0f * dt_;
        if (GetMoveSpeed() < 0.0f)
            GetMoveSpeed() = 0.0f;

        GetVelocity().x *= 0.95f;
        GetVelocity().z *= 0.95f;

        // ロックオン中は敵の方向を向く
        bool isLockOn = GetIsLockOn();
        Enemy *targetEnemy = GetEnemy();

        if (isLockOn && targetEnemy) {
            Vector3 playerPos = GetLocalPosition();
            Vector3 enemyPos = targetEnemy->GetLocalPosition();

            float dx = enemyPos.x - playerPos.x;
            float dz = enemyPos.z - playerPos.z;

            float targetYaw = std::atan2(dx, dz);
            Vector3 currentEuler = GetLocalRotation().ToEuler();
            float currentYaw = -currentEuler.y; // 座標系の修正

            float rotationDiff = CalculateShortestRotation(currentYaw, targetYaw);
            float rotationSpeed = 15.0f * dt_;
            float rotationAmount = std::clamp(rotationDiff, -rotationSpeed, rotationSpeed);

            currentEuler.y = -(currentYaw + rotationAmount);
            GetLocalRotation() = Quaternion::FromEulerAngles(currentEuler);
        }
    }
}
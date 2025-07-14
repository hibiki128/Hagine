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
    shadow_->GetLocalRotation().x = degreesToRadians(90.0f);
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

void Player::Move() {
    // カメラを取得
    FollowCamera *camera = GetCamera();
    if (!camera) {
        // カメラが取得できない場合は従来の方法で処理
        DefaultMovement();
        return;
    }

    // カメラのViewProjectionを取得
    ViewProjection &viewProjection = camera->GetViewProjection();

    // カメラのヨー角（水平回転角）を取得
    float cameraYaw = viewProjection.rotation_.y;

    // 移動入力の取得
    float xInput = 0.0f;
    float zInput = 0.0f;

    if (Input::GetInstance()->PushKey(DIK_D)) {
        xInput += 1.0f; // 右
    }
    if (Input::GetInstance()->PushKey(DIK_A)) {
        xInput -= 1.0f; // 左
    }
    if (Input::GetInstance()->PushKey(DIK_W)) {
        zInput += 1.0f; // 前
    }
    if (Input::GetInstance()->PushKey(DIK_S)) {
        zInput -= 1.0f; // 後ろ
    }

    // ダッシュ入力の確認
    isDashing_ = Input::GetInstance()->PushKey(DIK_LCONTROL) || Input::GetInstance()->PushKey(DIK_RCONTROL);
    float maxSpeedMultiplier = isDashing_ ? 2.5f : 1.0f;

    // 入力があれば徐々に加速、なければ減速
    if (xInput != 0.0f || zInput != 0.0f) {
        // 斜め移動の正規化（ベクトルの長さを1に）
        if (xInput != 0.0f && zInput != 0.0f) {
            float length = sqrt(xInput * xInput + zInput * zInput);
            xInput /= length;
            zInput /= length;
        }

        // 加速処理
        GetMoveSpeed() += GetAccelRate() * dt_;

        if (currentState_ == states_["FlyMove"].get()) {
            // 最大速度の制限（ダッシュ中は2倍）
            if (GetMoveSpeed() > GetMaxSpeed() * maxSpeedMultiplier) {
                GetMoveSpeed() = GetMaxSpeed() * maxSpeedMultiplier;
            }
        } else {
            // 最大速度の制限
            if (GetMoveSpeed() > GetMaxSpeed()) {
                GetMoveSpeed() = GetMaxSpeed();
            }
        }

        // カメラの向きを基準にした移動ベクトルを計算
        // カメラのヨー角に基づいて移動方向を回転させる
        float vx = (xInput * cos(cameraYaw) + zInput * sin(cameraYaw)) * GetMoveSpeed();
        float vz = (-xInput * sin(cameraYaw) + zInput * cos(cameraYaw)) * GetMoveSpeed();

        // 速度を設定
        GetVelocity().x = vx;
        GetVelocity().z = vz;

        // ロックオン状態かどうかチェック
        bool isLockOn = GetIsLockOn();
        Enemy *targetEnemy = GetEnemy();

        if (isLockOn && targetEnemy) {
            // ロックオン中はエネミーの方向を向く
            Vector3 playerPos = GetLocalPosition();
            Vector3 enemyPos = targetEnemy->GetLocalPosition();

            // エネミーへの方向ベクトルを計算
            float dx = enemyPos.x - playerPos.x;
            float dz = enemyPos.z - playerPos.z;

            // エネミーへの角度を計算
            float targetRotation = atan2(dx, dz);

            // 現在の回転と目標回転の差を計算
            float currentRotation = GetLocalRotation().y;
            float rotationDiff = targetRotation - currentRotation;

            // 回転差を-πからπの範囲に正規化
            while (rotationDiff > std::numbers::pi_v<float>)
                rotationDiff -= 2.0f * std::numbers::pi_v<float>;
            while (rotationDiff < -std::numbers::pi_v<float>)
                rotationDiff += 2.0f * std::numbers::pi_v<float>;

            // ロックオン時は素早く向きを合わせる（通常よりも速い回転速度）
            float rotationSpeed = 15.0f * dt_;
            float rotationAmount = std::min(std::abs(rotationDiff), rotationSpeed);
            if (rotationDiff < 0)
                rotationAmount = -rotationAmount;

            // 回転を適用
            GetLocalRotation().y += rotationAmount;

            // 回転を0から2πの範囲に正規化
            while (GetLocalRotation().y > 2.0f * std::numbers::pi_v<float>)
                GetLocalRotation().y -= 2.0f * std::numbers::pi_v<float>;
            while (GetLocalRotation().y < 0.0f)
                GetLocalRotation().y += 2.0f * std::numbers::pi_v<float>;
        } else if (vx != 0.0f || vz != 0.0f) {
            // ロックオンしていない場合は通常通り移動方向に向かせる
            float targetRotation = atan2(vx, vz);

            // 現在の回転と目標回転の差を計算
            float currentRotation = GetLocalRotation().y;
            float rotationDiff = targetRotation - currentRotation;

            // 回転差を-πからπの範囲に正規化
            while (rotationDiff > std::numbers::pi_v<float>)
                rotationDiff -= 2.0f * std::numbers::pi_v<float>;
            while (rotationDiff < -std::numbers::pi_v<float>)
                rotationDiff += 2.0f * std::numbers::pi_v<float>;

            // 回転の補間係数
            float rotationSpeed = 10.0f * dt_;
            float rotationAmount = std::min(std::abs(rotationDiff), rotationSpeed);
            if (rotationDiff < 0)
                rotationAmount = -rotationAmount;

            // 回転を適用
            GetLocalRotation().y += rotationAmount;

            // 回転を0から2πの範囲に正規化
            while (GetLocalRotation().y > 2.0f * std::numbers::pi_v<float>)
                GetLocalRotation().y -= 2.0f * std::numbers::pi_v<float>;
            while (GetLocalRotation().y < 0.0f)
                GetLocalRotation().y += 2.0f * std::numbers::pi_v<float>;
        }
    } else {
        // 入力がない場合は減速
        GetMoveSpeed() -= GetAccelRate() * 2.0f * dt_;
        if (GetMoveSpeed() < 0.0f) {
            GetMoveSpeed() = 0.0f;
        }

        // 減速中は前回の方向を維持
        GetVelocity().x *= 0.95f;
        GetVelocity().z *= 0.95f;

        // 入力がなくても、ロックオン中はエネミーの方向を向き続ける
        bool isLockOn = GetIsLockOn();
        Enemy *targetEnemy = GetEnemy();

        if (isLockOn && targetEnemy) {
            // ロックオン中はエネミーの方向を向く
            Vector3 playerPos = GetLocalPosition();
            Vector3 enemyPos = targetEnemy->GetLocalPosition();

            // エネミーへの方向ベクトルを計算
            float dx = enemyPos.x - playerPos.x;
            float dz = enemyPos.z - playerPos.z;

            // エネミーへの角度を計算
            float targetRotation = atan2(dx, dz);

            // 現在の回転と目標回転の差を計算
            float currentRotation = GetLocalRotation().y;
            float rotationDiff = targetRotation - currentRotation;

            // 回転差を-πからπの範囲に正規化
            while (rotationDiff > std::numbers::pi_v<float>)
                rotationDiff -= 2.0f * std::numbers::pi_v<float>;
            while (rotationDiff < -std::numbers::pi_v<float>)
                rotationDiff += 2.0f * std::numbers::pi_v<float>;

            // ロックオン時は素早く向きを合わせる
            float rotationSpeed = 15.0f * dt_;
            float rotationAmount = std::min(std::abs(rotationDiff), rotationSpeed);
            if (rotationDiff < 0)
                rotationAmount = -rotationAmount;

            // 回転を適用
            GetLocalRotation().y += rotationAmount;

            // 回転を0から2πの範囲に正規化
            while (GetLocalRotation().y > 2.0f * std::numbers::pi_v<float>)
                GetLocalRotation().y -= 2.0f * std::numbers::pi_v<float>;
            while (GetLocalRotation().y < 0.0f)
                GetLocalRotation().y += 2.0f * std::numbers::pi_v<float>;
        }
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
    if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
        // 右回転
        transform_->rotation_.y += 0.04f;
    }
    if (Input::GetInstance()->PushKey(DIK_LEFT)) {
        // 左回転
        transform_->rotation_.y -= 0.04f;
    }
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

void Player::DefaultMovement() {
    // 移動入力の取得
    float xInput = 0.0f;
    float zInput = 0.0f;

    if (Input::GetInstance()->PushKey(DIK_D)) {
        xInput += 1.0f;
    }
    if (Input::GetInstance()->PushKey(DIK_A)) {
        xInput -= 1.0f;
    }
    if (Input::GetInstance()->PushKey(DIK_W)) {
        zInput += 1.0f;
    }
    if (Input::GetInstance()->PushKey(DIK_S)) {
        zInput -= 1.0f;
    }

    // ダッシュ入力の確認
    isDashing_ = Input::GetInstance()->PushKey(DIK_LCONTROL) || Input::GetInstance()->PushKey(DIK_RCONTROL);
    float maxSpeedMultiplier = isDashing_ ? 2.5f : 1.0f;

    // 入力があれば徐々に加速、なければ減速
    if (xInput != 0.0f || zInput != 0.0f) {
        // 斜め移動の正規化（ベクトルの長さを1に）
        if (xInput != 0.0f && zInput != 0.0f) {
            float length = sqrt(xInput * xInput + zInput * zInput);
            xInput /= length;
            zInput /= length;
        }

        // 加速処理
        GetMoveSpeed() += GetAccelRate() * dt_;

        if (currentState_ == states_["FlyMove"].get()) {
            // 最大速度の制限（ダッシュ中は2倍）
            if (GetMoveSpeed() > GetMaxSpeed() * maxSpeedMultiplier) {
                GetMoveSpeed() = GetMaxSpeed() * maxSpeedMultiplier;
            }
        } else {
            // 最大速度の制限
            if (GetMoveSpeed() > GetMaxSpeed()) {
                GetMoveSpeed() = GetMaxSpeed();
            }
        }

        // 正規化したベクトルに速度を掛ける
        GetVelocity().x = xInput * GetMoveSpeed();
        GetVelocity().z = zInput * GetMoveSpeed();

        // ロックオン状態かどうかチェック
        bool isLockOn = GetIsLockOn();
        Enemy *targetEnemy = GetEnemy();

        // ロックオン中はエネミーの方向を向く（DefaultMovementでも同様に実装）
        if (isLockOn && targetEnemy) {
            Vector3 playerPos = GetLocalPosition();
            Vector3 enemyPos = targetEnemy->GetLocalPosition();

            float dx = enemyPos.x - playerPos.x;
            float dz = enemyPos.z - playerPos.z;

            float targetRotation = atan2(dx, dz);

            // 現在の回転と目標回転の差を計算
            float currentRotation = GetLocalRotation().y;
            float rotationDiff = targetRotation - currentRotation;

            // 回転差を-πからπの範囲に正規化
            while (rotationDiff > std::numbers::pi_v<float>)
                rotationDiff -= 2.0f * std::numbers::pi_v<float>;
            while (rotationDiff < -std::numbers::pi_v<float>)
                rotationDiff += 2.0f * std::numbers::pi_v<float>;

            // 回転速度
            float rotationSpeed = 15.0f * dt_;
            float rotationAmount = std::min(std::abs(rotationDiff), rotationSpeed);
            if (rotationDiff < 0)
                rotationAmount = -rotationAmount;

            // 回転を適用
            GetLocalRotation().y += rotationAmount;

            // 回転を0から2πの範囲に正規化
            while (GetLocalRotation().y > 2.0f * std::numbers::pi_v<float>)
                GetLocalRotation().y -= 2.0f * std::numbers::pi_v<float>;
            while (GetLocalRotation().y < 0.0f)
                GetLocalRotation().y += 2.0f * std::numbers::pi_v<float>;
        }
    } else {
        // 入力がない場合は減速
        GetMoveSpeed() -= GetAccelRate() * 2.0f * dt_;
        if (GetMoveSpeed() < 0.0f) {
            GetMoveSpeed() = 0.0f;
        }

        // 減速中は前回の方向を維持
        GetVelocity().x *= 0.95f;
        GetVelocity().z *= 0.95f;

        // 入力がなくても、ロックオン中はエネミーの方向を向き続ける
        bool isLockOn = GetIsLockOn();
        Enemy *targetEnemy = GetEnemy();

        if (isLockOn && targetEnemy) {
            Vector3 playerPos = GetLocalPosition();
            Vector3 enemyPos = targetEnemy->GetLocalPosition();

            float dx = enemyPos.x - playerPos.x;
            float dz = enemyPos.z - playerPos.z;

            float targetRotation = atan2(dx, dz);

            // 現在の回転と目標回転の差を計算
            float currentRotation = GetLocalRotation().y;
            float rotationDiff = targetRotation - currentRotation;

            // 回転差を-πからπの範囲に正規化
            while (rotationDiff > std::numbers::pi_v<float>)
                rotationDiff -= 2.0f * std::numbers::pi_v<float>;
            while (rotationDiff < -std::numbers::pi_v<float>)
                rotationDiff += 2.0f * std::numbers::pi_v<float>;

            // 回転速度
            float rotationSpeed = 15.0f * dt_;
            float rotationAmount = std::min(std::abs(rotationDiff), rotationSpeed);
            if (rotationDiff < 0)
                rotationAmount = -rotationAmount;

            // 回転を適用
            GetLocalRotation().y += rotationAmount;

            // 回転を0から2πの範囲に正規化
            while (GetLocalRotation().y > 2.0f * std::numbers::pi_v<float>)
                GetLocalRotation().y -= 2.0f * std::numbers::pi_v<float>;
            while (GetLocalRotation().y < 0.0f)
                GetLocalRotation().y += 2.0f * std::numbers::pi_v<float>;
        }
    }
}

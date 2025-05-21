#include "Player.h"
#include "Engine/Frame/Frame.h"
#include "State/PlayerStateAir.h"
#include "State/PlayerStateFly.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateJump.h"
#include "State/PlayerStateMove.h"
#include "application/Camera/FollowCamera.h"
#include <Input.h>
#include <cmath>
#include"numbers"

Player::Player() {
}

Player::~Player() {
}

void Player::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cube);
    states_["Idle"] = std::make_unique<PlayerStateIdle>();
    states_["Move"] = std::make_unique<PlayerStateMove>();
    states_["Jump"] = std::make_unique<PlayerStateJump>();
    states_["Air"] = std::make_unique<PlayerStateAir>();
    states_["Fly"] = std::make_unique<PlayerStateFly>();
    currentState_ = states_["Idle"].get();
    isGrounded_ = true; // 初期状態は地面にいる

    data_ = std::make_unique<DataHandler>("EntityData", "Player");
    shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetWorldRotation().x = degreesToRadians(90.0f);
    shadow_->GetWorldScale() = {1.5f, 1.5f, 1.5f};

    Load();
}

void Player::Update() {

    shadow_->GetWorldPosition() = {transform_.translation_.x, -0.95f, transform_.translation_.z};
    shadow_->Update();

    if (currentState_) {
        currentState_->Update(*this);
    }

    // 下方向の速度を制限
    if (velocity_.y < -40.0f) {
        velocity_.y = -40.0f;
    }

    CollisionGround();

    RotateUpdate();

    BaseObject::Update();
}

void Player::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    shadow_->Draw(viewProjection, offSet);
    BaseObject::Draw(viewProjection, offSet);
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
            ImGui::Text("向いている方向: %s", GetDirectionName(dir_));
            ImGui::DragFloat("ジャンプ速度", &jumpSpeed_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("落下速度", &fallSpeed_, 0.1f, -20.0f, 0.0f);
            ImGui::DragFloat("現在速度", &moveSpeed_, 0.1f, 0.0f, maxSpeed_);
            ImGui::DragFloat("最大速度", &maxSpeed_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("加速率", &accelRate_, 0.1f, 0.0f, 50.0f);
            ImGui::Text("現在位置: X=%.2f, Y=%.2f, Z=%.2f",
                        GetWorldPosition().x, GetWorldPosition().y, GetWorldPosition().z);
            ImGui::Text("現在速度: X=%.2f, Y=%.2f, Z=%.2f",
                        velocity_.x, velocity_.y, velocity_.z);

            if (ImGui::Button("セーブ")) {

                Save();
            }

            if (ImGui::TreeNode("操作説明")) {
                ImGui::Text("WASD : 移動");
                if (currentState_ != states_["Fly"].get()) {
                    ImGui::Text("SPACE : ジャンプ\n");
                    ImGui::Text("空中でSPACE : 浮遊\n");
                } else {
                    ImGui::Text("SPACE : 上昇\n");
                    ImGui::Text("LSHIFT : 下降\n");
                    ImGui::Text("LSHIFT2回押し : 落下\n");
                    ImGui::Text("Ctrl : ダッシュ\n");
                }

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

void Player::Save() {
    data_->Save("fallSpeed", fallSpeed_);
    data_->Save("moveSpeed", moveSpeed_);
    data_->Save("jumpSpeed", jumpSpeed_);
    data_->Save("maxSpeed", maxSpeed_);
    data_->Save("accelRate", accelRate_);
}

void Player::Load() {
    fallSpeed_ = data_->Load<float>("fallSpeed", -9.8f);
    moveSpeed_ = data_->Load<float>("moveSpeed", 0.0f);
    jumpSpeed_ = data_->Load<float>("jumpSpeed", 10.0f);
    maxSpeed_ = data_->Load<float>("maxSpeed", 10.0f);
    accelRate_ = data_->Load<float>("accelRate", 15.0f);
}

void Player::RotateUpdate() {
    if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
        // 右回転
        transform_.rotation_.y += 0.04f;
    }
    if (Input::GetInstance()->PushKey(DIK_LEFT)) {
        // 左回転
        transform_.rotation_.y -= 0.04f;
    }
}

void Player::CollisionGround() {
    // 位置更新前に次の位置を計算
    float nextY = GetWorldPosition().y + velocity_.y * Frame::DeltaTime();

    // 通常の位置更新
    GetWorldPosition().x += velocity_.x * Frame::DeltaTime();
    GetWorldPosition().z += velocity_.z * Frame::DeltaTime();

    // Y方向の処理（地面判定含む）
    if (nextY <= 0.0f) {
        // 地面に接地する場合
        GetWorldPosition().y = 0.0f;

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
        GetWorldPosition().y = nextY;
        isGrounded_ = false;
    }
}

// 新しく追加するメソッド
Direction Player::CalculateDirectionFromRotation() {
    // プレイヤーの回転角度を [0, 2π) の範囲に正規化
    float angle = NormalizeAngle(transform_.rotation_.y);

    // 8方向の場合の角度範囲（π/4 = 45度ごと）
    // 0度を前方として、時計回りに8方向を判定
    if (angle >= 7.0f *  std::numbers::pi_v<float> / 4.0f || angle <  std::numbers::pi_v<float> / 4.0f) {
       
        return Direction::Forward;
    } else if (angle >=  std::numbers::pi_v<float> / 4.0f && angle < 2.0f *  std::numbers::pi_v<float> / 4.0f) {
        return Direction::ForwardRight;
    } else if (angle >= 2.0f *  std::numbers::pi_v<float> / 4.0f && angle < 3.0f *  std::numbers::pi_v<float> / 4.0f) {
        return Direction::Right;
    } else if (angle >= 3.0f *  std::numbers::pi_v<float> / 4.0f && angle < 4.0f *  std::numbers::pi_v<float> / 4.0f) {
        return Direction::BackwardRight;
    } else if (angle >= 4.0f *  std::numbers::pi_v<float> / 4.0f && angle < 5.0f *  std::numbers::pi_v<float> / 4.0f) {
        return Direction::Behind;
    } else if (angle >= 5.0f *  std::numbers::pi_v<float> / 4.0f && angle < 6.0f *  std::numbers::pi_v<float> / 4.0f) {
        return Direction::BackwardLeft;
    } else if (angle >= 6.0f *  std::numbers::pi_v<float> / 4.0f && angle < 7.0f *  std::numbers::pi_v<float> / 4.0f) {
        return Direction::Left;
    } else if (angle >= 7.0f *  std::numbers::pi_v<float> / 4.0f && angle < 8.0f *  std::numbers::pi_v<float> / 4.0f) {
        return Direction::ForwardLeft;
    }

    // デフォルト（通常はここに来ないはず）
    return Direction::Forward;
}

// 角度の正規化関数
float Player::NormalizeAngle(float angle) {
    // 角度を [0, 2π) の範囲に正規化
    const float TWO_PI = 2.0f *  std::numbers::pi_v<float>;
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
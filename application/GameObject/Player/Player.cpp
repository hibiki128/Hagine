#include "Player.h"
#include "Engine/Frame/Frame.h"
#include "State/PlayerStateAir.h"
#include "State/PlayerStateIdle.h"
#include "State/PlayerStateJump.h"
#include "State/PlayerStateMove.h"
#include"State/PlayerStateFly.h"
#include <Input.h>
#include <cmath> // ベクトル計算に必要
#include "application/Camera/FollowCamera.h"

Player::Player() {
}

Player::~Player() {
}

void Player::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreateModel("debug/cube.obj");
    BaseObject::CreateCollider();
    states_["Idle"] = std::make_unique<PlayerStateIdle>();
    states_["Move"] = std::make_unique<PlayerStateMove>();
    states_["Jump"] = std::make_unique<PlayerStateJump>();
    states_["Air"] = std::make_unique<PlayerStateAir>();
    states_["Fly"] = std::make_unique<PlayerStateFly>();
    currentState_ = states_["Idle"].get();
    isGrounded_ = true; // 初期状態は地面にいる

    data_ = std::make_unique<DataHandler>("EntityData", "Player");
    Load();

}

void Player::Update() {
    if (currentState_) {
        currentState_->Update(*this);
    }

    // 下方向の速度を制限
    if (velocity_.y < -40.0f) {
        velocity_.y = -40.0f;
    }

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

    BaseObject::Update();
}

void Player::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
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
    if (!isLockOn_) {
        if (Input::GetInstance()->PushKey(DIK_D)) {
            // 右
            dir_ = Direction::Right;
        } else if (Input::GetInstance()->PushKey(DIK_A)) {
            // 左
            dir_ = Direction::Left;
        } else if (Input::GetInstance()->PushKey(DIK_W)) {
            // 前
            dir_ = Direction::Forward;
        } else if (Input::GetInstance()->PushKey(DIK_S)) {
            // 後ろ
            dir_ = Direction::Behind;
        }
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

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    BaseObject::DebugImGui();
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

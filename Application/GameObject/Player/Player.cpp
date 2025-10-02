#define NOMINMAX
#include "Player.h"
#include "Engine/Frame/Frame.h"
#include "State/Air/PlayerStateAir.h"
#include "State/Fly/PlayerStateFlyIdle.h"

#include "Bullet/ChageShot/ChageShot.h"
#include "Object/Base/BaseObjectManager.h"
#include "State/Action/PlayerStateRush.h"
#include "State/Fly/PlayerStateFlyMove.h"
#include "State/Ground/PlayerStateIdle.h"
#include "State/Ground/PlayerStateJump.h"
#include "State/Ground/PlayerStateMove.h"
#include "application/Camera/FollowCamera.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "numbers"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Input.h>
#include <cmath>
#include <Particle/ParticleEditor.h>

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
    states_["Rush"] = std::make_unique<PlayerStateRush>();
    currentState_ = states_["Idle"].get();
    isGrounded_ = true; // 初期状態は地面にいる

    data_ = std::make_unique<DataHandler>("EntityData", "Player");
    shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetWorldTransform()->SetRotationEuler(Vector3(degreesToRadians(-90.0f), 0.0f, 0.0f));
    shadow_->GetLocalScale() = {1.5f, 1.5f, 1.5f};

    chageShot_ = std::make_unique<ChageShot>();
    chageShot_->SetPlayer(this);
    chageShot_->Init("chageShot");

    // 手の生成
    leftHand_ = std::make_unique<PlayerHand>();
    leftHand_->Init("leftHand");

    rightHand_ = std::make_unique<PlayerHand>();
    rightHand_->Init("rightHand");

    this->AddChild(leftHand_.get());
    this->AddChild(rightHand_.get());

    MotionEditor::GetInstance()->Register(leftHand_.get());
    MotionEditor::GetInstance()->Register(rightHand_.get());

    rightHand_ptr_ = rightHand_.get();
    leftHand_ptr_ = leftHand_.get();

    BaseObjectManager::GetInstance()->AddObject(std::move(leftHand_));
    BaseObjectManager::GetInstance()->AddObject(std::move(rightHand_));

    Load();

    if (!comboInitialized_) {
        punchCombo_.Add(GetRightHand(), "Jab") // 1段目：右手ジャブ
            .Add(GetLeftHand(), "Hook")        // 2段目：左手フック
            .Add(GetRightHand(), "Cross")      // 3段目：右手クロス
            .Add(GetLeftHand(), "Uppercut")    // 4段目：左手アッパーカット
            .Add(GetRightHand(), "Overhand")   // 5段目：右手オーバーハンド
            .Add(GetLeftHand(), "Swing")       // 6段目：左手スイング
            .Add(GetRightHand(), "Elbow")      // 7段目：右手肘打ち
            .Add(GetLeftHand(), "Slam");       // 8段目：左手スラム

        comboInitialized_ = true;
    }

    shake_ = std::make_unique<Shake>();
    rushEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("RushEmitter");
}

void Player::Update() {

    ComboUpdate();

    dt_ = Frame::DeltaTime();
    shadow_->GetLocalPosition() = {transform_->translation_.x, -0.95f, transform_->translation_.z};
    shadow_->Update();
    if (chageShot_) {
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

    // 現在のFOVを滑らかに補間
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
        (*it)->UpdateWorldTransformHierarchy();

        // 弾が生きていない場合は削除
        if (!(*it)->IsAlive()) {
            it = bullets_.erase(it);
        } else {
            ++it;
        }
    }
    
    shake_->Update();

    UpdateShadowScale();
}

void Player::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    BaseObject::Draw(viewProjection, offSet);
    for (auto &bullet : bullets_) {
        bullet->Draw(viewProjection, offSet);
    }
    chageShot_->Draw(viewProjection, offSet);
    if (transform_->translation_.y < 0) {
        return;
    }
    shadow_->Draw(viewProjection, offSet);
}

void Player::DrawParticle(const ViewProjection &viewProjection) {
    chageShot_->DrawParticle(viewProjection);
    rushEmitter_->Draw(viewProjection);
    for (auto &bullet : bullets_) {
        bullet->DrawParticle(viewProjection);
    }
    leftHand_ptr_->DrawParticle(viewProjection);
    rightHand_ptr_->DrawParticle(viewProjection);
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

void Player::Shot() {
    if (Input::GetInstance()->TriggerKey(DIK_J)) {
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
    if (isLockOn_ && enemy_) {
        Vector3 toEnemy = enemy_->GetWorldPosition() - GetWorldPosition();
        if (toEnemy.Length() > 0.001f) {
            toEnemy = toEnemy.Normalize();

            // プレイヤーの正面方向（+Z方向）を敵の方向に向ける
            Vector3 forward = toEnemy;
            Vector3 worldUp = {0.0f, 1.0f, 0.0f}; // 上方向

            // forwardとworldUpが平行になる場合の対処
            Vector3 right;
            if (std::abs(forward.Dot(worldUp)) > 0.999f) {
                right = {1.0f, 0.0f, 0.0f}; // X軸を右方向として使用
            } else {
                right = (worldUp.Cross(forward)).Normalize();
            }

            Vector3 up = (forward.Cross(right)).Normalize();

            // 回転行列から目標クォータニオンを作成
            Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
            Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

            float rotateSpeed = 10.0f;
            transform_->quateRotation_ = Quaternion::Slerp(transform_->quateRotation_, targetRot, rotateSpeed * dt_);
        }
    } else {
        Vector3 euler = transform_->quateRotation_.ToEulerAngles();
        bool rotationChanged = false;
        if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
            euler.y -= 0.04f;
            rotationChanged = true;
        }
        if (Input::GetInstance()->PushKey(DIK_LEFT)) {
            euler.y += 0.04f;
            rotationChanged = true;
        }
        if (rotationChanged) {
            transform_->quateRotation_ = Quaternion::FromEulerAngles(euler);
        }
    }
}

void Player::ComboUpdate() {
    // 毎フレーム更新
    punchCombo_.Update(Frame::DeltaTime());

    // 入力処理
    if (Input::GetInstance()->TriggerKey(DIK_H)) {
        punchCombo_.TryExecuteCombo();
    }
    if (punchCombo_.IsComboActive()) {
        GetRightHand()->SetCollisionEnabled(punchCombo_.IsObjectAttackCompleted(GetRightHand()));
        GetLeftHand()->SetCollisionEnabled(punchCombo_.IsObjectAttackCompleted(GetLeftHand()));
    }
}

void Player::Move() {
    // 入力取得（WASD）
    float xInput = 0.0f;
    float zInput = 0.0f;

    if (Input::GetInstance()->PushKey(DIK_A))
        xInput += 1.0f;
    if (Input::GetInstance()->PushKey(DIK_D))
        xInput -= 1.0f;
    if (Input::GetInstance()->PushKey(DIK_W))
        zInput += 1.0f;
    if (Input::GetInstance()->PushKey(DIK_S))
        zInput -= 1.0f;

    // 減速処理（入力がない場合）
    if (xInput == 0.0f && zInput == 0.0f) {
        velocity_.x *= 0.65f;
        velocity_.z *= 0.65f;
        if (std::abs(velocity_.x) < 0.01f)
            velocity_.x = 0.0f;
        if (std::abs(velocity_.z) < 0.01f)
            velocity_.z = 0.0f;
        isDashing_ = false;
        return;
    }

    // カメラの方向ベクトル取得
    FollowCamera *camera = GetCamera();
    if (!camera)
        return;

    float yaw = camera->GetYaw();

    Vector3 cameraForward = {std::sin(yaw), 0.0f, std::cos(yaw)};
    Vector3 cameraRight = {-std::cos(yaw), 0.0f, std::sin(yaw)};

    // 入力方向をカメラベースで合成
    Vector3 moveDir = cameraRight * xInput + cameraForward * zInput;

    // 正規化（斜め方向も一定速度にする）
    moveDir = moveDir.Normalize();

    // --- 回転処理 ---
    if (!isLockOn_) {
        float targetYaw = std::atan2(-moveDir.x, moveDir.z);
        Quaternion targetRot = Quaternion::FromEulerAngles({0.0f, targetYaw, 0.0f});
        float rotateSpeed = 10.0f;
        transform_->quateRotation_ = Quaternion::Slerp(transform_->quateRotation_, targetRot, rotateSpeed * dt_);
    }

    // --- 移動処理 ---
    float currentMaxSpeed = maxSpeed_;
    isDashing_ = Input::GetInstance()->PushKey(DIK_LCONTROL);
    if (isDashing_) {
        currentMaxSpeed *= 1.5f;
    }

    velocity_.x += moveDir.x * accelRate_ * dt_;
    velocity_.z += moveDir.z * accelRate_ * dt_;

    float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
    if (speed > currentMaxSpeed) {
        float scale = currentMaxSpeed / speed;
        velocity_.x *= scale;
        velocity_.z *= scale;
    }
}

void Player::UpdateShadowScale() {
    if (transform_->translation_.y < 0) {
        return;
    }
    float height = transform_->translation_.y;
    float baseScale = 1.5f;
    float scaleFactor = std::max(0.3f, baseScale - height * 0.1f);
    shadow_->GetLocalScale() = {scaleFactor, scaleFactor, scaleFactor};
}

void Player::CollisionGround() {
    // 位置更新前に次の位置を計算
    float nextY = GetLocalPosition().y + velocity_.y * dt_;
    // 通常の位置更新
    GetLocalPosition().x += velocity_.x * dt_;
    GetLocalPosition().z += velocity_.z * dt_;

    // Y方向の処理（地面判定含む）
    if (nextY <= 0.0f) {
        // Rush状態の場合は地面から押し戻す
        if (currentState_ == states_["Rush"].get()) {
            GetLocalPosition().y = 0.1f; // 地面から少し浮いた位置に強制移動
            velocity_.y = 0.0f;          // Y方向の速度をリセット
            return;                      // 状態遷移は行わない
        }

        // 地面に接地する場合（Rush以外の状態）
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

Direction Player::CalculateDirectionFromRotation() {
    // クォータニオンからオイラー角（Yaw）を取得
    float yaw = transform_->quateRotation_.ToEulerAngles().y;
    float angle = NormalizeAngle(yaw);

    // 8方向の場合の角度範囲（π/4 = 45度ごと）
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

    while (diff > PI) {
        diff -= 2.0f * PI;
    }
    while (diff < -PI) {
        diff += 2.0f * PI;
    }

    return diff;
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
            ImGui::Text("lControlInputCount: %d", lControlInputCount_);
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

    shake_->imgui();
}

void Player::ChangeRush() {
    if (Input::GetInstance()->TriggerKey(DIK_LCONTROL)) {
        lControlInputCount_++;
        if (lControlInputCount_ == 1) {
            lControlInputTime_ = 0.0f;
        } else if (lControlInputCount_ == 2 && GetIsLockOn() && GetEnemy()) {
            // 急接近ステートに遷移
            ChangeState("Rush");
            rushEmitter_->SetStartRotate("Burst", GetWorldRotation().ToEulerDegrees());
            rushEmitter_->SetEndRotate("Burst", GetWorldRotation().ToEulerDegrees());
            rushEmitter_->SetPosition(GetWorldPosition());
            rushEmitter_->UpdateOnce();
            return;
        }
    }

    // 入力リセット処理
    if (lControlInputCount_ > 0) {
        lControlInputTime_ += GetDt();
        if (lControlInputTime_ >= INPUT_RESET_TIME) {
            lControlInputCount_ = 0;
            lControlInputTime_ = 0.0f;
        }
    }
}

Vector3 Player::GetMovementDirection() const {
    Vector3 dir = velocity_;
    float len = GetVelocityMagnitude();

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

Vector3 Player::GetForward() const {
    // クォータニオンから前方向ベクトルを計算（Z軸の負方向が前方向）
    return TransformNormal(Vector3(0.0f, 0.0f, -1.0f), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Player::GetBackward() const {
    return -GetForward();
}

Vector3 Player::GetRight() const {
    // クォータニオンから右方向ベクトルを計算（X軸の正方向が右方向）
    return TransformNormal(Vector3(1.0f, 0.0f, 0.0f), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Player::GetLeft() const {
    return -GetRight();
}

Vector3 Player::GetUp() const {
    // クォータニオンから上方向ベクトルを計算（Y軸の正方向が上方向）
    return TransformNormal(Vector3(0.0f, 1.0f, 0.0f), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Player::GetDown() const {
    return -GetUp();
}

Vector3 Player::GetPositionBehind(float distance) const {
    return transform_->translation_ + GetBackward() * distance;
}

Vector3 Player::GetPositionFront(float distance) const {
    return transform_->translation_ + GetForward() * distance;
}

Vector3 Player::GetPositionRight(float distance) const {
    return transform_->translation_ + GetRight() * distance;
}

Vector3 Player::GetPositionLeft(float distance) const {
    return transform_->translation_ + GetLeft() * distance;
}

Vector3 Player::GetPositionAbove(float distance) const {
    return transform_->translation_ + GetUp() * distance;
}

Vector3 Player::GetPositionBelow(float distance) const {
    return transform_->translation_ + GetDown() * distance;
}

ViewProjection &Player::GetViewProjection() {
    return *vp_;
}

void Player::SetCamera(FollowCamera *camera) {
    FollowCamera_ = camera;
}

void Player::SetVp(ViewProjection *vp) {
    vp_ = vp;
    shake_->Initialize(vp_);
}

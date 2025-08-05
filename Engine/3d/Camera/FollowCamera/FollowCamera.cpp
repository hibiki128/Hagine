#include "FollowCamera.h"
#include "Input.h"
#include <cmath>

void FollowCamera::Init() {
    viewProjection_.farZ = 1100;
    viewProjection_.Initialize();
    worldTransform_.Initialize();
    yaw_ = 0.0f;                 // 水平回転角度を初期化
    distanceFromTarget_ = -7.0f; // ターゲットからの距離を初期化
    heightOffset_ = 1.5f;        // ターゲットの上方オフセット
}

void FollowCamera::Update() {
    // 追従対象がいれば
    if (target_) {
        // カメラの回転処理
        Move();

        // カメラの位置をターゲット基準で計算
        Vector3 targetPosition = target_->translation_;
        worldTransform_.translation_.x = targetPosition.x + std::sin(yaw_) * distanceFromTarget_;
        worldTransform_.translation_.z = targetPosition.z + std::cos(yaw_) * distanceFromTarget_;
        worldTransform_.translation_.y = targetPosition.y + heightOffset_;

        // カメラがターゲットの方向を向くよう回転を設定
        Vector3 lookAt = targetPosition - worldTransform_.translation_;
        worldTransform_.quateRotation_.y = std::atan2(lookAt.x, lookAt.z);

        // 親子関係の更新
        worldTransform_.UpdateMatrix();
    }

    // ビュー行列にカメラの位置を反映
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
    viewProjection_.matWorld_ = worldTransform_.matWorld_;

    // ビュー行列の更新
    viewProjection_.UpdateMatrix();
}

void FollowCamera::imgui() {
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat3("wt position", &worldTransform_.translation_.x, 0.1f);
    ImGui::DragFloat3("vp position", &viewProjection_.translation_.x, 0.1f);
    ImGui::End();
}

void FollowCamera::Move() {
    if (Input::GetInstance()->PushKey(DIK_LEFT)) {
        yaw_ -= 0.04f; // 左回転
    }
    if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
        yaw_ += 0.04f; // 右回転
    }
}
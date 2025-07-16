#include "FollowCamera.h"
#include "Easing.h"
#include "Input.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <Engine/Frame/Frame.h>
#include <cmath>

void FollowCamera::Init() {
    viewProjection_.farZ = 1100;
    viewProjection_.Initialize();
    worldTransform_.Initialize();
    yaw_ = 0.0f;                  // 水平回転角度を初期化
    distanceFromTarget_ = -25.0f; // ターゲットからの距離を初期化
    heightOffset_ = 3.0f;         // ターゲットの上方オフセット
}

void FollowCamera::Update() {
    if (target_) {
        // カメラの移動処理
        Move();

        Vector3 targetPos = target_->GetLocalPosition();
        Vector3 velocity = target_->GetVelocity();

        // Yawからカメラの右方向ベクトルを算出
        Vector3 cameraRightDir = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};

        Vector3 cameraPos;

        Player *player = dynamic_cast<Player *>(target_);

        // ロックオン状態に応じて肩オフセットを処理
        if (player && player->GetIsLockOn() && player->GetEnemy()) {
            // ロックオン中のみ肩オフセット機能を有効化
            // プレイヤーの速度をカメラの右方向に投影して、カメラから見た左右移動成分を取得
            float lateralVelocity = velocity.x * cameraRightDir.x + velocity.z * cameraRightDir.z;

            // プレイヤーが動いている場合のみ肩オフセットを更新
            if (std::abs(lateralVelocity) > 0.1f) { // 閾値を設けて微小な動きは無視
                // カメラから見た左右方向の速度から肩の方向を計算（-1〜1にクランプ）
                float dirSign = std::clamp(lateralVelocity / target_->GetMaxSpeed(), -1.0f, 1.0f);

                // 肩の目標オフセット（左右に最大 shoulderMaxOffset_ 分ずらす）
                shoulderOffsetTarget_.x = -dirSign * shoulderMaxOffset_;
            }
            // プレイヤーが止まっている場合は shoulderOffsetTarget_.x を変更しない（現在の値を維持）

        } else {
            // ロックオンしていない場合は中央に戻す
            shoulderOffsetTarget_.x = 0.0f;
        }

        // 肩オフセットを滑らかに補間
        if (player && player->GetIsLockOn() && player->GetEnemy()) {
            // ロックオン中：プレイヤーの状態に基づいて補間を制御
            std::string currentStateName = player->GetCurrentStateName();
            if (currentStateName == "FlyMove" &&
                (!Input::GetInstance()->PushKey(DIK_SPACE) &&
                 !Input::GetInstance()->PushKey(DIK_LSHIFT))) {
                // Idle以外の状態では補間を行う
                shoulderOffsetCurrent_.x = Lerp(shoulderOffsetCurrent_.x, shoulderOffsetTarget_.x, shoulderLerpSpeed_ * Frame::DeltaTime());
            }
            // Idle状態では shoulderOffsetCurrent_.x をそのまま維持（補間しない）
        } else {
            // ロックオンしていない時は常に補間して中央に戻す
            shoulderOffsetCurrent_.x = Lerp(shoulderOffsetCurrent_.x, shoulderOffsetTarget_.x, shoulderLerpSpeed_ * Frame::DeltaTime());
        }

        // カメラ位置の計算
        if (player && player->GetIsLockOn() && player->GetEnemy()) {
            Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
            Vector3 toEnemyDir = enemyPos - targetPos;
            toEnemyDir.y = 0.0f;

            float length = std::sqrt(toEnemyDir.x * toEnemyDir.x + toEnemyDir.z * toEnemyDir.z);
            if (length > 0.001f) {
                toEnemyDir.x /= length;
                toEnemyDir.z /= length;
            }

            cameraPos.x = targetPos.x - toEnemyDir.x * std::abs(cameraOffset_.z);
            cameraPos.z = targetPos.z - toEnemyDir.z * std::abs(cameraOffset_.z);
            cameraPos.y = targetPos.y + cameraOffset_.y;

            // カメラのヨー角更新
            yaw_ = std::atan2(toEnemyDir.x, toEnemyDir.z);
        } else {
            cameraPos.x = targetPos.x + std::sin(yaw_) * cameraOffset_.z;
            cameraPos.z = targetPos.z + std::cos(yaw_) * cameraOffset_.z;
            cameraPos.y = targetPos.y + cameraOffset_.y;
        }

        Vector3 shoulderOffset = cameraRightDir * shoulderOffsetCurrent_.x;
        cameraPos += shoulderOffset;

        worldTransform_.translation_ = cameraPos;

        // クォータニオンで回転をセット（yawのみ回転、ピッチ・ロールは0）
        worldTransform_.rotation_ = Quaternion::FromEulerAngles({0.0f, -yaw_, 0.0f});

        // 行列更新
        worldTransform_.UpdateMatrix();
    }

    // ビュープロジェクション更新
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.rotation_ = worldTransform_.rotation_;
    viewProjection_.matWorld_ = worldTransform_.matWorld_;
    viewProjection_.UpdateMatrix();
}
void FollowCamera::imgui() {
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat3("wt position", &worldTransform_.translation_.x, 0.1f);
    ImGui::DragFloat3("vp position", &viewProjection_.translation_.x, 0.1f);
    ImGui::DragFloat3("offsetCurrent", &shoulderOffsetCurrent_.x, 0.1f);
    ImGui::DragFloat3("offsetTarget", &shoulderOffsetTarget_.x, 0.1f);
    ImGui::End();
}

void FollowCamera::Move() {
    Player *player = dynamic_cast<Player *>(target_);
    if (!player || !player->GetIsLockOn()) {
        if (Input::GetInstance()->PushKey(DIK_LEFT)) {
            yaw_ -= 0.04f;
        }
        if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
            yaw_ += 0.04f;
        }
    }
}
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

        Vector3 cameraPos;

        Player *player = dynamic_cast<Player *>(target_);

        // Rush状態の場合は追従を停止
        if (player) {
            std::string currentStateName = player->GetCurrentStateName();
            if (currentStateName == "Rush") {
                // プレイヤーが目標位置に近づいているかチェック
                Vector3 currentPos = player->GetLocalPosition();

                if (player->GetIsLockOn() && player->GetEnemy()) {
                    Vector3 targetPos = player->GetEnemy()->GetPositionBehind(3.0f);
                    float distanceToTarget = (targetPos - currentPos).Length();

                    // 目標位置に十分近づいていない場合はカメラ追従を停止
                    if (distanceToTarget > rushCameraResumeDistance_) {
                        // Rush中のカメラ位置・回転を保存
                        rushCameraPosition_ = worldTransform_.translation_;
                        rushCameraRotation_ = worldTransform_.quateRotation_;
                        isResumeFromRush_ = false;

                        worldTransform_.UpdateMatrix();
                        viewProjection_.translation_ = worldTransform_.translation_;
                        viewProjection_.rotation_ = worldTransform_.quateRotation_;
                        viewProjection_.matWorld_ = worldTransform_.matWorld_;
                        viewProjection_.UpdateMatrix();
                        return;
                    } else {
                        // 追従再開の準備
                        if (!isResumeFromRush_) {
                            isResumeFromRush_ = true;
                            rushCameraPosition_ = worldTransform_.translation_;
                            rushCameraRotation_ = worldTransform_.quateRotation_;
                        }
                    }
                }
            } else {
                // Rush状態以外になったらフラグをリセット
                if (isResumeFromRush_) {
                    isResumeFromRush_ = false;
                }
            }
        }

        // ロックオン状態に応じて肩オフセットを処理
        if (player && player->GetIsLockOn() && player->GetEnemy()) {
            // カメラ位置とヨー角の計算を先に行う
            Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
            Vector3 toEnemyDir = enemyPos - targetPos;

            // yaw_を更新（肩オフセット計算のため）
            Vector3 toEnemyDirXZ = {toEnemyDir.x, 0.0f, toEnemyDir.z};
            float lengthXZ = toEnemyDirXZ.Length();
            if (lengthXZ > 0.001f) {
                toEnemyDirXZ = toEnemyDirXZ.Normalize();
                yaw_ = std::atan2(toEnemyDirXZ.x, toEnemyDirXZ.z);
            }

            // Yawからカメラの右方向ベクトルを算出（更新されたyaw_を使用）
            Vector3 cameraRightDir = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};

            // 肩オフセット処理
            float lateralVelocity = velocity.x * cameraRightDir.x + velocity.z * cameraRightDir.z;

            if (std::abs(lateralVelocity) > 0.1f) {
                float dirSign = std::clamp(lateralVelocity / target_->GetMaxSpeed(), -1.0f, 1.0f);
                shoulderOffsetTarget_.x = -dirSign * shoulderMaxOffset_;
            } else {
                // 入力がない場合はデフォルトで右肩にオフセット
                shoulderOffsetTarget_.x = shoulderMaxOffset_;
            }
        } else {
            shoulderOffsetTarget_.x = 0.0f;
        }

        // Yawからカメラの右方向ベクトルを算出
        Vector3 cameraRightDir = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};

        // 肩オフセットを滑らかに補間
        if (player && player->GetIsLockOn() && player->GetEnemy()) {
            std::string currentStateName = player->GetCurrentStateName();
            if (currentStateName == "FlyMove") {
                shoulderOffsetCurrent_.x = Lerp(shoulderOffsetCurrent_.x, shoulderOffsetTarget_.x, shoulderLerpSpeed_ * Frame::DeltaTime());
            }
        } else {
            shoulderOffsetCurrent_.x = Lerp(shoulderOffsetCurrent_.x, shoulderOffsetTarget_.x, shoulderLerpSpeed_ * Frame::DeltaTime());
        }

        // カメラ位置とヨー角の計算
        if (player && player->GetIsLockOn() && player->GetEnemy()) {
            Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
            Vector3 toEnemyDir = enemyPos - targetPos;

            // 3D空間での敵への方向を計算（Y軸も含む）
            float length = toEnemyDir.Length();
            if (length > 0.001f) {
                toEnemyDir = toEnemyDir.Normalize();
            }

            // カメラ位置を敵の反対方向に配置
            cameraPos = targetPos - toEnemyDir * std::abs(cameraOffset_.z);

            // カメラの向きを敵の方向に設定（3軸すべて）
            Vector3 forward = toEnemyDir;
            Vector3 worldUp = {0.0f, 1.0f, 0.0f};

            // forwardとworldUpが平行になる場合の対処
            Vector3 right;
            if (std::abs(forward.Dot(worldUp)) > 0.999f) {
                right = {1.0f, 0.0f, 0.0f};
            } else {
                right = (worldUp.Cross(forward)).Normalize();
            }

            Vector3 up = (forward.Cross(right)).Normalize();

            // 回転行列から目標クォータニオンを作成
            Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
            Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

            // カメラの回転を滑らかに補間
            float rotateSpeed = 10.0f;
            worldTransform_.quateRotation_ = Quaternion::Slerp(worldTransform_.quateRotation_, targetRot, rotateSpeed * Frame::DeltaTime());
        } else {
            cameraPos.x = targetPos.x + std::sin(yaw_) * cameraOffset_.z;
            cameraPos.z = targetPos.z + std::cos(yaw_) * cameraOffset_.z;
            cameraPos.y = targetPos.y + cameraOffset_.y;

            // 非ロックオン時は従来通り
            worldTransform_.quateRotation_ = Quaternion::FromEulerAngles({0.0f, -yaw_, 0.0f});
        }

        Vector3 shoulderOffset = cameraRightDir * shoulderOffsetCurrent_.x;
        cameraPos += shoulderOffset;

        
        if (isResumeFromRush_) {
            // 通常の追従で計算された位置・回転を目標値として使用
            Vector3 targetCameraPos = worldTransform_.translation_;
            Quaternion targetCameraRot = worldTransform_.quateRotation_;

            // Rush中の固定位置から目標位置へ高速補間
            worldTransform_.translation_ = Lerp(rushCameraPosition_, targetCameraPos, rushResumeBlendSpeed_ * Frame::DeltaTime());
            worldTransform_.quateRotation_ = Quaternion::Slerp(rushCameraRotation_, targetCameraRot, rushResumeBlendSpeed_ * Frame::DeltaTime());

            // 補間がほぼ完了したらフラグを解除
            float positionDiff = (worldTransform_.translation_ - targetCameraPos).Length();
            float rotationDiff = std::abs(1.0f - std::abs(worldTransform_.quateRotation_.Dot(targetCameraRot)));

            if (positionDiff < 0.5f && rotationDiff < 0.01f) {
                isResumeFromRush_ = false;
            }

            // Rush復帰補間中の値を更新
            rushCameraPosition_ = worldTransform_.translation_;
            rushCameraRotation_ = worldTransform_.quateRotation_;
        }

        worldTransform_.translation_ = cameraPos;

        // 行列更新
        worldTransform_.UpdateMatrix();
    }

    // ビュープロジェクション更新
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.rotation_ = worldTransform_.quateRotation_;
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
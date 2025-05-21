#include "FollowCamera.h"
#include "Easing.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>
#include"application/GameObject/Enemy/Enemy.h"

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

        // プレイヤーの現在位置
        Vector3 targetPos = target_->GetWorldPosition();

        // プレイヤーの速度ベクトル
        Vector3 velocity = target_->GetVelocity();

        // X軸方向の速度から肩の方向を計算（-1〜1にクランプ）
        float dirSign = std::clamp(velocity.x / target_->GetMaxSpeed(), -1.0f, 1.0f);

        // 肩の目標オフセット（左右に最大 shoulderMaxOffset_ 分ずらす）
        shoulderOffsetTarget_.x = -dirSign * shoulderMaxOffset_;

        // 肩オフセットを滑らかに補間
        shoulderOffsetCurrent_.x = Lerp(shoulderOffsetCurrent_.x, shoulderOffsetTarget_.x, shoulderLerpSpeed_ * ImGui::GetIO().DeltaTime);

        Vector3 cameraPos;

        // プレイヤーがロックオン状態かチェック
        Player *player = dynamic_cast<Player *>(target_);
        if (player && player->GetIsLockOn() && player->GetEnemy()) {
            // ロックオン中は常にプレイヤーの後ろにカメラを配置

            // プレイヤーからエネミーへの方向を計算
            Vector3 enemyPos = player->GetEnemy()->GetWorldPosition();
            Vector3 toEnemyDir = enemyPos - targetPos;
            toEnemyDir.y = 0.0f; // Y軸方向は無視して水平面での方向のみ考慮

            // 方向ベクトルを正規化
            float length = std::sqrt(toEnemyDir.x * toEnemyDir.x + toEnemyDir.z * toEnemyDir.z);
            if (length > 0.001f) {
                toEnemyDir.x /= length;
                toEnemyDir.z /= length;
            }

            // プレイヤーの後ろにカメラを配置（エネミーの反対方向）
            cameraPos.x = targetPos.x - toEnemyDir.x * std::abs(cameraOffset_.z);
            cameraPos.z = targetPos.z - toEnemyDir.z * std::abs(cameraOffset_.z);
            cameraPos.y = targetPos.y + cameraOffset_.y;

            // カメラの向き（ヨー角）を更新（エネミー方向を向くように）
            yaw_ = std::atan2(toEnemyDir.x, toEnemyDir.z);
        } else {
            // 通常時はyaw_角度に基づいてカメラを配置
            cameraPos.x = targetPos.x + std::sin(yaw_) * cameraOffset_.z;
            cameraPos.z = targetPos.z + std::cos(yaw_) * cameraOffset_.z;
            cameraPos.y = targetPos.y + cameraOffset_.y;
        }

        // 肩オフセットを適用
        cameraPos += shoulderOffsetCurrent_;

        // カメラのワールド座標に反映
        worldTransform_.translation_ = cameraPos;

        // 回転も更新（カメラはプレイヤーの方を向くように）
        worldTransform_.rotation_.y = yaw_;

        // 行列を更新
        worldTransform_.UpdateMatrix();
    }

    // ビュープロジェクションの更新
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.rotation_ = worldTransform_.rotation_;
    viewProjection_.matWorld_ = worldTransform_.matWorld_;
    viewProjection_.UpdateMatrix();
}

void FollowCamera::imgui() {
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat3("wt position", &worldTransform_.translation_.x, 0.1f);
    ImGui::DragFloat3("vp position", &viewProjection_.translation_.x, 0.1f);
    ImGui::End();
}

void FollowCamera::Move() {
    // ロックオン中でない場合のみキー入力でカメラを回転させる
    Player *player = dynamic_cast<Player *>(target_);
    if (!player || !player->GetIsLockOn()) {
        if (Input::GetInstance()->PushKey(DIK_LEFT)) {
            yaw_ -= 0.04f; // 左回転
        }
        if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
            yaw_ += 0.04f; // 右回転
        }
    }
    // ロックオン中はUpdate内でカメラ位置と向きが自動的に更新される
}
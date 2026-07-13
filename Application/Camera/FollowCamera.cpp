#define NOMINMAX
#include "FollowCamera.h"
#include "Input.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

using namespace Hagine;

FollowCamera::FollowCamera() {
    lockOn_ = std::make_unique<CameraLockOn>();
    rush_ = std::make_unique<CameraRush>();
    skillCutscene_ = std::make_unique<CameraSkillCutscene>();
}

FollowCamera::~FollowCamera() {
    // "カメラ演出" の登録解除は CameraSkillCutscene のデストラクタで行う
}

void FollowCamera::Init() {
    // ViewProjectionの初期設定
    viewProjection_.farZ_ = kFarZ;
    viewProjection_.Initialize("");
    worldTransform_.Initialize();
    yaw_ = kInitialYaw;

    // ─── 各パーツの初期化 ───
    lockOn_->Init(this);
    rush_->Init(this);
    skillCutscene_->Init(this);
}

void FollowCamera::Update() {
    // ターゲットが存在しない場合は処理を行わない
    if (!target_)
        return;

    // 必殺技の顔アップ演出中は専用処理でカメラを確定する
    if (skillCutscene_->UpdateSkillCloseUp()) {
        return;
    }

    // カメラの入力移動処理（手動ヨー回転）
    Move();

    Player *player = dynamic_cast<Player *>(target_);
    const Vector3 targetPos = target_->GetLocalPosition();
    const Vector3 velocity = target_->GetVelocity();

    // ロックオン状態の取得
    const bool isCurrentlyLockedOn = player && player->GetIsLockOn() && player->GetEnemy();

    // ロックオンの開始/解除に伴う肩オフセットの切り替え
    lockOn_->UpdateLockOnTransition(isCurrentlyLockedOn);

    // Rush中の専用カメラ。遠距離追従ではここでカメラを確定し、以降の通常処理をスキップする
    if (rush_->UpdateRushCamera(player)) {
        return;
    }

    // ロックオン時のみ：肩オフセット目標と高さオフセットを更新
    if (isCurrentlyLockedOn) {
        lockOn_->UpdateLockOnShoulderAndHeight(player, targetPos, velocity);
    }

    // 肩オフセットの補間（解除時の戻り or 通常追従）
    lockOn_->UpdateShoulderOffset();

    // 最終的なカメラ位置・回転を算出
    const Vector3 cameraPos = ComputeCameraTransform(isCurrentlyLockedOn, player, targetPos);

    // Rush演出からの復帰補間、または位置の確定
    rush_->ApplyCameraPosition(cameraPos);

    // worldTransform_ の位置・回転を ViewProjection へ反映して行列を更新する
    ApplyToViewProjection();

    // 視錐台ロックオン判定の更新
    lockOn_->UpdateFrustumLockOn();
}

void FollowCamera::Move() {
    Player *player = dynamic_cast<Player *>(target_);
    GamePad *gamePad = player->GetGamePad();

    // ロックオン中は手動回転を受け付けない
    if (player && player->GetIsLockOn()) {
        return;
    }

    if (!gamePad->IsConnected()) {
        Input *input = Input::GetInstance();
        // キーボードによる手動回転
        if (input->PushKey(DIK_LEFT)) {
            yaw_ -= manualYawSpeed_;
        }
        if (input->PushKey(DIK_RIGHT)) {
            yaw_ += manualYawSpeed_;
        }
    } else {
        // ゲームパッドによる手動回転
        if (player) {
            const float stickSensitivity = 0.05f;
            yaw_ += gamePad->GetRightStickX() * stickSensitivity;
        }
    }
}

Vector3 FollowCamera::ComputeCameraTransform(bool isCurrentlyLockedOn, Player *player, const Vector3 &targetPos) {
    Vector3 cameraPos;

    if (isCurrentlyLockedOn) {
        // ロックオン時：敵の方向を基準にした計算
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();
        Vector3 toEnemyDir = enemyPos - targetPos;
        float length = toEnemyDir.Length();
        if (length > kEpsilon)
            toEnemyDir = toEnemyDir.Normalize();

        Vector3 forward = toEnemyDir;
        Vector3 right = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};
        Vector3 up = (forward.Cross(right)).Normalize();

        Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
        worldTransform_.quateRotation_ = Quaternion::FromMatrix(rotMatrix);

        // 各種オフセットの反映
        cameraPos = targetPos - forward * std::abs(cameraOffset_.z);
        cameraPos += up * lockOn_->GetLockOnHeightOffsetCurrent();
        cameraPos += right * lockOn_->GetShoulderOffsetX();
    } else {
        // 非ロックオン時：手動ヨー角を基準にした計算
        cameraPos.x = targetPos.x + std::sin(yaw_) * cameraOffset_.z;
        cameraPos.z = targetPos.z + std::cos(yaw_) * cameraOffset_.z;
        cameraPos.y = targetPos.y + cameraOffset_.y;
        worldTransform_.quateRotation_ = Quaternion::FromEulerAngles({kVectorZero, -yaw_, kVectorZero});

        Vector3 right = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};
        cameraPos += right * lockOn_->GetShoulderOffsetX();
    }

    return cameraPos;
}

void FollowCamera::ApplyToViewProjection() {
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.isUseQuaternion_ = true;
    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
    viewProjection_.UpdateMatrix();
}

void FollowCamera::imgui() {
#ifdef USE_IMGUI
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat3("wt position", &worldTransform_.translation_.x, 0.1f);
    ImGui::DragFloat3("vp position", &viewProjection_.translation_.x, 0.1f);

    // ロックオン（肩・高さ・イージング・視錐台）
    lockOn_->DrawImGui();

    // Rush（突進）カメラのイージング
    rush_->DrawImGui();

    // 必殺技の顔アップ演出
    skillCutscene_->DrawImGui();

    ImGui::End();
#endif
}

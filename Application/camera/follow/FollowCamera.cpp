#define NOMINMAX
#include "FollowCamera.h"
#include <Input.h>
#include <Application/entity/enemy/Enemy.h>
#include <Application/entity/field/around/AroundField.h>
#include <Application/entity/field/ground/Ground.h>
#include <Application/entity/player/Player.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;

FollowCamera::FollowCamera()
{
    pLockOn_ = std::make_unique<CameraLockOn>();
    pRush_ = std::make_unique<CameraRush>();
    pSkillCutscene_ = std::make_unique<CameraSkillCutscene>();
}

FollowCamera::~FollowCamera()
{
    // "カメラ演出" の登録解除は CameraSkillCutscene のデストラクタで行う
}

void FollowCamera::Init()
{
    // ViewProjectionの初期設定
    viewProjection_.farZ_ = kFarZ;
    viewProjection_.Initialize("");
    worldTransform_.Initialize();
    yaw_ = kInitialYaw;

    // ─── 各パーツの初期化 ───
    pLockOn_->Init(this);
    pRush_->Init(this);
    pSkillCutscene_->Init(this);
}

void FollowCamera::Update()
{
    // ターゲットが存在しない場合は処理を行わない
    if (!pTarget_)
        return;

    // 必殺技の顔アップ演出中は専用処理でカメラを確定する
    if (pSkillCutscene_->UpdateSkillCloseUp())
    {
        return;
    }

    // カメラの入力移動処理（手動ヨー回転）
    Move();

    Player *pPlayer = dynamic_cast<Player *>(pTarget_);
    const Vector3 targetPos = pTarget_->GetLocalPosition();
    const Vector3 velocity = pTarget_->GetVelocity();

    // ロックオン状態の取得
    const bool isCurrentlyLockedOn = pPlayer && pPlayer->GetIsLockOn() && pPlayer->GetEnemy();

    // ロックオンの開始/解除に伴う肩オフセットの切り替え
    pLockOn_->UpdateLockOnTransition(isCurrentlyLockedOn);

    // Rush中の専用カメラ。遠距離追従ではここでカメラを確定し、以降の通常処理をスキップする
    if (pRush_->UpdateRushCamera(pPlayer))
    {
        return;
    }

    // ロックオン時のみ：肩オフセット目標と高さオフセットを更新
    if (isCurrentlyLockedOn)
    {
        pLockOn_->UpdateLockOnShoulderAndHeight(pPlayer, targetPos, velocity);
    }

    // 肩オフセットの補間（解除時の戻り or 通常追従）
    pLockOn_->UpdateShoulderOffset();

    // 最終的なカメラ位置・回転を算出
    const Vector3 cameraPos = ComputeCameraTransform(isCurrentlyLockedOn, pPlayer, targetPos);

    // Rush演出からの復帰補間、または位置の確定
    pRush_->ApplyCameraPosition(cameraPos);

    // 地形・フィールド境界との衝突解消（すり抜け防止）
    ResolveCameraCollision();

    // worldTransform_ の位置・回転を ViewProjection へ反映して行列を更新する
    ApplyToViewProjection();

    // 視錐台ロックオン判定の更新
    pLockOn_->UpdateFrustumLockOn();
}

void FollowCamera::Move()
{
    Player *pPlayer = dynamic_cast<Player *>(pTarget_);
    GamePad *pGamePad = pPlayer->GetGamePad();

    // ロックオン中は手動回転を受け付けない
    if (pPlayer && pPlayer->GetIsLockOn())
    {
        return;
    }

    if (!pGamePad->IsConnected())
    {
        Input *pInput = Input::GetInstance();
        // キーボードによる手動回転
        if (pInput->PushKey(DIK_LEFT))
        {
            yaw_ -= manualYawSpeed_;
        }
        if (pInput->PushKey(DIK_RIGHT))
        {
            yaw_ += manualYawSpeed_;
        }
    }
    else
    {
        // ゲームパッドによる手動回転
        if (pPlayer)
        {
            const float stickSensitivity = 0.05f;
            yaw_ += pGamePad->GetRightStickX() * stickSensitivity;
        }
    }
}

Vector3 FollowCamera::ComputeCameraTransform(bool isCurrentlyLockedOn, Player *pPlayer, const Vector3 &targetPos)
{
    Vector3 cameraPos;

    if (isCurrentlyLockedOn)
    {
        // ロックオン時：敵の方向を基準にした計算
        Vector3 enemyPos = pPlayer->GetEnemy()->GetLocalPosition();
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
        cameraPos += up * pLockOn_->GetLockOnHeightOffsetCurrent();
        cameraPos += right * pLockOn_->GetShoulderOffsetX();
    }
    else
    {
        // 非ロックオン時：手動ヨー角を基準にした計算
        cameraPos.x = targetPos.x + std::sin(yaw_) * cameraOffset_.z;
        cameraPos.z = targetPos.z + std::cos(yaw_) * cameraOffset_.z;
        cameraPos.y = targetPos.y + cameraOffset_.y;
        worldTransform_.quateRotation_ = Quaternion::FromEulerAngles({kVectorZero, -yaw_, kVectorZero});

        Vector3 right = {std::cos(yaw_), kVectorZero, -std::sin(yaw_)};
        cameraPos += right * pLockOn_->GetShoulderOffsetX();
    }

    return cameraPos;
}

void FollowCamera::ApplyToViewProjection()
{
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.isUseQuaternion_ = true;
    viewProjection_.quateRotation_ = worldTransform_.quateRotation_;
    viewProjection_.UpdateMatrix();
}

void FollowCamera::ResolveCameraCollision()
{
    if (!pTarget_)
    {
        return;
    }

    Vector3 cameraPos = worldTransform_.translation_;

    //ResolveFieldCollision(cameraPos);
    ResolveTerrainCollision(cameraPos);

    if ((cameraPos - worldTransform_.translation_).LengthSq() > 0.0f)
    {
        worldTransform_.translation_ = cameraPos;
        worldTransform_.UpdateMatrix();
    }
}

void FollowCamera::ResolveFieldCollision(Vector3 &cameraPos)
{
    // ─── AroundField 円柱境界の内側へクランプ ───
    const CylinderCollider *field = AroundField::GetFieldCollider();
    if (!field)
    {
        return;
    }

    const Vector3 center = field->GetCenterPosition();
    const float maxRadius = field->GetRadius() - kFieldClampMargin;

    Vector3 horizontal = {cameraPos.x - center.x, 0.0f, cameraPos.z - center.z};
    float dist = horizontal.Length();
    if (dist > maxRadius && dist > kEpsilon)
    {
        float scale = maxRadius / dist;
        cameraPos.x = center.x + horizontal.x * scale;
        cameraPos.z = center.z + horizontal.z * scale;
    }

    const float halfHeight = field->GetHeight() * 0.5f;
    cameraPos.y = std::clamp(cameraPos.y,
                             center.y - halfHeight + kFieldClampMargin,
                             center.y + halfHeight - kFieldClampMargin);
}

void FollowCamera::ResolveTerrainCollision(Vector3 &cameraPos)
{
    // ─── 地形メッシュとの遮蔽・めり込み解消 ───
    MeshCollider *terrain = Ground::GetTerrainCollider();
    if (!terrain)
    {
        return;
    }

    // 注視点（プレイヤーの少し上）からカメラへレイを飛ばし、
    // 地形に遮られていたらヒット位置の手前へ引き寄せる
    Vector3 pivot = pTarget_->GetLocalPosition();
    pivot.y += kCameraPivotHeight;

    Vector3 toCamera = cameraPos - pivot;
    float distance = toCamera.Length();
    if (distance > kEpsilon)
    {
        Vector3 dir = toCamera / distance;
        float hitDistance = 0.0f;
        Vector3 hitNormal;
        if (terrain->Raycast(pivot, dir, distance, hitDistance, hitNormal))
        {
            float clamped = (std::max)(hitDistance - kCameraCollisionMargin, kCameraMinDistance);
            cameraPos = pivot + dir * clamped;
        }
    }

    // 地表すれすれ・地面下に潜るのを防ぐ最低高度を確保する
    float floorY = Ground::GetSurfaceY(cameraPos.x, cameraPos.z) + kCameraFloorClearance;
    if (cameraPos.y < floorY)
    {
        cameraPos.y = floorY;
    }
}

void FollowCamera::imgui()
{
#ifdef USE_IMGUI
    ImGui::Begin("FollowCamera");
    ImGui::DragFloat3("wt position", &worldTransform_.translation_.x, 0.1f);
    ImGui::DragFloat3("vp position", &viewProjection_.translation_.x, 0.1f);

    // ロックオン（肩・高さ・イージング・視錐台）
    pLockOn_->DrawImGui();

    // Rush（突進）カメラのイージング
    pRush_->DrawImGui();

    // 必殺技の顔アップ演出
    pSkillCutscene_->DrawImGui();

    ImGui::End();
#endif
}

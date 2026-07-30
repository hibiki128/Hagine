#define NOMINMAX
#include "FollowCamera.h"
#include "parts/CameraLockOn.h"
#include "parts/CameraRush.h"
#include "parts/CameraSkillCutscene.h"
#include <Frame.h>
#include <Input.h>
#include <Application/entity/enemy/Enemy.h>
#include <Application/entity/field/around/AroundField.h>
#include <Application/entity/field/ground/Ground.h>
#include <Application/entity/player/Player.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;

FollowCamera::FollowCamera(std::unique_ptr<CameraLockOn> pLockOn,
                           std::unique_ptr<CameraRush> pRush,
                           std::unique_ptr<CameraSkillCutscene> pSkillCutscene)
    : pLockOn_(std::move(pLockOn)),
      pRush_(std::move(pRush)),
      pSkillCutscene_(std::move(pSkillCutscene))
{
    // パーツの生成は行わない（注入されたものを受け取るだけ）
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

    // 瞬間移動コンボのカメラ固定中は、追従計算を行わず旧位置に留める。
    // 時間切れになった次フレームから通常追従へ戻るため、瞬間移動先へパッとスナップする
    if (holdTimer_ > 0.0f)
    {
        holdTimer_ -= Hagine::Frame::DeltaTime();
        ApplyToViewProjection(); // 凍結した worldTransform_ をそのまま反映
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
            yaw_ += pGamePad->GetRightStickX() * kStickYawSensitivity;
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

        // 敵方向をそのまま forward にすると、敵の真上/真下で up との外積が退化して暴れる。
        // ヨー＋クランプしたピッチから forward を構築して防ぐ
        float xzLen = std::sqrt(toEnemyDir.x * toEnemyDir.x + toEnemyDir.z * toEnemyDir.z);
        float pitch = std::atan2(toEnemyDir.y, xzLen); // +: 敵が上 / -: 敵が下
        pitch = std::clamp(pitch, -kMaxLockOnPitch, kMaxLockOnPitch);

        Vector3 forward = {std::sin(yaw_) * std::cos(pitch),
                           std::sin(pitch),
                           std::cos(yaw_) * std::cos(pitch)};
        Vector3 right = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};
        Vector3 up = (forward.Cross(right)).Normalize();

        Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
        worldTransform_.quaternionRotation_ = Quaternion::FromMatrix(rotMatrix);

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
        worldTransform_.quaternionRotation_ = Quaternion::FromEulerAngles({0.0f, -yaw_, 0.0f});

        Vector3 right = {std::cos(yaw_), 0.0f, -std::sin(yaw_)};
        cameraPos += right * pLockOn_->GetShoulderOffsetX();
    }

    return cameraPos;
}

void FollowCamera::ApplyToViewProjection()
{
    viewProjection_.translation_ = worldTransform_.translation_;
    viewProjection_.isUseQuaternion_ = true;
    viewProjection_.quaternionRotation_ = worldTransform_.quaternionRotation_;
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
    const CylinderCollider *pField = AroundField::GetFieldCollider();
    if (!pField)
    {
        return;
    }

    const Vector3 center = pField->GetCenterPosition();
    const float maxRadius = pField->GetRadius() - kFieldClampMargin;

    Vector3 horizontal = {cameraPos.x - center.x, 0.0f, cameraPos.z - center.z};
    float dist = horizontal.Length();
    if (dist > maxRadius && dist > kEpsilon)
    {
        float scale = maxRadius / dist;
        cameraPos.x = center.x + horizontal.x * scale;
        cameraPos.z = center.z + horizontal.z * scale;
    }

    const float halfHeight = pField->GetHeight() * 0.5f;
    cameraPos.y = std::clamp(cameraPos.y,
                             center.y - halfHeight + kFieldClampMargin,
                             center.y + halfHeight - kFieldClampMargin);
}

void FollowCamera::ResolveTerrainCollision(Vector3 &cameraPos)
{
    // ─── 地形メッシュとの遮蔽・めり込み解消 ───
    MeshCollider *pTerrain = Ground::GetTerrainCollider();
    if (!pTerrain)
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
        if (pTerrain->Raycast(pivot, dir, distance, hitDistance, hitNormal))
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

/// ===================================================
/// パーツへの委譲（ファサードの窓口）
/// ヘッダーではパーツを前方宣言のみに留めるため、実体はここで定義する
/// ===================================================

void FollowCamera::DrawFrustum()
{
    pLockOn_->DrawFrustum();
}

void FollowCamera::UpdateFrustumLockOn()
{
    pLockOn_->UpdateFrustumLockOn();
}

void FollowCamera::DrawLockOnFrustum(LineRenderer *pLineRenderer) const
{
    pLockOn_->DrawLockOnFrustum(pLineRenderer);
}

void FollowCamera::StartSkillCloseUp(BaseObject *pPerformer)
{
    pSkillCutscene_->StartSkillCloseUp(pPerformer);
}

void FollowCamera::EndSkillCloseUp()
{
    pSkillCutscene_->EndSkillCloseUp();
}

bool FollowCamera::IsSkillCloseUpActive() const
{
    return pSkillCutscene_->IsSkillCloseUpActive();
}

float FollowCamera::GetLockOnRange() const
{
    return pLockOn_->GetLockOnRange();
}

float FollowCamera::GetLockOnHalfFovH() const
{
    return pLockOn_->GetLockOnHalfFovH();
}

float FollowCamera::GetLockOnHalfFovV() const
{
    return pLockOn_->GetLockOnHalfFovV();
}

bool FollowCamera::GetDrawLockOnFrustumDebug() const
{
    return pLockOn_->GetDrawLockOnFrustumDebug();
}

void FollowCamera::SetLockOnRange(float range)
{
    pLockOn_->SetLockOnRange(range);
}

void FollowCamera::SetLockOnHalfFovH(float radian)
{
    pLockOn_->SetLockOnHalfFovH(radian);
}

void FollowCamera::SetLockOnHalfFovV(float radian)
{
    pLockOn_->SetLockOnHalfFovV(radian);
}

void FollowCamera::SetDrawLockOnFrustumDebug(bool isDraw)
{
    pLockOn_->SetDrawLockOnFrustumDebug(isDraw);
}

void FollowCamera::DrawImGui()
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

#define NOMINMAX
#include "CameraSkillCutscene.h"
#include <Application/camera/follow/FollowCamera.h>
#include <frame/Frame.h>
#include <object/base/BaseObject.h>
#include <utility/debug/param/GameParamHub.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;

void CameraSkillCutscene::Init(FollowCamera *pOwner)
{
    pOwner_ = pOwner;

    // 必殺技の顔アップ演出パラメータをゲームパラメータHubへ登録する
    auto *pHub = GameParamHub::GetInstance();
    pHub->Register("カメラ演出", "顔からの距離", &closeUpDistance_, {0.1f, 1.0f, 30.0f});
    pHub->Register("カメラ演出", "顔の高さオフセット", &closeUpFaceHeight_, {0.1f, 0.0f, 10.0f});
    pHub->Register("カメラ演出", "回り込み速度", &closeUpApproachSpeed_, {0.1f, 0.5f, 30.0f});
}

CameraSkillCutscene::~CameraSkillCutscene()
{
    // ポインタ失効前にゲームパラメータHubから登録を解除する
    GameParamHub::GetInstance()->Unregister("カメラ演出");
}

bool CameraSkillCutscene::UpdateSkillCloseUp()
{
    if (!pSkillCloseUpTarget_)
    {
        return false;
    }

    const float deltaTime = Frame::DeltaTime();

    // 対象の正面方向（+Z基準）。演出中も対象が照準追従で回るため毎フレーム取り直す
    const Vector3 basePos = pSkillCloseUpTarget_->GetWorldPosition();
    Matrix4x4 rotMat = QuaternionToMatrix4x4(pSkillCloseUpTarget_->GetLocalRotation());
    Vector3 forward = TransformNormal(Vector3(kVectorZero, kVectorZero, 1.0f), rotMat);

    // 回り込みは水平面で行い、高さは顔オフセットで合わせる
    forward.y = kVectorZero;
    if (forward.Length() < kEpsilon)
    {
        forward = {kVectorZero, kVectorZero, 1.0f};
    }
    forward = forward.Normalize();

    const Vector3 facePos = basePos + Vector3(kVectorZero, closeUpFaceHeight_, kVectorZero);
    const Vector3 goalPos = facePos + forward * closeUpDistance_;

    WorldTransform &wt = pOwner_->GetCameraWorldTransform();

    // 現在位置から目標へ指数補間で回り込む（目標が動いても滑らかに追従する）
    float t = std::min(closeUpApproachSpeed_ * deltaTime, kMaxBlendValue);
    wt.translation_ += (goalPos - wt.translation_) * t;

    // 常に顔を注視する
    Vector3 look = facePos - wt.translation_;
    if (look.Length() > kEpsilon)
    {
        look = look.Normalize();
        Vector3 worldUp = {kVectorZero, kUpVectorY, kVectorZero};

        Vector3 right;
        if (std::abs(look.Dot(worldUp)) > kParallelThreshold)
        {
            right = {kRightVectorX, kVectorZero, kVectorZero};
        }
        else
        {
            right = (worldUp.Cross(look)).Normalize();
        }
        Vector3 up = (look.Cross(right)).Normalize();
        wt.quateRotation_ = Quaternion::FromMatrix(MakeRotateMatrix(right, up, look));
    }

    wt.UpdateMatrix();
    pOwner_->ApplyToViewProjection();
    return true;
}

void CameraSkillCutscene::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Separator();
    ImGui::Text("【必殺技 顔アップ演出】");
    ImGui::Text("演出中: %s", IsSkillCloseUpActive() ? "ON" : "OFF");
    ImGui::DragFloat("顔からの距離", &closeUpDistance_, 0.1f, 1.0f, 30.0f);
    ImGui::DragFloat("顔の高さオフセット", &closeUpFaceHeight_, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("回り込み速度", &closeUpApproachSpeed_, 0.1f, 0.5f, 30.0f);
#endif // USE_IMGUI
}

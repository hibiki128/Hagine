#define NOMINMAX
#include "CameraFinisher.h"
#include <Application/camera/follow/FollowCamera.h>
#include <Application/entity/field/ground/Ground.h>
#include <Easing.h>
#include <frame/Frame.h>
#include <object/base/BaseObject.h>
#include <utility/debug/param/GameParamHub.h>
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Hagine;

namespace {
// ゲームパラメータHub上の出所ラベル
constexpr const char *kParamOwner = "コンボ派生技カメラ";
} // namespace

void CameraFinisher::Init(FollowCamera *pOwner)
{
    pOwner_ = pOwner;

    auto *pHub = GameParamHub::GetInstance();
    pHub->Register(kParamOwner, "位置の追従速度", &followSpeed_, {0.1f, 0.5f, 40.0f});
    pHub->Register(kParamOwner, "注視の追従速度", &lookSpeed_, {0.1f, 0.5f, 40.0f});
    pHub->Register(kParamOwner, "傾きの追従速度", &rollSpeed_, {0.1f, 0.5f, 40.0f});
    pHub->Register(kParamOwner, "カメラ位置の側の追従速度", &sideTrackSpeed_, {0.05f, 0.0f, 10.0f});
    pHub->Register(kParamOwner, "画角:離れに応じて引く割合", &framingRate_, {0.01f, 0.0f, 3.0f});
    pHub->Register(kParamOwner, "画角:引きの上限距離", &framingMaxDistance_, {0.5f, 5.0f, 150.0f});
    pHub->Register(kParamOwner, "横位置:距離", &sideDistance_, {0.1f, 2.0f, 60.0f});
    pHub->Register(kParamOwner, "横位置:高さ", &sideHeight_, {0.1f, -10.0f, 20.0f});
    pHub->Register(kParamOwner, "見上げ:距離", &lowAngleDistance_, {0.1f, 2.0f, 60.0f});
    pHub->Register(kParamOwner, "見上げ:下げ幅", &lowAngleDrop_, {0.1f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "肩越し:後方距離", &shoulderDistance_, {0.1f, 1.0f, 30.0f});
    pHub->Register(kParamOwner, "肩越し:横ずらし", &shoulderSide_, {0.1f, -10.0f, 10.0f});
    pHub->Register(kParamOwner, "肩越し:高さ", &shoulderHeight_, {0.1f, -5.0f, 15.0f});
    pHub->Register(kParamOwner, "旋回:半径", &orbitRadius_, {0.1f, 2.0f, 60.0f});
    pHub->Register(kParamOwner, "旋回:高さ", &orbitHeight_, {0.1f, -10.0f, 25.0f});
    pHub->Register(kParamOwner, "旋回:速度", &orbitSpeed_, {0.01f, -3.0f, 3.0f});
    pHub->Register(kParamOwner, "注視点の高さ", &lookHeightOffset_, {0.1f, -5.0f, 10.0f});
}

CameraFinisher::~CameraFinisher()
{
    // ポインタ失効前にゲームパラメータHubから登録を解除する
    GameParamHub::GetInstance()->Unregister(kParamOwner);
}

void CameraFinisher::Start(BaseObject *pPerformer, BaseObject *pTarget, FinisherCameraStyle style)
{
    if (!pPerformer || !pTarget)
    {
        return;
    }

    pPerformer_ = pPerformer;
    pTarget_ = pTarget;
    style_ = style;
    isActive_ = true;
    currentRollDeg_ = 0.0f;
    targetRollDeg_ = 0.0f;

    const Vector3 cameraPos = pOwner_->GetCameraWorldTransform().translation_;
    const Vector3 midPos = (pPerformer_->GetWorldPosition() + pTarget_->GetWorldPosition()) * 0.5f;

    // カメラを置く側は、直前まで通常追従していたカメラのある側に合わせる。
    // こうすると演出への入りが最短の振りで済み、左右が入れ替わって見えない
    lastAxis_ = GetHorizontalAxis();
    const Vector3 toCamera = cameraPos - midPos;
    Vector3 sideDir = {toCamera.x, 0.0f, toCamera.z};
    if (sideDir.Length() < kEpsilon)
    {
        sideDir = {-lastAxis_.z, 0.0f, lastAxis_.x};
    }
    cameraSideDir_ = sideDir.Normalize();

    // 旋回は現在のカメラ方向から続ける（0度から始めると画面が大きく振られる）
    orbitAngle_ = std::atan2(toCamera.x, toCamera.z);

    SnapToGoal();
}

void CameraFinisher::SetStyle(FinisherCameraStyle style, bool isCut)
{
    if (!isActive_)
    {
        return;
    }

    // 旋回へ切り替えるときは、今カメラがいる方向から回し始める。
    // 横位置をとる側（sideSign_）は技の途中では変えない
    if (style == FinisherCameraStyle::Orbit && style_ != FinisherCameraStyle::Orbit)
    {
        const Vector3 midPos = (pPerformer_->GetWorldPosition() + pTarget_->GetWorldPosition()) * 0.5f;
        const Vector3 toCamera = pOwner_->GetCameraWorldTransform().translation_ - midPos;
        orbitAngle_ = std::atan2(toCamera.x, toCamera.z);
    }

    style_ = style;

    if (isCut)
    {
        SnapToGoal();
    }
}

void CameraFinisher::SnapToGoal()
{
    Vector3 goalPos;
    Vector3 goalLook;
    ComputeGoal(goalPos, goalLook);
    ClampToGround(goalPos);
    pOwner_->GetCameraWorldTransform().translation_ = goalPos;
    currentLookAt_ = goalLook;
}

Vector3 CameraFinisher::GetHorizontalAxis() const
{
    Vector3 axis = pTarget_->GetWorldPosition() - pPerformer_->GetWorldPosition();
    axis.y = 0.0f;
    if (axis.Length() < kEpsilon)
    {
        return lastAxis_;
    }
    return axis.Normalize();
}

Vector3 CameraFinisher::GetCameraSide(const Vector3 &axis) const
{
    // 2人を結ぶ線に直交する2方向のうち、いま居る側を選ぶ。
    // 蹴り返しで使用者と相手が入れ替わると axis は180度反転するため、
    // 単純に直交ベクトルを取るだけだとカメラが毎回反対側へ飛んでしまう
    Vector3 side = {-axis.z, 0.0f, axis.x};
    if (side.x * cameraSideDir_.x + side.z * cameraSideDir_.z < 0.0f)
    {
        side = {-side.x, 0.0f, -side.z};
    }
    return side;
}

float CameraFinisher::GetFramingDistance(float baseDistance, float framingRate) const
{
    // 2人の3次元的な離れ具合をそのまま距離へ足す。
    // 吹き飛ばしや上空への回り込みで一気に離れても、両者が画面から外れない
    const float separation = (pTarget_->GetWorldPosition() - pPerformer_->GetWorldPosition()).Length();
    const float distance = baseDistance + separation * framingRate;
    return (std::min)(distance, framingMaxDistance_);
}

void CameraFinisher::ComputeGoal(Vector3 &outPosition, Vector3 &outLookAt) const
{
    const Vector3 performerPos = pPerformer_->GetWorldPosition();
    const Vector3 targetPos = pTarget_->GetWorldPosition();
    const Vector3 midPos = (performerPos + targetPos) * 0.5f;

    const Vector3 axis = GetHorizontalAxis(); // 使用者→相手（水平）
    const Vector3 side = GetCameraSide(axis); // カメラを置く側（水平）

    // 注視点は基本的に2人の中点。どのアングルでも両者が画面に入るようにする
    outLookAt = midPos + Vector3(0.0f, lookHeightOffset_, 0.0f);

    switch (style_)
    {
    case FinisherCameraStyle::SideProfile:
    {
        // 2人を結ぶ線の真横。両者が画面に収まるので打ち合いが読みやすい（基本の絵）
        const float distance = GetFramingDistance(sideDistance_, framingRate_);
        outPosition = midPos + side * distance + Vector3(0.0f, sideHeight_, 0.0f);
        break;
    }

    case FinisherCameraStyle::LowAngleUp:
    {
        // 低い位置から見上げる。打ち上がった相手を大きく見せる。
        // 横位置から少しだけ振った斜め横なので、真横からの絵と地続きに見える
        const float distance = GetFramingDistance(lowAngleDistance_, framingRate_);
        outPosition = midPos + side * (distance * 0.8f) - axis * (distance * 0.45f);
        outPosition.y = (std::min)(performerPos.y, targetPos.y) - lowAngleDrop_;
        break;
    }

    case FinisherCameraStyle::OverShoulder:
    {
        // 使用者の肩越しに相手を捉える。連射のような主観的な場面向き。
        // 相手が離れるほど後ろへ下がって、相手が豆粒にならないようにする
        const float distance = GetFramingDistance(shoulderDistance_, framingRate_ * 0.4f);
        outPosition = performerPos - axis * distance +
                      side * shoulderSide_ + Vector3(0.0f, shoulderHeight_, 0.0f);
        outLookAt = targetPos + Vector3(0.0f, lookHeightOffset_, 0.0f);
        break;
    }

    case FinisherCameraStyle::Orbit:
    default:
    {
        // 中点まわりをゆっくり回り込む。長めのフェーズに動きを足すだけで、
        // 速く回すと画面が回転しているようにしか見えないので控えめにする
        const float radius = GetFramingDistance(orbitRadius_, framingRate_);
        const Vector3 orbitDir = {std::sin(orbitAngle_), 0.0f, std::cos(orbitAngle_)};
        outPosition = midPos + orbitDir * radius + Vector3(0.0f, orbitHeight_, 0.0f);
        break;
    }
    }
}

void CameraFinisher::ClampToGround(Vector3 &position) const
{
    const float floorY = Ground::GetSurfaceY(position.x, position.z) + kGroundClearance;
    if (position.y < floorY)
    {
        position.y = floorY;
    }
}

bool CameraFinisher::UpdateFinisherCamera()
{
    if (!isActive_)
    {
        return false;
    }
    // 対象が失われたら通常追従へ戻す（技側のキャンセルが間に合わない場合の保険）
    if (!pPerformer_ || !pTarget_)
    {
        isActive_ = false;
        return false;
    }

    const float deltaTime = Frame::DeltaTime();

    lastAxis_ = GetHorizontalAxis();
    orbitAngle_ += orbitSpeed_ * deltaTime;

    // カメラを置く側を、戦っている向きの変化にゆっくり追従させる。
    // 即座に合わせると蹴り返しのたびに線を跨いでしまうので、意図的に鈍くする
    const Vector3 side = GetCameraSide(lastAxis_);
    cameraSideDir_ += (side - cameraSideDir_) * (std::min)(sideTrackSpeed_ * deltaTime, kEasingProgressMax);
    if (cameraSideDir_.Length() > kEpsilon)
    {
        cameraSideDir_ = cameraSideDir_.Normalize();
    }

    Vector3 goalPos;
    Vector3 goalLook;
    ComputeGoal(goalPos, goalLook);
    ClampToGround(goalPos);

    WorldTransform &wt = pOwner_->GetCameraWorldTransform();

    // 位置・注視点をそれぞれ指数補間で追従させる（注視の方を速くして被写体を外さない）
    const float posT = (std::min)(followSpeed_ * deltaTime, kEasingProgressMax);
    const float lookT = (std::min)(lookSpeed_ * deltaTime, kEasingProgressMax);
    wt.translation_ += (goalPos - wt.translation_) * posT;
    currentLookAt_ += (goalLook - currentLookAt_) * lookT;

    // 画面の傾き（ダッチアングル）も滑らかに追従させる
    const float rollT = (std::min)(rollSpeed_ * deltaTime, kEasingProgressMax);
    currentRollDeg_ += (targetRollDeg_ - currentRollDeg_) * rollT;

    Vector3 look = currentLookAt_ - wt.translation_;
    if (look.Length() > kEpsilon)
    {
        look = look.Normalize();

        Vector3 right;
        if (std::abs(look.Dot(kWorldUp)) > kParallelThreshold)
        {
            right = kWorldRight;
        }
        else
        {
            right = (kWorldUp.Cross(look)).Normalize();
        }
        Vector3 up = (look.Cross(right)).Normalize();

        // 視線軸まわりに right / up を回して画面を傾ける
        const float roll = currentRollDeg_ * std::numbers::pi_v<float> / 180.0f;
        const float cosRoll = std::cos(roll);
        const float sinRoll = std::sin(roll);
        const Vector3 rolledRight = right * cosRoll + up * sinRoll;
        const Vector3 rolledUp = up * cosRoll - right * sinRoll;

        wt.quaternionRotation_ = Quaternion::FromMatrix(MakeRotateMatrix(rolledRight, rolledUp, look));
    }

    wt.UpdateMatrix();
    pOwner_->ApplyToViewProjection();
    return true;
}

void CameraFinisher::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Separator();
    ImGui::Text("【コンボ派生技カメラ】");
    ImGui::Text("演出中: %s", isActive_ ? "ON" : "OFF");
    ImGui::DragFloat("位置の追従速度##fin", &followSpeed_, 0.1f, 0.5f, 40.0f);
    ImGui::DragFloat("注視の追従速度##fin", &lookSpeed_, 0.1f, 0.5f, 40.0f);
    ImGui::DragFloat("画角:離れに応じて引く割合##fin", &framingRate_, 0.01f, 0.0f, 3.0f);
    ImGui::DragFloat("画角:引きの上限距離##fin", &framingMaxDistance_, 0.5f, 5.0f, 150.0f);
    ImGui::DragFloat("横位置:距離##fin", &sideDistance_, 0.1f, 2.0f, 60.0f);
    ImGui::DragFloat("横位置:高さ##fin", &sideHeight_, 0.1f, -10.0f, 20.0f);
    ImGui::DragFloat("見上げ:距離##fin", &lowAngleDistance_, 0.1f, 2.0f, 60.0f);
    ImGui::DragFloat("見上げ:下げ幅##fin", &lowAngleDrop_, 0.1f, 0.0f, 20.0f);
    ImGui::DragFloat("肩越し:後方距離##fin", &shoulderDistance_, 0.1f, 1.0f, 30.0f);
    ImGui::DragFloat("旋回:半径##fin", &orbitRadius_, 0.1f, 2.0f, 60.0f);
    ImGui::DragFloat("旋回:速度##fin", &orbitSpeed_, 0.01f, -3.0f, 3.0f);
#endif // USE_IMGUI
}

#define NOMINMAX
#include "GroundCrack.h"
#include "Application/entity/field/ground/Ground.h"
#include <collider/type/MeshCollider.h>
#include <object/base/BaseObjectManager.h>
#include <utility/debug/param/GameParamHub.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
#include <string>

using namespace Hagine;

namespace {
/// <summary>
/// 0〜2πの範囲でランダムな角度を返す（地割れの向きをばらつかせる）
/// </summary>
/// <returns>float: ランダムな角度（ラジアン）</returns>
float RandomYaw()
{
    static std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<float> distribution(0.0f, 2.0f * std::numbers::pi_v<float>);
    return distribution(generator);
}
} // namespace

void GroundCrack::Init()
{
    decals_.resize(kDecalPoolSize);

    for (int i = 0; i < kDecalPoolSize; ++i)
    {
        auto object = std::make_unique<BaseObject>();
        object->Init("groundCrack_" + std::to_string(i));
        object->CreatePrimitiveModel(PrimitiveType::Plane);
        object->SetTexture(kTexturePath);

        // 地面に貼るデカールなので陰影は付けない。
        // ライティングを切ることでディファードのG-Bufferにも載らなくなり、
        // 半透明のまま前方描画側で正しくアルファ合成される
        object->GetLighting() = false;
        object->SetBlendMode(BlendMode::Normal);

        // 画像は「ひび割れが不透明の黒／背景が完全透明」なので、色は白のまま使う
        object->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
        object->SetAlpha(0.0f);

        // 演出用の使い捨てオブジェクトなので、シーンの保存対象には含めない
        object->SetShouldSave(false);

        // 未使用の間はスケール0にして、マネージャから描かれても何も出ないようにする
        object->GetLocalScale() = {0.0f, 0.0f, 0.0f};
        object->UpdateWorldTransformHierarchy();

        // 描画・行列更新はマネージャの通常経路に任せる（所有はこのクラスのまま）。
        // 登録解除は BaseObject のデストラクタが自動で行う
        BaseObjectManager::GetInstance()->RegisterExternal(object.get());

        decals_[i].pObject = std::move(object);
    }
}

void GroundCrack::HideDecal(Decal &decal)
{
    decal.isActive = false;
    decal.pObject->GetLocalScale() = {0.0f, 0.0f, 0.0f};
    decal.pObject->SetAlpha(0.0f);
    decal.pObject->UpdateWorldTransformHierarchy();
}

GroundCrack::Decal *GroundCrack::AcquireDecal()
{
    // 空いているものを優先して使う
    for (Decal &decal : decals_)
    {
        if (!decal.isActive)
        {
            return &decal;
        }
    }

    // 全部出ているときは一番古くから出ているものを使い回す
    Decal *pOldest = &decals_[0];
    for (Decal &decal : decals_)
    {
        if (decal.timer > pOldest->timer)
        {
            pOldest = &decal;
        }
    }
    return pOldest;
}

void GroundCrack::Spawn(const Vector3 &position)
{
    if (decals_.empty())
    {
        return;
    }

    // 地表の高さと傾きを調べる。坂に貼っても浮かない・埋まらないようにする
    Vector3 surfaceNormal = kWorldUp;
    float surfaceY = Ground::GetSurfaceY(position.x, position.z);

    if (MeshCollider *pTerrain = Ground::GetTerrainCollider())
    {
        const Vector3 rayOrigin = {position.x, position.y + kRaycastStartHeight, position.z};
        float hitDistance = 0.0f;
        Vector3 hitNormal;
        if (pTerrain->Raycast(rayOrigin, {0.0f, -1.0f, 0.0f}, kRaycastDistance, hitDistance, hitNormal))
        {
            surfaceY = rayOrigin.y - hitDistance;
            if (hitNormal.Length() > kEpsilon)
            {
                surfaceNormal = hitNormal.Normalize();
                // 地形の裏面を拾った場合に備えて上向きへ揃える
                if (surfaceNormal.Dot(kWorldUp) < 0.0f)
                {
                    surfaceNormal = {-surfaceNormal.x, -surfaceNormal.y, -surfaceNormal.z};
                }
            }
        }
    }

    Decal *pDecal = AcquireDecal();
    pDecal->timer = 0.0f;
    pDecal->isActive = true;

    BaseObject *pObject = pDecal->pObject.get();
    pObject->GetLocalPosition() = {position.x, surfaceY + heightOffset_, position.z};

    // ─── 板ポリの「表」がどちらを向くか ───
    // Plane プリミティブの頂点法線はローカル +Z を向いているが、カリングの表裏を決めるのは
    // 法線ではなく頂点の巻き順。インデックスは 0,2,1 / 1,2,3 で、+Z 側から見ると反時計回りになる。
    // このプロジェクトのラスタライザは CullMode=BACK かつ FrontCounterClockwise 未設定（=FALSE、
    // つまり画面上で時計回りが表）なので、実際に見えるのは「ローカル -Z 側」になる。
    // したがって -Z が地表法線を向くように、ローカル +Z は地面へ潜る向きに合わせる。
    // 面内の向き（ヨー）は毎回ランダムにして、同じ絵が並ばないようにする
    const float yaw = RandomYaw();
    Vector3 forward = {-surfaceNormal.x, -surfaceNormal.y, -surfaceNormal.z};
    Vector3 right = {std::cos(yaw), 0.0f, std::sin(yaw)};
    right = right - forward * right.Dot(forward); // 法線と直交させる
    if (right.Length() < kEpsilon)
    {
        right = kWorldRight;
    }
    right = right.Normalize();
    const Vector3 up = (forward.Cross(right)).Normalize();
    pObject->GetLocalRotation() = Quaternion::FromMatrix(MakeRotateMatrix(right, up, forward));

    const float startScale = decalScale_ * startScaleRate_;
    pObject->GetLocalScale() = {startScale, startScale, startScale};
    pObject->SetAlpha(1.0f);
    pObject->UpdateWorldTransformHierarchy();
}

void GroundCrack::RequestOnLanding(BaseObject *pTarget)
{
    if (!pTarget)
    {
        return;
    }
    pWatchTarget_ = pTarget;
    watchTimer_ = 0.0f;
}

void GroundCrack::UpdateWatch(float deltaTime)
{
    if (!pWatchTarget_)
    {
        return;
    }

    watchTimer_ += deltaTime;

    const Vector3 targetPosition = pWatchTarget_->GetWorldPosition();
    const float standingY = Ground::GetStandingY(targetPosition.x, targetPosition.z);

    if (targetPosition.y <= standingY + landingThreshold_)
    {
        // 叩きつけられた相手が地面に到達した。足元に地割れを出す
        Spawn({targetPosition.x, standingY, targetPosition.z});
        pWatchTarget_ = nullptr;
        return;
    }

    // 空中で止まった・吹き飛ばしが途切れた場合に監視が残り続けないようにする
    if (watchTimer_ >= watchTimeout_)
    {
        pWatchTarget_ = nullptr;
    }
}

void GroundCrack::Update(float deltaTime)
{
    UpdateWatch(deltaTime);

    for (Decal &decal : decals_)
    {
        if (!decal.isActive)
        {
            continue;
        }

        decal.timer += deltaTime;
        if (decal.timer >= lifeTime_)
        {
            HideDecal(decal);
            continue;
        }

        BaseObject *pObject = decal.pObject.get();

        // 出現直後だけ一気に広がって、衝撃で割れたように見せる
        float scaleRate = 1.0f;
        if (expandDuration_ > kEpsilon && decal.timer < expandDuration_)
        {
            const float t = decal.timer / expandDuration_;
            scaleRate = startScaleRate_ + (1.0f - startScaleRate_) * (t * t * (3.0f - 2.0f * t));
        }
        const float scale = decalScale_ * scaleRate;
        pObject->GetLocalScale() = {scale, scale, scale};

        // 寿命の終わり側 fadeDuration_ 秒をかけて徐々に透明にする
        float alpha = 1.0f;
        const float remain = lifeTime_ - decal.timer;
        if (fadeDuration_ > kEpsilon && remain < fadeDuration_)
        {
            alpha = std::clamp(remain / fadeDuration_, 0.0f, 1.0f);
        }
        pObject->SetAlpha(alpha);

        pObject->UpdateWorldTransformHierarchy();
    }
}

void GroundCrack::DrawImGui(const Vector3 &testPosition)
{
#ifdef USE_IMGUI
    if (!ImGui::CollapsingHeader("地割れ演出"))
    {
        return;
    }

    ImGui::TextDisabled("叩きつけた相手が着地した瞬間に発生します");

    if (ImGui::Button("テスト発生（この位置に出す）", ImVec2(260.0f, 0.0f)))
    {
        Spawn(testPosition);
    }

    ImGui::Text("着地監視: %s (経過 %.2f 秒)",
                pWatchTarget_ ? pWatchTarget_->GetName().c_str() : "なし", watchTimer_);

    int activeCount = 0;
    for (const Decal &decal : decals_)
    {
        if (decal.isActive)
        {
            ++activeCount;
        }
    }
    ImGui::Text("表示中: %d / %d 枚", activeCount, static_cast<int>(decals_.size()));

    ImGui::Separator();
    for (size_t i = 0; i < decals_.size(); ++i)
    {
        const Decal &decal = decals_[i];
        if (!decal.isActive)
        {
            continue;
        }
        const Vector3 position = decal.pObject->GetLocalPosition();
        ImGui::Text("[%d] 位置 %.1f, %.1f, %.1f / 経過 %.2f 秒 / 大きさ %.2f",
                    static_cast<int>(i), position.x, position.y, position.z,
                    decal.timer, decal.pObject->GetLocalScale().x);
    }
#else
    (void)testPosition;
#endif // USE_IMGUI
}

void GroundCrack::RegisterParams()
{
    auto *pHub = GameParamHub::GetInstance();
    pHub->Register(kParamOwner, "大きさ", &decalScale_, {0.1f, 0.5f, 40.0f});
    pHub->Register(kParamOwner, "消えるまでの時間(秒)", &lifeTime_, {0.05f, 0.1f, 15.0f});
    pHub->Register(kParamOwner, "フェード時間(秒)", &fadeDuration_, {0.05f, 0.0f, 10.0f});
    pHub->Register(kParamOwner, "広がる時間(秒)", &expandDuration_, {0.01f, 0.0f, 2.0f});
    pHub->Register(kParamOwner, "出現時の大きさの割合", &startScaleRate_, {0.01f, 0.0f, 1.0f});
    pHub->Register(kParamOwner, "地表から浮かせる高さ", &heightOffset_, {0.005f, 0.0f, 1.0f});
    pHub->Register(kParamOwner, "着地とみなす高さ", &landingThreshold_, {0.05f, 0.0f, 5.0f});
    pHub->Register(kParamOwner, "着地監視の打ち切り時間(秒)", &watchTimeout_, {0.05f, 0.1f, 10.0f});
}

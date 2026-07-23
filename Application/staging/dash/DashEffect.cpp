#include "DashEffect.h"
#include <particle/gpu/ParticleCSSpawner.h>

using namespace Hagine;

void DashEffect::Init()
{
    // 風切りライン（進行方向と逆へ高速で流れるストリーク）。
    // Spawn したエミッターの更新・描画はエンジンが自動で回すので、
    // ここでは毎フレーム発生位置と速度を与えるだけでよい。
    windEmitter_ = ParticleCSSpawner::GetInstance()->Spawn("dashWind");

    if (windEmitter_)
    {
        // dashWind テンプレートは自動発生 ON なので、Spawn 直後に原点で発生してしまう。
        // ダッシュ中だけ Update で SetAuto(true) にするため、まずは発生を止めておく。
        windEmitter_->SetAuto(false);
        // 生成時のスケールを基準として覚えておく（空中ダッシュ時にこの値へ戻す）
        baseWindScale_ = windEmitter_->GetScale();
    }
}

void DashEffect::Update(const Vector3 &position, const Vector3 &velocity,
                        const Vector3 &forward, bool active, bool grounded)
{
    // 進行方向を求める（十分な速度があれば速度から、低速時は正面方向で代用）
    Vector3 dir = velocity;
    float speed = dir.Length();
    if (speed > kMinMoveSpeed)
    {
        dir = dir / speed;
    }
    else
    {
        float forwardLen = forward.Length();
        dir = (forwardLen > 0.001f) ? forward / forwardLen : lastDir_;
    }
    lastDir_ = dir;

    Vector3 center = position + Vector3(0.0f, kBodyHeightOffset, 0.0f);

    // 空中ダッシュでは発生位置をさらに上げる
    if (!grounded)
    {
        center.y += kAirDashRiseY;
    }

    if (windEmitter_)
    {
        // 少し進行方向の先から発生させ、体の横を通り過ぎるように逆方向へ流す。
        // 発生速度はワールド空間なので、ここで毎フレーム進行方向から計算する
        windEmitter_->SetTranslate(center + dir * kWindForwardOffset);
        Vector3 back = -dir;
        Vector3 spread = {kWindSpread, kWindSpread, kWindSpread};
        windEmitter_->SetMinVelocity(back * kWindSpeedMin - spread);
        windEmitter_->SetMaxVelocity(back * kWindSpeedMax + spread);

        // 地上ダッシュはエミッターを縦に伸ばし、空中ダッシュは基準スケールに戻す
        Vector3 scale = baseWindScale_;
        scale.y = grounded ? kGroundWindScaleY : baseWindScale_.y;
        windEmitter_->SetScale(scale);

        windEmitter_->SetAuto(active);
    }
}

void DashEffect::DrawImGui()
{
    if (windEmitter_)
    {
        windEmitter_->DrawImGui();
    }
}

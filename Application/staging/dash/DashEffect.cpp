#include "DashEffect.h"
#include <particle/gpu/ParticleCSEditor.h>

using namespace Hagine;

void DashEffect::Init()
{
    // 風切りライン（進行方向と逆へ高速で流れるストリーク）
    windEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("dashWind");
    // 通過跡の残光（その場に残ってすっと消える淡い光）
    afterglowEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("dashAfterglow");
}

void DashEffect::Update(const Vector3 &position, const Vector3 &velocity,
                        const Vector3 &forward, bool active)
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

    if (windEmitter_)
    {
        // 少し進行方向の先から発生させ、体の横を通り過ぎるように逆方向へ流す。
        // 発生速度はワールド空間なので、ここで毎フレーム進行方向から計算する
        windEmitter_->SetTranslate(center + dir * kWindForwardOffset);
        Vector3 back = -dir;
        Vector3 spread = {kWindSpread, kWindSpread, kWindSpread};
        windEmitter_->SetMinVelocity(back * kWindSpeedMin - spread);
        windEmitter_->SetMaxVelocity(back * kWindSpeedMax + spread);
        windEmitter_->SetAuto(active);
        windEmitter_->Update(); // emitフラグ残留防止のため毎フレーム呼ぶ
    }

    if (afterglowEmitter_)
    {
        // 残光はほぼ動かず、通り過ぎた軌跡として残るのでエミッターを体に追従させるだけでよい
        afterglowEmitter_->SetTranslate(center);
        afterglowEmitter_->SetAuto(active);
        afterglowEmitter_->Update();
    }
}

void DashEffect::DrawCompute(const ViewProjection &vp)
{
    if (windEmitter_)
    {
        windEmitter_->DrawCompute(vp);
    }
    if (afterglowEmitter_)
    {
        afterglowEmitter_->DrawCompute(vp);
    }
}

void DashEffect::DrawGraphics(const ViewProjection &vp)
{
    if (windEmitter_)
    {
        windEmitter_->DrawGraphics(vp);
    }
    if (afterglowEmitter_)
    {
        afterglowEmitter_->DrawGraphics(vp);
    }
}

void DashEffect::DrawImGui()
{
    if (windEmitter_)
    {
        windEmitter_->DrawImGui();
    }
    if (afterglowEmitter_)
    {
        afterglowEmitter_->DrawImGui();
    }
}

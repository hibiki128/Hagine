#include "FootEffect.h"
#include <algorithm>
#include <cmath>
#include <Frame.h>
#include <particle/gpu/ParticleCSEditor.h>

using namespace Hagine;

void FootEffect::Init()
{
    // 着地の砂煙（接地の瞬間に一度だけ出す）
    landingEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("landing");
    if (landingEmitter_)
    {
        // 自動発生を切り、着地の瞬間に EmitOnce で単発発生させる
        landingEmitter_->SetAuto(false);
    }

    // 走行中の砂煙（走っている間だけ自動発生させる）
    runEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("running");
    if (runEmitter_)
    {
        runEmitter_->SetAuto(false);
    }
}

void FootEffect::Update(const Vector3 &position, const Vector3 &velocity, bool grounded, bool active)
{
    // ─── 進行方向（水平成分）を更新する ───
    Vector3 horizontal = {velocity.x, 0.0f, velocity.z};
    const float horizontalSpeed = horizontal.Length();
    if (horizontalSpeed > kDirEpsilon)
    {
        lastDir_ = horizontal / horizontalSpeed;
    }

    // ─── 着地の瞬間を検出する ───
    // 接地後は velocity.y が 0 に潰されるため、空中にいる間の落下速度を覚えておく
    if (!grounded)
    {
        fallSpeed_ = (velocity.y < 0.0f) ? -velocity.y : 0.0f;
    }
    const bool landed = active && grounded && !wasGrounded_ && fallSpeed_ >= kLandingMinFallSpeed;
    wasGrounded_ = grounded;
    if (grounded)
    {
        fallSpeed_ = 0.0f;
    }

    if (landingEmitter_)
    {
        if (landed)
        {
            landingEmitter_->SetTranslate(position + Vector3(0.0f, kLandingOffsetY, 0.0f));
            landingEmitter_->EmitOnce();
        }
        landingEmitter_->Update(); // emitフラグ残留防止のため毎フレーム呼ぶ
    }

    if (runEmitter_)
    {
        // 一瞬スティックを倒しただけで砂煙が出ないよう、しきい値を超える速度で
        // kRunStartDelay 秒走り続けて初めて発生させる（止まったら即リセット）。
        // 発生間隔のタイマーはエミッター側が自動更新中だけ進むので、
        // ここで発生を止めている間はチョン押しを何度繰り返しても溜まらない
        const bool moving = active && grounded && horizontalSpeed > kRunMinSpeed;
        runTimer_ = moving ? runTimer_ + Frame::DeltaTime() : 0.0f;

        const bool running = moving && runTimer_ >= kRunStartDelay;
        if (running)
        {
            // 足元の少し後ろから発生させ、進行方向と逆へ砂煙を流す。
            // 発生速度はワールド空間なので、ここで毎フレーム進行方向から計算する
            const Vector3 back = -lastDir_;
            runEmitter_->SetTranslate(position - lastDir_ * kRunBackOffset + Vector3(0.0f, kRunOffsetY, 0.0f));

            // エミッターへは軸ごとの min/max しか渡せないため、「後方へ kRunSpeedMin〜Max・
            // 左右へ ±kRunSideSpread」という向き付きの範囲を、XZ成分ごとの範囲に分解する。
            // 左右方向は後方ベクトルに直交する (back.z, -back.x) なので、
            // 各軸へ乗る左右のばらつきは |back| の反対成分に比例する
            const float sideX = std::fabs(back.z) * kRunSideSpread;
            const float sideZ = std::fabs(back.x) * kRunSideSpread;
            const float backNearX = back.x * kRunSpeedMin;
            const float backFarX = back.x * kRunSpeedMax;
            const float backNearZ = back.z * kRunSpeedMin;
            const float backFarZ = back.z * kRunSpeedMax;

            runEmitter_->SetMinVelocity({(std::min)(backNearX, backFarX) - sideX,
                                         -kRunUpSpread,
                                         (std::min)(backNearZ, backFarZ) - sideZ});
            runEmitter_->SetMaxVelocity({(std::max)(backNearX, backFarX) + sideX,
                                         kRunUpSpread,
                                         (std::max)(backNearZ, backFarZ) + sideZ});
        }
        runEmitter_->SetAuto(running);
        runEmitter_->Update(); // emitフラグ残留防止のため毎フレーム呼ぶ
    }
}

void FootEffect::DrawCompute(const ViewProjection &vp)
{
    if (landingEmitter_)
    {
        landingEmitter_->DrawCompute(vp);
    }
    if (runEmitter_)
    {
        runEmitter_->DrawCompute(vp);
    }
}

void FootEffect::DrawGraphics(const ViewProjection &vp)
{
    if (landingEmitter_)
    {
        landingEmitter_->DrawGraphics(vp);
    }
    if (runEmitter_)
    {
        runEmitter_->DrawGraphics(vp);
    }
}

void FootEffect::DrawImGui()
{
    if (landingEmitter_)
    {
        landingEmitter_->DrawImGui();
    }
    if (runEmitter_)
    {
        runEmitter_->DrawImGui();
    }
}

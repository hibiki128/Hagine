#include "FootEffect.h"
#include <algorithm>
#include <cmath>
#include <Frame.h>
#include <particle/gpu/ParticleCSSpawner.h>

using namespace Hagine;

void FootEffect::Init()
{
    // 着地の砂煙（接地の瞬間に一度だけ出す）。
    // Spawn したエミッターの更新・描画はエンジンが自動で回す。
    landingEmitter_ = ParticleCSSpawner::GetInstance()->Spawn("landing");
    if (landingEmitter_)
    {
        // 自動発生を切り、着地の瞬間に EmitOnce で単発発生させる
        landingEmitter_->SetAuto(false);
    }

    // 走行中の砂煙（走っている間だけ自動発生させる）
    runEmitter_ = ParticleCSSpawner::GetInstance()->Spawn("running");
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
    }

    if (runEmitter_)
    {
        // チョン押しで砂煙が出ないよう、kRunStartDelay 秒走り続けて初めて発生させる
        const bool moving = active && grounded && horizontalSpeed > kRunMinSpeed;
        runTimer_ = moving ? runTimer_ + Frame::DeltaTime() : 0.0f;

        const bool running = moving && runTimer_ >= kRunStartDelay;
        if (running)
        {
            // 足元の少し後ろから発生させ、進行方向と逆へ砂煙を流す。
            // 発生速度はワールド空間なので、ここで毎フレーム進行方向から計算する
            const Vector3 back = -lastDir_;
            runEmitter_->SetTranslate(position - lastDir_ * kRunBackOffset + Vector3(0.0f, kRunOffsetY, 0.0f));

            // エミッターへは軸ごとの min/max しか渡せないので、「後方へ〜・左右へ±」という
            // 向き付きの範囲をXZ成分ごとに分解する（左右は後方ベクトルに直交する成分）
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

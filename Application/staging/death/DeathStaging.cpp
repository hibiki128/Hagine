#include "DeathStaging.h"
#include "particle/gpu/ParticleCSSpawner.h"
#include <Frame.h>
#include <numbers>

using namespace Hagine;
DeathStaging::DeathStaging()
{
    // die.obj（死亡ポーズメッシュ）の表面から発生するエミッター。
    // Spawn したエミッターの更新・描画はエンジンが自動で回す。
    deathParticle_ = ParticleCSSpawner::GetInstance()->Spawn("die");
    if (deathParticle_)
    {
        // die テンプレートは自動発生 ON なので、Spawn 直後にそのまま原点で発生してしまう。
        // 死亡演出が始まる（Update で SetAuto(true) される）まで発生を止めておく。
        deathParticle_->SetAuto(false);
    }
    isStart_ = false;
}

void DeathStaging::Initialize(
    const Vector3 &position, const Quaternion &rotation,
    const Vector3 &scale, const Vector4 &color)
{
    position_ = position;
    rotation_ = rotation;
    scale_ = scale;
    color_ = color;
}

void DeathStaging::Update()
{
    time_ += Frame::DeltaTime();

    if (!deathParticle_)
    {
        return;
    }

    // die.obj は Blender の OBJ 出力既定（前方=-Z）のため、glTF（前方=+Z）から再生される
    // 死亡アニメーションの最終ポーズに対して Y軸180度回転した状態で出力されている。
    // ここでメッシュローカルの補正を掛け、倒れたモデルとパーティクルの向きを一致させる
    static const Quaternion kDieObjYawFix =
        Quaternion::FromAxisAngle({0.0f, 1.0f, 0.0f}, std::numbers::pi_v<float>);

    // 一定時間内は発生源メッシュを死亡ポーズへ重ね、色を体色に合わせて発生させる
    if (time_ <= kParticleActiveTime)
    {
        deathParticle_->SetTranslate(position_);
        deathParticle_->SetRotation(rotation_ * kDieObjYawFix);
        deathParticle_->SetScale(scale_);
        deathParticle_->SetStartColor(color_);
        deathParticle_->SetEndColor({color_.x, color_.y, color_.z, kAlphaZero});
        deathParticle_->SetAuto(true);
    }
    else
    {
        // 時間経過後はパーティクルの自動発生を停止
        deathParticle_->SetAuto(false);
    }

    // 重力の影響を開始
    if (time_ >= kGravityStartTime)
    {
        deathParticle_->SetEnableGravity(true);
        deathParticle_->SetEnableLifeTimeScale(true);
        deathParticle_->SetMaxVelocity({kMaxVelocityX, kMaxVelocityY, kMaxVelocityZ});
        deathParticle_->SetMinVelocity({kMinVelocityX, kMinVelocityY, kMinVelocityZ});

        isStart_ = true;
    }
}

void DeathStaging::imgui()
{
}

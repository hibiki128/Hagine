#include "DeathStaging.h"
#include "Particle/CSParticle/ParticleCSEditor.h"
#include <Frame.h>

using namespace Hagine;
DeathStaging::DeathStaging() {
    deathParticle_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("death");
    deathParticle_R_Arm = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("death_arm");
    deathParticle_L_Arm = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("death_arm");
    isStart_ = false;
}

void DeathStaging::Initialize(
    Vector3 position, Vector4 color,
    Vector3 pos_R_Arm, Vector4 c_R_Arm,
    Vector3 pos_L_Arm, Vector4 c_L_Arm) {
    position_ = position;
    color_ = color;
    position_R_Arm = pos_R_Arm;
    color_R_Arm = c_R_Arm;
    position_L_Arm = pos_L_Arm;
    color_L_Arm = c_L_Arm;
}

void DeathStaging::Update() {
    time_ += Frame::DeltaTime();

    // パーティクルの更新
    deathParticle_->Update();
    deathParticle_L_Arm->Update();
    deathParticle_R_Arm->Update();

    // 一定時間内はパーティクルの発生位置と色を更新
    if (time_ <= kParticleActiveTime) {
        deathParticle_->SetTranslate(position_);
        deathParticle_->SetStartColor(color_);
        deathParticle_->SetEndColor({color_.x, color_.y, color_.z, kAlphaZero});
        deathParticle_->SetAuto(true);

        deathParticle_R_Arm->SetTranslate(position_R_Arm);
        deathParticle_R_Arm->SetStartColor(color_R_Arm);
        deathParticle_R_Arm->SetEndColor({color_R_Arm.x, color_R_Arm.y, color_R_Arm.z, kAlphaZero});
        deathParticle_R_Arm->SetAuto(true);

        deathParticle_L_Arm->SetTranslate(position_L_Arm);
        deathParticle_L_Arm->SetStartColor(color_L_Arm);
        deathParticle_L_Arm->SetEndColor({color_L_Arm.x, color_L_Arm.y, color_L_Arm.z, kAlphaZero});
        deathParticle_L_Arm->SetAuto(true);
    } else {
        // 時間経過後はパーティクルの自動発生を停止
        deathParticle_->SetAuto(false);
        deathParticle_R_Arm->SetAuto(false);
        deathParticle_L_Arm->SetAuto(false);
    }

    // 重力の影響を開始
    if (time_ >= kGravityStartTime) {
        deathParticle_->SetEnableGravity(true);
        deathParticle_->SetEnableLifeTimeScale(true);
        deathParticle_->SetMaxVelocity({kMaxVelocityX, kMaxVelocityY, kMaxVelocityZ});
        deathParticle_->SetMinVelocity({kMinVelocityX, kMinVelocityY, kMinVelocityZ});

        deathParticle_R_Arm->SetEnableGravity(true);
        deathParticle_R_Arm->SetEnableLifeTimeScale(true);
        deathParticle_R_Arm->SetMaxVelocity({kMaxVelocityX, kMaxVelocityY, kMaxVelocityZ});
        deathParticle_R_Arm->SetMinVelocity({kMinVelocityX, kMinVelocityY, kMinVelocityZ});

        deathParticle_L_Arm->SetEnableGravity(true);
        deathParticle_L_Arm->SetEnableLifeTimeScale(true);
        deathParticle_L_Arm->SetMaxVelocity({kMaxVelocityX, kMaxVelocityY, kMaxVelocityZ});
        deathParticle_L_Arm->SetMinVelocity({kMinVelocityX, kMinVelocityY, kMinVelocityZ});

        isStart_ = true;
    }
}

void DeathStaging::Draw(const ViewProjection &vp) {

    deathParticle_->Draw(vp);
    deathParticle_R_Arm->Draw(vp);
    deathParticle_L_Arm->Draw(vp);
}

void DeathStaging::imgui() {
    /* if (ImGui::CollapsingHeader("右手")) {
         deathParticle_R_Arm->DrawImGui();
     }
     if (ImGui::CollapsingHeader("左手")) {
         deathParticle_L_Arm->DrawImGui();
     }*/
}
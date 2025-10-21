#include "FadeOut.h"
#include "Scene/SceneTransition.h"
#include "SpriteManager.h"
#include <Particle/CSParticle/ParticleCSEditor.h>

void FadeOut::Initialize() {
    SpriteManager::GetInstance()->SetSaveFolder("Transition");
    SpriteManager::GetInstance()->LoadAllSprites();
    fadeOut_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("FadeOut");
    fadeOut_->SetAuto(true);
    timer_ = 0.0f;
    fadeOut_->SetTranslate({0.0f, 40.6f, -79.1f});
    Quaternion rotation = Quaternion::FromEulerAngles({degreesToRadians(-30.0f), 0.0f, 0.0f});
    fadeOut_->SetRotation(rotation);
    SceneTransition::GetInstance()->SetUseTransition(true);
}

void FadeOut::Update() {
    fadeOut_->Update();
    timer_ += 1.0f / 60.0f; // フレーム時間を加算（60FPS想定）
}

void FadeOut::Draw(const ViewProjection &vp) {
    if (timer_ <= 0.5f) {
        SpriteManager::GetInstance()->DrawAll();
    } else {
        fadeOut_->SetEnableGravity(true);
    }
    if (timer_ >= 0.6f) {
        fadeOut_->SetAuto(false);
    }
    fadeOut_->Draw(vp);
    if (timer_ >= 2.0f) {
        isFinish_ = true;
    }
}

void FadeOut::Finalize() {
    ParticleCSEmitter::ClearNameCounter("FadeOut");
}

void FadeOut::ImGui() {
    fadeOut_->DrawImGui();
}
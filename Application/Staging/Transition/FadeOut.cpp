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
    fadeOut_->SetTranslate({kPositionX, kPositionY, kPositionZ});
    Quaternion rotation = Quaternion::FromEulerAngles({degreesToRadians(kRotationX), 0.0f, 0.0f});
    fadeOut_->SetRotation(rotation);
    SceneTransition::GetInstance()->SetUseTransition(true);
}

void FadeOut::Update() {
    fadeOut_->Update();
    timer_ += kDeltaTime;
}

void FadeOut::Draw(const ViewProjection &vp) {
    if (timer_ <= kSpriteDrawTime) {
        SpriteManager::GetInstance()->GetSprite("transition")->sprite->SetAlpha(1.0f);
    } else {
        SpriteManager::GetInstance()->GetSprite("transition")->sprite->SetAlpha(0.0f);
        fadeOut_->SetEnableGravity(true);
    }
    if (timer_ >= kParticleStopTime) {
        fadeOut_->SetAuto(false);
    }
    fadeOut_->Draw(vp);
    if (timer_ >= kFinishTime) {
        isFinish_ = true;
    }
}

void FadeOut::Finalize() {
    ParticleCSEmitter::ClearNameCounter("FadeOut");
}

void FadeOut::ImGui() {
    fadeOut_->DrawImGui();
}
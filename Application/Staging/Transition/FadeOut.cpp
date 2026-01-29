#include "FadeOut.h"
#include "Scene/SceneTransition.h"
#include "SpriteManager.h"
#include <Particle/CSParticle/ParticleCSEditor.h>

using namespace Hagine;
using namespace Math;
using namespace Graphics;
using namespace Camera;

void FadeOut::Initialize() {
    SpriteManager::GetInstance()->SetSaveFolder("Transition");
    SpriteManager::GetInstance()->LoadAllSprites();
    fadeOut_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("FadeOut");
    fadeOut_->SetAuto(true);
    timer_ = 0.0f;
    fadeOut_->SetTranslate({kPositionX, kPositionY, kPositionZ});
    Math::Quaternion rotation = Math::Quaternion::FromEulerAngles({degreesToRadians(kRotationX), 0.0f, 0.0f});
    fadeOut_->SetRotation(rotation);
    Scene::SceneTransition::GetInstance()->SetUseTransition(true);
}

void FadeOut::Update() {
    fadeOut_->Update();
    timer_ += kDeltaTime;
}

void FadeOut::Draw(const Camera::ViewProjection &vp) {
    if (timer_ <= kSpriteDrawTime) {
        SpriteManager::GetInstance()->DrawAll();
    } else {
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
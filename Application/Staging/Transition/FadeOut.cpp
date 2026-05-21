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
    if (SpriteManager::GetInstance()->GetSprite("transition")) {
        // 一定時間内はスプライトを表示
        if (timer_ <= kSpriteDrawTime) {
            SpriteManager::GetInstance()->GetSprite("transition")->sprite->SetAlpha(1.0f);
        } else {
            // 時間経過後はスプライトを非表示にし、パーティクルの重力を有効化
            SpriteManager::GetInstance()->GetSprite("transition")->sprite->SetAlpha(0.0f);
            fadeOut_->SetEnableGravity(true);
        }
    }

    // パーティクルの自動発生を停止
    if (timer_ >= kParticleStopTime) {
        fadeOut_->SetAuto(false);
    }

    // パーティクルの描画
    fadeOut_->Draw(vp);

    // フェードアウト完了判定
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
#include "FadeOut.h"
#include "Scene/SceneTransition.h"
#include "SpriteManager.h"
#include <Particle/CSParticle/ParticleCSEditor.h>

using namespace Hagine;
void FadeOut::Initialize() {
    // スプライトマネージャーの設定と読み込み
    SpriteManager::GetInstance()->SetSaveFolder("Transition");
    SpriteManager::GetInstance()->LoadAllSprites();

    // パーティクルエミッターの生成と初期設定
    fadeOut_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("FadeOut");
    fadeOut_->SetAuto(true);
    timer_ = 0.0f;
    fadeOut_->SetTranslate({kPositionX, kPositionY, kPositionZ});

    // 回転の設定
    Quaternion rotation = Quaternion::FromEulerAngles({degreesToRadians(kRotationX), 0.0f, 0.0f});
    fadeOut_->SetRotation(rotation);

    // シーントランジションの使用を有効化
    SceneTransition::GetInstance()->SetUseTransition(true);
}

void FadeOut::Update() {
    // パーティクルの更新
    fadeOut_->Update();
    // 経過時間を加算
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
    // エミッター名のカウンターをクリア
    ParticleCSEmitter::ClearNameCounter("FadeOut");
}

void FadeOut::ImGui() {
    // デバッグ用のImGui描画
    fadeOut_->DrawImGui();
}
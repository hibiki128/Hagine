#include "TitleScene.h"
#include <Frame.h>

void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
}

void TitleScene::Finalize() {
    BaseScene::Finalize();
}

void TitleScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
}

void TitleScene::Draw() {
    /// -------描画処理開始-------

    BaseObjectManager::GetInstance()->Draw(vp_);

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //-------------------------

    /// -------描画処理終了-------
}

void TitleScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    /// -------描画処理終了-------
}

void TitleScene::AddSceneSetting() {
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void TitleScene::AddObjectSetting() {
}

void TitleScene::AddParticleSetting() {
}

void TitleScene::CameraUpdate() {
    debugCamera_->Update();
}

void TitleScene::ChangeScene() {
}
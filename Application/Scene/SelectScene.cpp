#include "SelectScene.h"

void SelectScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {12.0f, -4.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
}

void SelectScene::Finalize() {
    BaseScene::Finalize();
}

void SelectScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
}

void SelectScene::Draw() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    /// -------描画処理終了-------
}

void SelectScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    /// -------描画処理終了-------
}

void SelectScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void SelectScene::AddObjectSetting() {
}

void SelectScene::AddParticleSetting() {
}

void SelectScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void SelectScene::ChangeScene() {
}
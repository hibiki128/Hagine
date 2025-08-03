#include "ClearScene.h"
#include "Engine/Utility/Scene/SceneManager.h"
void ClearScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {12.0f, -4.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    clearLogo_ = std::make_unique<Sprite>();
    clearLogo_->Initialize("game/Clear.png", clearLogoPosition_, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
    titleButton_ = std::make_unique<Sprite>();
    titleButton_->Initialize("game/titleButton.png", titleButtonPosition_, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
}

void ClearScene::Finalize() {
    BaseScene::Finalize();
}

void ClearScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    clearLogo_->SetPosition(clearLogoPosition_);
    clearLogo_->SetSize(clearLogo_->GetTexSize() * clearLogoSize_);
    titleButton_->SetPosition(titleButtonPosition_);
    titleButton_->SetSize(titleButton_->GetTexSize() * titleButtonSize_);
}

void ClearScene::Draw() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----
    clearLogo_->Draw();
    titleButton_->Draw();
    //------------------------

    /// -------描画処理終了-------
}

void ClearScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    /// -------描画処理終了-------
}

void ClearScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void ClearScene::AddObjectSetting() {
}

void ClearScene::AddParticleSetting() {
}

void ClearScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void ClearScene::ChangeScene() {
    if (input_->TriggerKey(DIK_SPACE)) {
        // シーンを変更
        sceneManager_->NextSceneReservation("TITLE");
    }
}
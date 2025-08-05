#include "GameScene.h"

void GameScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
}

void GameScene::Finalize() {
    BaseScene::Finalize();
}

void GameScene::Update() {

    /* if (!debugCamera_->GetActive()) {
         BaseObjectManager::GetInstance()->Update();
     }*/

    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
}

void GameScene::Draw() {
    /// -------描画処理開始-------

    BaseObjectManager::GetInstance()->Draw(vp_);
    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //-------------------------

    //-----3DObjectの開始-----

    //-----------------------

    //------Particleの描画開始-------

    //-----------------------------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    /// -------描画処理終了-------
}

void GameScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    //-----3DObjectの描画-----

    //-----------------------

    //------Particleの描画開始-------

    //-----------------------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void GameScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void GameScene::AddObjectSetting() {
}

void GameScene::AddParticleSetting() {
}

void GameScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void GameScene::ChangeScene() {
}
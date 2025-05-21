#include "ClearScene.h"
#include "SceneManager.h"

void ClearScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {12.0f, -4.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
}

void ClearScene::Finalize() {
}

void ClearScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
}

void ClearScene::Draw() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //-------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------

    //-----------------------------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void ClearScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------

    //-----------------------------

    /// ----------------------------------

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

#ifdef _DEBUG
    
#endif // _DEBUG
#ifndef _DEBUG
    if (input_->TriggerKey(DIK_SPACE)) {
        sceneManager_->NextSceneReservation("TITLE");
    }
#endif // !_DEBUG
}
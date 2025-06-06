#include "TitleScene.h"
#include "SceneManager.h"
#include <Engine/Frame/Frame.h>

void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    obj_ = std::make_unique<BaseObject>();
    obj_->Init("test");
    obj_->CreateModel("animation/walk.gltf");
    BaseObjectManager::GetInstance()->AddObject(std::move(obj_));

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

    BaseObjectManager::GetInstance()->DrawWireframe(vp_);

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

void TitleScene::DrawForOffScreen() {
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

void TitleScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void TitleScene::AddObjectSetting() {
 
}

void TitleScene::AddParticleSetting() {
}

void TitleScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void TitleScene::ChangeScene() {

#ifdef _DEBUG

#endif // _DEBUG
#ifndef _DEBUG
    if (input_->TriggerKey(DIK_SPACE)) {
        sceneManager_->NextSceneReservation("GAME");
    }
#endif // !_DEBUG
}
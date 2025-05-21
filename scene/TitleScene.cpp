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
}

void TitleScene::Finalize() {
}

void TitleScene::Update() {

    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
}

void TitleScene::Draw() {
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
    if (ImGui::BeginTabBar("イージング設定")) {
        if (ImGui::BeginTabItem("イージング設定")) {
            ImGui::DragFloat("最大時間", &ease_.maxTime, 0.1f);
            ImGui::DragFloat("イージング時間", &ease_.time, 0.1f);
            ImGui::DragFloat("振幅", &ease_.amplitude, 0.1f);
            ImGui::DragFloat("周期", &ease_.period, 0.1f);
            ImGui::EndTabItem();
        };
        ImGui::EndTabBar();
    }
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
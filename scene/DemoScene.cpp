#include "DemoScene.h"
#include "SceneManager.h"

void DemoScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    ptEditor_ = ParticleEditor::GetInstance();
}

void DemoScene::Finalize() {
    BaseScene::Finalize();
}

void DemoScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
}

void DemoScene::Draw() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------
    ptEditor_->DrawAll(vp_);
    //-----------------------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void DemoScene::DrawForOffScreen() {
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

void DemoScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void DemoScene::AddObjectSetting() {
}

void DemoScene::AddParticleSetting() {
    ptEditor_->EditorWindow();
    ptEditor_->DebugAll();
}

void DemoScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void DemoScene::ChangeScene() {
}

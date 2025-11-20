#include "DemoScene.h"
#include"SpriteManager.h"

void DemoScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize("DemoCamera");
    LightGroup::GetInstance()->LoadLightData("DemoLight");

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    ptEditor_ = ParticleEditor::GetInstance();
    ptCSEditor_ = ParticleCSEditor::GetInstance();
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

    SpriteManager::GetInstance()->DrawAll();
    BaseObjectManager::GetInstance()->Draw(vp_);

    ptEditor_->DrawAll(vp_);
    ptCSEditor_->DrawAll(vp_);

    /// ----------------------------------

    /// -------描画処理終了-------
}

void DemoScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    /// -------描画処理終了-------
}

void DemoScene::AddSceneSetting() {
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void DemoScene::AddObjectSetting() {
}

void DemoScene::AddParticleSetting() {
#ifdef USE_IMGUI
    // CPUとGPUパーティクルをタブで分ける
    if (ImGui::BeginTabBar("ParticleSystemTabs")) {
        if (ImGui::BeginTabItem("CPU パーティクル")) {
            ptEditor_->ShowImGuiEditor();
            ptEditor_->DebugAll();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("GPU パーティクル (CS)")) {
            ptCSEditor_->ShowImGuiEditor();
            ptCSEditor_->DebugAll();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
#endif // USE_IMGUI
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

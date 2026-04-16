#include "DemoScene.h"
#include "Engine/Utility/Scene/SceneManager.h"

void DemoScene::Initialize() {
    BaseScene::Initialize();
    vp_.Initialize("DemoCamera");
    lightGroup_->LoadLightData("DemoLight");

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
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

    /// -------描画処理終了-------
}

void DemoScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// -------描画処理終了-------
}

void DemoScene::AddSceneSetting() {
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void DemoScene::AddObjectSetting() {
}

void DemoScene::AddParticleSetting() {
    DrawParticleEditorUI();
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

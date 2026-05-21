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
    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();
}

void DemoScene::Draw() {
    /// ===================================================
    /// 描画処理開始
    /// ===================================================

    // すべてのオブジェクトを描画
    DrawAllObjects();

    /// ===================================================
    /// 描画処理終了
    /// ===================================================
}

void DemoScene::DrawForOffScreen() {
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void DemoScene::AddSceneSetting() {
    // デバッグ表示
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void DemoScene::AddObjectSetting() {
}

void DemoScene::AddParticleSetting() {
    // パーティクルエディタの表示
    DrawParticleEditorUI();
}

void DemoScene::CameraUpdate() {
    // デバッグカメラまたは通常カメラの更新
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void DemoScene::ChangeScene() {
}

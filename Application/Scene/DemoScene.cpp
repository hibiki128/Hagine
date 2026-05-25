#include "DemoScene.h"
#include "Engine/Utility/Scene/SceneManager.h"

void DemoScene::Initialize() {
    /// ===================================================
    /// 初期化
    /// ===================================================
    BaseScene::Initialize();
    vp_.Initialize("DemoCamera");
    lightGroup_->LoadLightData("DemoLight");

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
}

void DemoScene::Finalize() {
    /// ===================================================
    /// 終了処理
    /// ===================================================
    BaseScene::Finalize();
}

void DemoScene::Update() {
    /// ===================================================
    /// 更新処理
    /// ===================================================

    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();
}

void DemoScene::Draw() {
    /// ===================================================
    /// 描画処理
    /// ===================================================

    // すべてのオブジェクトを描画
    DrawAllObjects();
}

void DemoScene::DrawForOffScreen() {
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void DemoScene::AddSceneSetting() {
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void DemoScene::AddObjectSetting() {
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
}

void DemoScene::AddParticleSetting() {
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
    DrawParticleEditorUI();
}

void DemoScene::CameraUpdate() {
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void DemoScene::ChangeScene() {
    /// ===================================================
    /// シーン切り替え
    /// ===================================================
}

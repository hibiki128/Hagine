#include "SelectScene.h"
#include "Engine/Utility/Scene/SceneManager.h"
void SelectScene::Initialize() {
    BaseScene::Initialize();
    vp_.Initialize();
    vp_.translation_ = {12.0f, -4.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
}

void SelectScene::Finalize() {
    BaseScene::Finalize();
}

void SelectScene::Update() {
    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();
}

void SelectScene::Draw() {
    /// ===================================================
    /// 描画処理開始
    /// ===================================================

    /// ===================================================
    /// 描画処理終了
    /// ===================================================
}

void SelectScene::DrawForOffScreen() {
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void SelectScene::AddSceneSetting() {
    // デバッグ表示
    debugCamera_->imgui();
}

void SelectScene::AddObjectSetting() {
}

void SelectScene::AddParticleSetting() {
}

void SelectScene::CameraUpdate() {
    // デバッグカメラまたは通常カメラの更新
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void SelectScene::ChangeScene() {
}
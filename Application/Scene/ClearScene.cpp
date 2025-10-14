#include "ClearScene.h"
#include "Engine/Utility/Scene/SceneManager.h"
void ClearScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {12.0f, -4.0f, -30.0f};

    BaseObjectManager::GetInstance()->LoadAll("ClearScene");

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    fadeOut_ = std::make_unique<FadeOut>();
    fadeOut_->Initialize();
    timer_ = 0.0f;
}

void ClearScene::Finalize() {
    BaseScene::Finalize();
    fadeOut_->Finalize();
}

void ClearScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    fadeOut_->Update();

    timer_ += 1.0f / 60.0f;
}

void ClearScene::Draw() {
    /// -------描画処理開始-------

    fadeOut_->Draw(vp_);
    if (timer_ >= 1.5f) {
        BaseObjectManager::GetInstance()->Draw(vp_);
    }

    /// -------描画処理終了-------
}

void ClearScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// -------描画処理終了-------
}

void ClearScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void ClearScene::AddObjectSetting() {
}

void ClearScene::AddParticleSetting() {
    fadeOut_->ImGui();
}

void ClearScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void ClearScene::ChangeScene() {
    if (input_->TriggerKey(DIK_SPACE) && timer_ >= 1.5f) {
        // シーンを変更
        sceneManager_->NextSceneReservation("TITLE");
    }
}
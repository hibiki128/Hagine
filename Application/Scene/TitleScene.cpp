#include "TitleScene.h"
#include "Engine/Utility/Scene/SceneManager.h"
#include <Frame.h>
void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
    skyBox_ = SkyBox::GetInstance();
    skyBox_->Initialize("game/skybox.dds");

    titleLogo_ = std::make_unique<Sprite>();
    titleLogo_->Initialize("game/titleLogo.png", titleLogoPosition_, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
    startButton_ = std::make_unique<Sprite>();
    startButton_->Initialize("game/startButton.png", startButtonPosition_, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
}

void TitleScene::Finalize() {
    BaseScene::Finalize();
}

void TitleScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    titleLogo_->SetPosition(titleLogoPosition_);
    titleLogo_->SetSize(titleLogo_->GetTexSize() * titleLogoSize_);
    startButton_->SetPosition(startButtonPosition_);
    startButton_->SetSize(startButton_->GetTexSize() * startButtonSize_);
}

void TitleScene::Draw() {
    /// -------描画処理開始-------
    titleLogo_->Draw();
    startButton_->Draw();
    skyBox_->Draw(vp_);

    BaseObjectManager::GetInstance()->Draw(vp_);

    /// -------描画処理終了-------
}

void TitleScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    /// -------描画処理終了-------
}

void TitleScene::AddSceneSetting() {
    debugCamera_->imgui();
}

void TitleScene::AddObjectSetting() {
    if (ImGui::CollapsingHeader("UI")) {
        if (ImGui::TreeNode("タイトル")) {
            ImGui::DragFloat2("位置", &titleLogoPosition_.x, 0.1f);
            ImGui::DragFloat("サイズ", &titleLogoSize_, 0.1f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("スタートボタン")) {
            ImGui::DragFloat2("位置", &startButtonPosition_.x, 0.1f);
            ImGui::DragFloat("サイズ", &startButtonSize_, 0.1f);
            ImGui::TreePop();
        }
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
    if (input_->TriggerKey(DIK_SPACE)) {
        sceneManager_->NextSceneReservation("GAME");
    }
}
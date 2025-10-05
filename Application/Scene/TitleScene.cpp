#include "TitleScene.h"
#include "Engine/Utility/Scene/SceneManager.h"
#include "myMath.h"
#include <Frame.h>
void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    LightGroup::GetInstance()->LoadLightData("TitleScene");
    vp_.eulerRotation_ = {
        degreesToRadians(26.3f),
        degreesToRadians(-122.7f),
        degreesToRadians(0.0f)};
    vp_.Initialize("CurrentCamera");
    BaseObjectManager::GetInstance()->LoadAll("TitleScene");
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
    skyBox_ = SkyBox::GetInstance();
    skyBox_->Initialize("game/skybox.dds");

    titleUI_ = std::make_unique<TitleUI>();
    titleUI_->Initialize();
}

void TitleScene::Finalize() {
    BaseScene::Finalize();
}

void TitleScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
    time_ += Frame::DeltaTime();
    if (time_ >= kMaxTime_) {
        vp_.EaseCameraMove(EasingType::InCubic, "TitleMovedCamera", 1.0f);
    }

    titleUI_->Update();
}

void TitleScene::Draw() {
    /// -------描画処理開始-------
    skyBox_->Draw(vp_);

    BaseObjectManager::GetInstance()->Draw(vp_);

    SpriteManager::GetInstance()->DrawAll();

    titleUI_->Draw(vp_);

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
    vp_.ShowDebugInfo();
}

void TitleScene::AddObjectSetting() {
}

void TitleScene::AddParticleSetting() {
}

void TitleScene::CameraUpdate() {
    debugCamera_->Update();
}

void TitleScene::ChangeScene() {
    if (input_->TriggerKey(DIK_SPACE) && time_ >= 3.5f) {
        sceneManager_->NextSceneReservation("GAME");
    }
}
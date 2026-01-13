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
    firstMove_ = false;
    secondMove_ = false;

    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);
}

void TitleScene::Finalize() {
    BaseScene::Finalize();
}

void TitleScene::Update() {
    gamePad_->Update();
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();
    time_ += Frame::DeltaTime();
    if (time_ >= kMaxTime_ && !firstMove_) {
        vp_.EaseCameraMove(EasingType::InCubic, "TitleMovedCamera", 1.0f);
        firstMove_ = true;
    }
    if (!gamePad_->IsConnected()) {
        if (time_ >= 3.0f && input_->TriggerKey(DIK_SPACE) && !secondMove_ && !vp_.GetIsCameraMove()) {
            vp_.EaseCameraMove(EasingType::InQuint, "EnemyEyeCamera", 1.0f);
            secondMove_ = true;
        }
    } else {
        if (time_ >= 3.0f && gamePad_->IsTrigger(XINPUT_GAMEPAD_A) && !secondMove_ && !vp_.GetIsCameraMove()) {
            vp_.EaseCameraMove(EasingType::InQuint, "EnemyEyeCamera", 1.0f);
            secondMove_ = true;
        }
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
    if (secondMove_ && !vp_.GetIsCameraMove() && titleUI_->GetIsFinish()) {
        SceneTransition::GetInstance()->SetUseTransition(false);
        sceneManager_->NextSceneReservation("GAME");
    }
#ifndef _DEBUG
#endif // !_DEBUG
}
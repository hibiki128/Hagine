#include "TitleScene.h"
#include "Engine/Utility/Scene/SceneManager.h"
#include "myMath.h"
#include <Frame.h>
void TitleScene::Initialize() {
    BaseScene::Initialize();
    lightGroup_->LoadLightData("TitleScene");
    vp_.eulerRotation_ = {
        degreesToRadians(26.3f),
        degreesToRadians(-122.7f),
        degreesToRadians(0.0f)};
    vp_.Initialize("CurrentCamera");
    objectManager_->LoadAll("TitleScene");
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
    // ゲームパッドの更新
    gamePad_->Update();

    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();

    // 経過時間の更新
    time_ += Frame::DeltaTime();

    // 一定時間経過後にカメラを移動
    if (time_ >= kMaxTime_ && !firstMove_) {
        vp_.EaseCameraMove(EasingType::InCubic, "TitleMovedCamera", 1.0f);
        firstMove_ = true;
    }

    // 入力によるシーン進行（カメラ移動）
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

    // UIの更新
    titleUI_->Update();
}

void TitleScene::Draw() {
    /// ===================================================
    /// 描画処理開始
    /// ===================================================

    // スカイボックスの描画
    skyBox_->Draw(vp_);

    // 2Dスプライトの描画
    spriteManager_->DrawAll();

    // 3Dオブジェクトの描画
    objectManager_->Draw(vp_);

    // タイトルUIの描画
    titleUI_->Draw(vp_);

    /// ===================================================
    /// 描画処理終了
    /// ===================================================
}

void TitleScene::DrawForOffScreen() {
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void TitleScene::AddSceneSetting() {
    // デバッグ表示
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void TitleScene::AddObjectSetting() {
}

void TitleScene::AddParticleSetting() {
    // パーティクルエディタの表示
    DrawParticleEditorUI();
}

void TitleScene::CameraUpdate() {
    // デバッグカメラの更新
    debugCamera_->Update();
}

void TitleScene::ChangeScene() {
    // 演出終了後にチュートリアルシーンへ遷移
    if (secondMove_ && !vp_.GetIsCameraMove() && titleUI_->GetIsFinish()) {
        SceneTransition::GetInstance()->SetUseTransition(false);
        sceneManager_->NextSceneReservation("TUTORIAL");
    }
}
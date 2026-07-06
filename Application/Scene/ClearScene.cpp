#include "ClearScene.h"
#include "Utility/Scene/SceneManager.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Frame.h>

REGISTER_SCENE("CLEAR", ClearScene)

using namespace Hagine;
void ClearScene::Initialize() {
    /// ===================================================
    /// 初期化
    /// ===================================================
    BaseScene::Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    /// ===================================================
    /// ロード
    /// ===================================================
    objectManager_->LoadAll("ClearScene");
    lightGroup_->LoadLightData("ClearLight");

    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    debugCamera_ = std::make_unique<DebugCamera>();
    ground_ = std::make_unique<Ground>();
    resultStaging_ = std::make_unique<ResultStaging>();
    resultUI_ = std::make_unique<ResultUI>();
    skyBox_ = SkyBox::GetInstance();
    gamePad_ = std::make_unique<GamePad>();

    /// ===================================================
    /// 初期化
    /// ===================================================
    debugCamera_->Initialize(&vp_);
    ground_->Init("Ground");
    skyBox_->Initialize("game/skybox.dds");
    vp_.Initialize("P_StartCamera");
    resultStaging_->Initialize();
    resultUI_->Initialize();
    ground_->GetLighting() = true;

    skyBox_->Initialize("game/skybox_night.dds");
    gamePad_->Init(0);

    /// ===================================================
    /// セット
    /// ===================================================
    resultUI_->SetClearTime(sceneManager_->GetClearTime());
    resultUI_->SetHP(sceneManager_->GetHP());

    ground_->GetLighting() = true;

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    drawSystem_->Register("ClearScene_3D", DrawLayer::kPreEffect, [this](const ViewProjection &vp) {
        objectManager_->Draw(vp);
        skyBox_->Draw(vp);
        ground_->Draw(vp);
        resultStaging_->Draw(vp);
    });
    drawSystem_->Register("ClearScene_UI", DrawLayer::kPostEffect, [this](const ViewProjection &) {
        resultUI_->Draw();
        spriteManager_->DrawAll();
    });
}

void ClearScene::Finalize() {
    /// ===================================================
    /// 終了処理
    /// ===================================================
    BaseScene::Finalize();
}

void ClearScene::Update() {
    /// ===================================================
    /// 更新処理
    /// ===================================================

    // ゲームパッドの更新
    gamePad_->Update();

    // カメラの更新
    CameraUpdate();

    // カメラ演出終了後の処理
    if (!vp_.GetIsCameraMove() && cameraStart_) {
        resultUI_->SetIsStartEasing(true);
        resultStaging_->SetfireWorkStarted(true);
    }

    // シーン切り替え
    ChangeScene();

    // 地面の更新
    ground_->Update();

    // リザルト演出の更新
    resultStaging_->Update();

    // リザルトUIの更新
    resultUI_->Update();
}

void ClearScene::Draw() {
    // 描画は DrawSystem が管理
}

void ClearScene::DrawForOffScreen() {
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void ClearScene::AddSceneSetting() {
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->imgui();
    vp_.ShowDebugInfo();

    // 演出のデバッグ
    resultStaging_->DrawImGui();
}

void ClearScene::AddObjectSetting() {
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
}

void ClearScene::AddParticleSetting() {
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
}

void ClearScene::CameraUpdate() {
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    currentCameraStartTimer_ += Frame::DeltaTime();
    if (currentCameraStartTimer_ > cameraStartTimer_ && !cameraStart_) {
        // カメラのイージング開始
        vp_.EaseCameraMove(EasingType::InCubic, "P_EndCamera", 1.5f);
        cameraStart_ = true;
        resultStaging_->SetStartEasing(true);
    }

    // デバッグカメラまたは通常カメラの更新
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void ClearScene::ChangeScene() {
    /// ===================================================
    /// シーン切り替え
    /// ===================================================
    if (!gamePad_->IsConnected()) {
        if (resultUI_->IsAllAnimationFinished() && input_->TriggerKey(DIK_SPACE)) {
            sceneManager_->NextSceneReservation("TITLE");
        }
    } else {
        if (resultUI_->IsAllAnimationFinished() && gamePad_->IsTrigger(XINPUT_GAMEPAD_A)) {
            sceneManager_->NextSceneReservation("TITLE");
        }
    }
}

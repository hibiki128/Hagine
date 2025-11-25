#include "ClearScene.h"
#include "Engine/Utility/Scene/SceneManager.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
void ClearScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    /// ===================================================
    /// ロード
    /// ===================================================
    BaseObjectManager::GetInstance()->LoadAll("ClearScene");
    LightGroup::GetInstance()->LoadLightData("GameLight");

    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    debugCamera_ = std::make_unique<DebugCamera>();
    ground_ = std::make_unique<Ground>();
    resultStaging_ = std::make_unique<ResultStaging>();
    resultUI_ = std::make_unique<ResultUI>();
    skyBox_ = SkyBox::GetInstance();

    /// ===================================================
    /// 初期化
    /// ===================================================
    debugCamera_->Initialize(&vp_);
    ground_->Init("Ground");
    skyBox_->Initialize("game/skybox.dds");
    vp_.Initialize("P_StartCamera");
    resultStaging_->Initialize();
    resultUI_->Initialize();
    vp_.EaseCameraMove(EasingType::InCubic, "P_EndCamera", 1.5f);

    /// ===================================================
    /// セット
    /// ===================================================
    resultUI_->SetClearTime(sceneManager_->GetClearTime());
    resultUI_->SetHP(sceneManager_->GetHP());

}

void ClearScene::Finalize() {
    BaseScene::Finalize();
}

void ClearScene::Update() {
    // カメラ更新
    CameraUpdate();

    if (!vp_.GetIsCameraMove()) {
        resultUI_->SetIsStartEasing(true);
    }

    // シーン切り替え
    ChangeScene();

    ground_->Update();

    resultStaging_->Update();

    resultUI_->Update();
}

void ClearScene::Draw() {
    /// -------描画処理開始-------

    BaseObjectManager::GetInstance()->Draw(vp_);

    skyBox_->Draw(vp_);

    ground_->Draw(vp_);

    resultUI_->Draw();

    SpriteManager::GetInstance()->DrawAll();

    /// -------描画処理終了-------
}

void ClearScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// -------描画処理終了-------
}

void ClearScene::AddSceneSetting() {
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void ClearScene::AddObjectSetting() {
}

void ClearScene::AddParticleSetting() {
}

void ClearScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void ClearScene::ChangeScene() {
    // リザルト演出が完全に終了しているかチェック
    if (resultUI_->IsAllAnimationFinished() && input_->TriggerKey(DIK_SPACE)) {
        // シーンを変更
        sceneManager_->NextSceneReservation("TITLE");
    }
}
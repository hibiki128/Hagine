#include "ClearScene.h"
#include "utility/scene/SceneManager.h"
#include "edit/motion/MotionEditor.h"
#include <Frame.h>

REGISTER_SCENE("CLEAR", ClearScene)

using namespace Hagine;
void ClearScene::Initialize()
{
    /// ===================================================
    /// 初期化
    /// ===================================================
    BaseScene::Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    /// ===================================================
    /// ロード
    /// ===================================================
    pObjectManager_->LoadAll("ClearScene");
    pLightGroup_->LoadLightData("ClearLight");

    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    debugCamera_ = std::make_unique<DebugCamera>();
    ground_ = std::make_unique<Ground>();
    resultStaging_ = std::make_unique<ResultStaging>();
    resultUI_ = std::make_unique<ResultUI>();
    pSkyBox_ = SkyBox::GetInstance();
    gamePad_ = std::make_unique<GamePad>();

    /// ===================================================
    /// 初期化
    /// ===================================================
    debugCamera_->Initialize(&vp_);
    ground_->Init("Ground");
    pSkyBox_->Initialize("game/skybox.dds");
    vp_.Initialize("P_StartCamera");
    resultStaging_->Initialize();
    resultUI_->Initialize();
    ground_->GetLighting() = true;

    pSkyBox_->Initialize("game/skybox_night.dds");
    gamePad_->Init(0);

    /// ===================================================
    /// セット
    /// ===================================================
    isGameOver_ = pSceneManager_->GetIsGameOver();

    // cube_1 はゲームクリア時は Winner（JSON既定）、ゲームオーバー時は Loser を表示する。
    // モデルのみ差し替える（コライダー・親子関係は再読込しない）
    if (isGameOver_)
    {
        if (BaseObject *pResultChara = pObjectManager_->GetObjectByName("cube_1"))
        {
            pResultChara->SetModel("animation/Player/Loser.gltf");
        }
    }

    resultUI_->SetClearTime(pSceneManager_->GetClearTime());
    resultUI_->SetHP(pSceneManager_->GetHP());
    resultUI_->SetIsGameOver(isGameOver_);

    ground_->GetLighting() = true;

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    pDrawSystem_->Register("ClearScene_3D", DrawLayer::PreEffect, [this](const ViewProjection &vp) {
        pObjectManager_->Draw(vp);
        pSkyBox_->Draw(vp);
        ground_->Draw(vp);
        resultStaging_->Draw(vp);
    });
    pDrawSystem_->Register("ClearScene_UI", DrawLayer::PostEffect, [this](const ViewProjection &) {
        resultUI_->Draw();
        pSpriteManager_->DrawAll();
    });
}

void ClearScene::Finalize()
{
    /// ===================================================
    /// 終了処理
    /// ===================================================
    BaseScene::Finalize();
}

void ClearScene::Update()
{
    /// ===================================================
    /// 更新処理
    /// ===================================================

    // ゲームパッドの更新
    gamePad_->Update();

    // カメラの更新
    CameraUpdate();

    // カメラ演出終了後の処理
    if (!vp_.GetIsCameraMove() && cameraStart_)
    {
        resultUI_->SetIsStartEasing(true);

        // 花火はクリア時のみ（ゲームオーバー時は上げない）
        if (!isGameOver_)
        {
            resultStaging_->SetfireWorkStarted(true);
        }
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

void ClearScene::Draw()
{
    // 描画は DrawSystem が管理
}

void ClearScene::DrawForOffScreen()
{
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void ClearScene::AddSceneSetting()
{
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->DrawImGui();
    vp_.ShowDebugInfo();

    // 演出のデバッグ
    resultStaging_->DrawImGui();
}

void ClearScene::AddObjectSetting()
{
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
}

void ClearScene::AddParticleSetting()
{
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
}

void ClearScene::CameraUpdate()
{
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    currentCameraStartTimer_ += Frame::DeltaTime();
    if (currentCameraStartTimer_ > cameraStartTimer_ && !cameraStart_)
    {
        // カメラのイージング開始
        vp_.EaseCameraMove(EasingType::InCubic, "P_EndCamera", 1.5f);
        cameraStart_ = true;
        resultStaging_->SetStartEasing(true);
    }

    // デバッグカメラまたは通常カメラの更新
    if (debugCamera_->GetActive())
    {
        debugCamera_->Update();
    }
    else
    {
        vp_.UpdateMatrix();
    }
}

void ClearScene::ChangeScene()
{
    /// ===================================================
    /// シーン切り替え
    /// ===================================================
    if (!gamePad_->IsConnected())
    {
        if (resultUI_->IsAllAnimationFinished() && pInput_->TriggerKey(DIK_SPACE))
        {
            pSceneManager_->NextSceneReservation("TITLE");
        }
    }
    else
    {
        if (resultUI_->IsAllAnimationFinished() && gamePad_->IsTrigger(XINPUT_GAMEPAD_A))
        {
            pSceneManager_->NextSceneReservation("TITLE");
        }
    }
}

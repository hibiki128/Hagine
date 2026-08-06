#include "DemoScene.h"
#include "utility/scene/SceneManager.h"

REGISTER_SCENE("DEMO", DemoScene)

using namespace Hagine;
void DemoScene::Initialize()
{
    /// ===================================================
    /// 初期化
    /// ===================================================
    BaseScene::Initialize();
    camera_->Load("DemoCamera");
    pLightGroup_->LoadLightData("DemoLight");

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize();

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    // GPU パーティクル（ptCSEditor_）の Compute/Graphics は DrawSystem が全体駆動するため
    // シーン側では登録しない（全シーンでプレビュー・確認できるようにするため）。
    pDrawSystem_->Register("DemoScene_All", DrawLayer::PreEffect, [this](const ViewProjection &vp) {
        pSpriteManager_->DrawAll();
        pObjectManager_->Draw(vp);
    });
}

void DemoScene::Finalize()
{
    /// ===================================================
    /// 終了処理
    /// ===================================================
    BaseScene::Finalize();
}

void DemoScene::Update()
{
    /// ===================================================
    /// 更新処理
    /// ===================================================

    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();
}

void DemoScene::Draw()
{
    // 描画は DrawSystem が管理
}

void DemoScene::DrawForOffScreen()
{
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void DemoScene::AddSceneSetting()
{
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->DrawImGui();
    camera_->ShowDebugWindow();
}

void DemoScene::AddObjectSetting()
{
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
}

void DemoScene::AddParticleSetting()
{
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
}

void DemoScene::CameraUpdate()
{
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    // カメラの行列更新は CameraManager がまとめて行うので、ここでは操作だけ渡す
    debugCamera_->Update();
}

void DemoScene::ChangeScene()
{
    /// ===================================================
    /// シーン切り替え
    /// ===================================================
}

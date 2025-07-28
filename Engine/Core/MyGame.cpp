#include "MyGame.h"
#include "Scene/SceneFactory.h"
#include <Frame.h>

void MyGame::Initialize() {
    Framework::Initialize();
    Framework::LoadResource();
    Framework::PlaySounds();
    Framework::RegisterShortcutKey();
    // -----ゲーム固有の処理-----

    // 最初のシーンの生成
    sceneFactory_ = new SceneFactory();
    // シーンマネージャに最初のシーンをセット
    sceneManager_->SetSceneFactory(sceneFactory_);
    sceneManager_->NextSceneReservation("GAME");
    // -----------------------
}

void MyGame::Finalize() {
    // -----ゲーム固有の処理-----

    // -----------------------

    Framework::Finalize();
}

void MyGame::Update() {
    Framework::Update();

    // -----ゲーム固有の処理-----
#ifdef _DEBUG

    imGuiManager_->Begin();
    imGuizmoManager_->BeginFrame();
    imGuizmoManager_->SetViewProjection(sceneManager_->GetBaseScene()->GetViewProjection());
    imGuiManager_->UpdateIni();
    imGuiManager_->SetCurrentScene(sceneManager_->GetBaseScene());
    imGuiManager_->ShowMainMenu();
    if (imGuiManager_->GetIsShowMainUI()) {
        imGuiManager_->ShowDockSpace();
        imGuiManager_->ShowSceneWindow(offscreen_.get(), sceneManager_->GetCurrentSceneName());
    }
    imGuiManager_->ShowMainUI(offscreen_.get());
    baseObjectManager_->DrawImGui();
    imGuiManager_->End();
#endif // _DEBUG

    motionEditor_->Update(Frame::DeltaTime());

    // -----------------------
}

void MyGame::Draw() {
    dxCommon_->PreRenderTexture();
    srvManager_->PreDraw();
    // -----描画開始-----

    // -----シーンごとの処理------

    if (sceneManager_->GetTransitionEnd()) {
        collisionManager_->Draw(*sceneManager_->GetBaseScene()->GetViewProjection());
    }
    sceneManager_->Draw();
#ifdef _DEBUG
    //-----線描画-----
    DrawLine3D::GetInstance()->Draw(*sceneManager_->GetBaseScene()->GetViewProjection());
    //---------------
#endif // _DEBUG

    dxCommon_->PreDraw();

    offscreen_->SetProjection(sceneManager_->GetBaseScene()->GetViewProjection()->matProjection_);

    offscreen_->Draw();
    dxCommon_->TransitionDepthBarrier();
    sceneManager_->DrawTransition();
    sceneManager_->DrawForOffScreen();

    // フレーム統計を更新（ImGui描画前）
    ParticleEditor::GetInstance()->UpdateFrameStats();

#ifdef _DEBUG
    imGuiManager_->Draw();
#endif // _DEBUG
       // ------------------------

    // -----描画終了-----
    dxCommon_->PostDraw();
}
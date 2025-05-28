#include "MyGame.h"
#include "SceneFactory.h"

void MyGame::Initialize() {
    Framework::Initialize();
    Framework::LoadResource();
    Framework::PlaySounds();
    // -----ゲーム固有の処理-----

    // 最初のシーンの生成
    sceneFactory_ = new SceneFactory();
    // シーンマネージャに最初のシーンをセット
    sceneManager_->SetSceneFactory(sceneFactory_);
    sceneManager_->NextSceneReservation("TITLE");
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
    if (input->TriggerKey(DIK_F11)) {
        winApp->ToggleFullScreen();
    }
    if (input->TriggerKey(DIK_F5)) {
        imGuiManager_->GetIsShowMainUI() = !imGuiManager_->GetIsShowMainUI();
    }

#ifdef _DEBUG
    imGuiManager_->Begin();
    imGuizmoManager_->BeginFrame();
    imGuizmoManager_->SetViewProjection(sceneManager_->GetBaseScene()->GetViewProjection());
    imGuiManager_->UpdateIni();
    imGuiManager_->SetCurrentScene(sceneManager_->GetBaseScene());
    imGuiManager_->ShowMainMenu();
    if (imGuiManager_->GetIsShowMainUI()) {
        imGuiManager_->ShowDockSpace();
        imGuiManager_->ShowSceneWindow();
    }
    imGuiManager_->ShowMainUI(offscreen_.get());
    imGuiManager_->End();

#endif // _DEBUG

    // -----------------------
}

void MyGame::Draw() {
    dxCommon->PreRenderTexture();
    srvManager->PreDraw();
    // -----描画開始-----

    // -----シーンごとの処理------

    if (sceneManager_->GetTransitionEnd()) {
        collisionManager_->Draw(*sceneManager_->GetBaseScene()->GetViewProjection());
    }
    sceneManager_->Draw();
    sceneManager_->DrawTransition();
#ifdef _DEBUG
    //-----線描画-----
    DrawLine3D::GetInstance()->Draw(*sceneManager_->GetBaseScene()->GetViewProjection());
    //---------------
#endif // _DEBUG

    dxCommon->PreDraw();

    offscreen_->SetProjection(sceneManager_->GetBaseScene()->GetViewProjection()->matProjection_);

    offscreen_->Draw();

    dxCommon->TransitionDepthBarrier();
    sceneManager_->DrawForOffScreen();
    sceneManager_->DrawTransition();

#ifdef _DEBUG
    imGuiManager_->Draw();
#endif // _DEBUG
       // ------------------------

    // -----描画終了-----
    dxCommon->PostDraw();
}
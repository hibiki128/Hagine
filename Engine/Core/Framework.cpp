#include "Framework.h"
#include "Engine/Frame/Frame.h"
#include "ImGui/ImGuiManager.h"
#include "ResourceLeakChecker/D3DResourceLeakChecker.h"

void Framework::Run() {
    // ゲームの初期化
    Initialize();

    while (true) // ゲームループ
    {
        // 更新
        Update();
        // 終了リクエストが来たら抜ける
        if (IsEndRequest()) {
            break;
        }
        // 描画
        Draw();
    }
    // ゲームの終了
    Finalize();
}

void Framework::Initialize() {

    D3DResourceLeakChecker();

    ///---------WinApp--------
    // WindowsAPIの初期化
    winApp = WinApp::GetInstance();
    winApp->Initialize();
    ///-----------------------

    ///---------DirectXCommon----------
    // DirectXCommonの初期化
    dxCommon = DirectXCommon::GetInstance();
    dxCommon->Initialize(winApp);
    ///--------------------------------

    ///--------SRVManager--------
    // SRVマネージャの初期化
    srvManager = SrvManager::GetInstance();
    srvManager->Initialize();
    ///--------------------------

    ///--------BaseObjectManager--------
    baseObjectManager_ = BaseObjectManager::GetInstance();
    ///---------------------------------

    /// ---------ImGui---------
#ifdef _DEBUG
    imGuiManager_ = ImGuiManager::GetInstance();
    imGuiManager_->Initialize(winApp);
    imGuiManager_->GetIsShowMainUI() = true;
#endif // _DEBUG
       /// -----------------------

        /// ---------ImGuizmo---------
#ifdef _DEBUG
    imGuizmoManager_ = ImGuizmoManager::GetInstance();
#endif // _DEBUG
       /// -----------------------

    // offscreenのSRV作成
    dxCommon->CreateOffscreenSRV();
    // depthのSRV作成
    dxCommon->CreateDepthSRV();

    ///----------Input-----------
    // 入力の初期化
    input = Input::GetInstance();
    input->Init(winApp->GetHInstance(), winApp->GetHwnd());
    ///--------------------------

    ///-----------PipeLineManager-----------
    pipeLineManager_ = PipeLineManager::GetInstance();
    pipeLineManager_->Initialize(dxCommon);
    ///-------------------------------------

    ///-----------TextureManager----------
    textureManager_ = TextureManager::GetInstance();
    textureManager_->Initialize(srvManager);
    ///-----------------------------------

    ///-----------ModelManager------------
    modelManager_ = ModelManager::GetInstance();
    modelManager_->Initialize(srvManager);
    ///----------------------------------

    ///----------PrimitiveModel-----------
    primitiveModel = PrimitiveModel::GetInstance();
    primitiveModel->Initialize();
    ///-----------------------------------

    ///----------SpriteCommon------------
    // スプライト共通部の初期化
    spriteCommon = SpriteCommon::GetInstance();
    spriteCommon->Initialize();
    ///----------------------------------

    ///----------ParticleCommon------------
    particleCommon = ParticleCommon::GetInstance();
    particleCommon->Initialize(dxCommon);
    ///------------------------------------

    ///---------Audio-------------
    audio = Audio::GetInstance();
    audio->Initialize();
    ///---------------------------

    ///-------CollisionManager--------------
    collisionManager_ = std::make_unique<CollisionManager>();
    collisionManager_->Initialize();
    ///-------------------------------------

    ///-------SceneManager--------
    sceneManager_ = SceneManager::GetInstance();
    sceneManager_->Initialize();
    ///---------------------------

    ///-------OffScreen--------
    offscreen_ = std::make_unique<OffScreen>();
    offscreen_->Initialize();
    ///------------------------

    ///-------DrawLine3D-------
    line3d_ = DrawLine3D::GetInstance();
    line3d_->Initialize();
    ///------------------------

    LightGroup::GetInstance()->Initialize();

    ///-------ParticleEditor-------
    particleEditor = ParticleEditor::GetInstance();
    particleEditor->Initialize();
    ///----------------------------

    ///-------ParticleGroupManager-------
    particleGroupManager_ = ParticleGroupManager::GetInstance();
    particleGroupManager_->Initialize();
    ///---------------------------------

    /// 時間の初期化
    Frame::Init();
}

void Framework::Finalize() {
    sceneManager_->Finalize();

    // WindowsAPIの終了処理
    winApp->Finalize();

    ///-------PipeLineManager-------
    pipeLineManager_->Finalize();
    ///-----------------------------

    ///-------TextureManager-------
    textureManager_->Finalize();
    ///-----------------------------

    ///-------ModelCommon-------
    modelManager_->Finalize();
    ///---------------------------

    ///-------PrimitiveModel-------
    primitiveModel->Finalize();
    ///-----------------------------

    ///-------ParticleGroupManager-------
    particleGroupManager_->Finalize();
    ///---------------------------------

#ifdef _DEBUG
    imGuiManager_->Finalize();
#endif // _DEBUG
    baseObjectManager_->Finalize();
    line3d_->Finalize();
    srvManager->Finalize();
    audio->Finalize();
    LightGroup::GetInstance()->Finalize();
    particleEditor->Finalize();
    spriteCommon->Finalize();
    particleCommon->Finalize();
    dxCommon->Finalize();
    delete sceneFactory_;
}

void Framework::Update() {

    /// deltaTimeの更新
    Frame::Update();

    baseObjectManager_->Update();

    sceneManager_->Update();

    collisionManager_->Update();

    LightGroup::GetInstance()->Update(*sceneManager_->GetBaseScene()->GetViewProjection());

    /// -------更新処理開始----------

    // -------Input-------
    // 入力の更新
    input->Update();
    // -------------------

    /// -------更新処理終了----------
    endRequest_ = winApp->ProcessMessage();
}

void Framework::LoadResource() {
}

void Framework::PlaySounds() {
}

void Framework::Draw() {
}

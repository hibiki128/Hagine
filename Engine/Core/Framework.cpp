#include "Framework.h"
#include <Frame.h>

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
    ///---------WinApp--------
    // WindowsAPIの初期化
    winApp_ = WinApp::GetInstance();
    winApp_->Initialize();
    ///-----------------------

    ///---------DirectXCommon----------
    // DirectXCommonの初期化
    dxCommon_ = DirectXCommon::GetInstance();
    dxCommon_->Initialize(winApp_);
    ///--------------------------------

    ///--------SRVManager--------
    // SRVマネージャの初期化
    srvManager_ = SrvManager::GetInstance();
    srvManager_->Initialize();
    ///--------------------------

    ///--------BaseObjectManager--------
    baseObjectManager_ = BaseObjectManager::GetInstance();
    ///---------------------------------

    ///--------SpriteManager--------
    spriteManager_ = SpriteManager::GetInstance();
    ///---------------------------------

    /// ---------ImGuizmo---------
#ifdef _DEBUG
    imGuizmoManager_ = ImGuizmoManager::GetInstance();
#endif // _DEBUG
       /// -----------------------

    /// ---------ImGui---------
#ifdef _DEBUG
    imGuiManager_ = ImGuiManager::GetInstance();
    imGuiManager_->Initialize(winApp_, imGuizmoManager_);
    imGuiManager_->GetIsShowMainUI() = true;
#endif // _DEBUG
       /// -----------------------

    // offscreenのSRV作成
    dxCommon_->CreateOffscreenSRV();
    // depthのSRV作成
    dxCommon_->CreateDepthSRV();

    ///----------Input-----------
    // 入力の初期化
    input_ = Input::GetInstance();
    input_->Init(winApp_->GetHInstance(), winApp_->GetHwnd());
    ///--------------------------

    ///-----------PipeLineManager-----------
    pipeLineManager_ = PipeLineManager::GetInstance();
    pipeLineManager_->Initialize(dxCommon_);
    ///-------------------------------------

    ///-----------PipeLineManager-----------
    computePipeLineManager_ = ComputePipeLineManager::GetInstance();
    computePipeLineManager_->Initialize(dxCommon_);
    ///-------------------------------------

    ///-----------TextureManager----------
    textureManager_ = TextureManager::GetInstance();
    textureManager_->Initialize(srvManager_);
    ///-----------------------------------

    ///-----------ModelCommon-------------
    modelCommon_ = ModelCommon::GetInstance();
    modelCommon_->Initialize();
    ///-----------------------------------

    ///-----------ModelManager------------
    modelManager_ = ModelManager::GetInstance();
    modelManager_->Initialize(srvManager_);
    ///----------------------------------

    ///----------PrimitiveModel-----------
    primitiveModel_ = PrimitiveModel::GetInstance();
    primitiveModel_->Initialize();
    ///-----------------------------------

    ///----------SpriteCommon------------
    // スプライト共通部の初期化
    spriteCommon_ = SpriteCommon::GetInstance();
    spriteCommon_->Initialize();
    ///----------------------------------

    ///----------ParticleCommon------------
    particleCommon_ = ParticleCommon::GetInstance();
    particleCommon_->Initialize(dxCommon_);
    ///------------------------------------

    ///---------Audio-------------
    audio_ = Audio::GetInstance();
    audio_->Initialize();
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

    ///-------SkyBox-------
    skyBox_ = SkyBox::GetInstance();
    skyBox_->Initialize("debug/rostock_laage_airport_4k.dds");
    ///--------------------

    ///--------LightGroup------------
    lightGroup_ = LightGroup::GetInstance();
    lightGroup_->Initialize();
    ///------------------------------

    ///-------ParticleEditor-------
    particleEditor_ = ParticleEditor::GetInstance();
    particleEditor_->Initialize();
    ///----------------------------

    ///-------ParticleGroupManager-------
    particleGroupManager_ = ParticleGroupManager::GetInstance();
    particleGroupManager_->Initialize();
    ///---------------------------------

    ///-------ParticleCSEditor-------
    particleCSEditor_ = ParticleCSEditor::GetInstance();
    particleCSEditor_->Initialize();
    ///----------------------------

    ///-------ParticleCSGroupManager-------
    particleCSGroupManager_ = ParticleCSGroupManager::GetInstance();
    particleCSGroupManager_->Initialize();
    ///---------------------------------

    ///--------ShortcutManager------------
    shortcutManager_ = ShortcutManager::GetInstance();
    shortcutManager_->Initialize(input_);
    ///-----------------------------------

    ///-------AttackManager-------
    motionEditor_ = MotionEditor::GetInstance();
    ///---------------------------

    ///-------SceneTransition-------
    sceneTransition_ = SceneTransition::GetInstance();
    ///-----------------------------

    /// 時間の初期化
    Frame::Init();
}

void Framework::Finalize() {
    baseObjectManager_->Finalize();
    sceneManager_->Finalize();
    sceneTransition_->Finalize();

    // WindowsAPIの終了処理
    winApp_->Finalize();

    ///-------PipeLineManager-------
    pipeLineManager_->Finalize();
    ///-----------------------------

    ///-------ComputePipeLineManager-------
    computePipeLineManager_->Finalize();
    ///-----------------------------

    ///-------TextureManager-------
    textureManager_->Finalize();
    ///-----------------------------

    ///-------ModelCommon-------
    modelManager_->Finalize();
    ///---------------------------

    ///-------PrimitiveModel-------
    primitiveModel_->Finalize();
    ///-----------------------------

    ///-------ParticleGroupManager-------
    particleGroupManager_->Finalize();
    ///---------------------------------

    ///-------ParticleCSGroupManager-------
    particleCSGroupManager_->Finalize();
    ///---------------------------------

#ifdef _DEBUG
    imGuiManager_->Finalize();
    imGuizmoManager_->Finalize();
#endif // _DEBUG
    shortcutManager_->Finalize();
    spriteManager_->Finalize();

    line3d_->Finalize();
    skyBox_->Finalize();
    srvManager_->Finalize();
    audio_->Finalize();
    lightGroup_->Finalize();
    motionEditor_->Finalize();
    LightGroup::GetInstance()->Finalize();
    particleEditor_->Finalize();
    particleCSEditor_->Finalize();
    spriteCommon_->Finalize();
    particleCommon_->Finalize();
    modelCommon_->Finalize();
    dxCommon_->Finalize();
    delete sceneFactory_;
}

void Framework::RegisterShortcutKey() {
    // フルスクリーン
    shortcutManager_->RegisterShortcut("FullScreen", DIK_F11, [this]() {
        winApp_->ToggleFullScreen();
    });
#ifdef _DEBUG
    shortcutManager_->RegisterShortcut("ShowShortcuts", DIK_F1, [this]() {
        imGuiManager_->SetShortcutWindow(true);
    });
    // オブジェクトロード
    shortcutManager_->RegisterShortcut("ObjectLoad", {DIK_LSHIFT, DIK_LCONTROL, DIK_M}, [this]() {
        baseObjectManager_->OpenObjectLoadModal();
    });
    // 終了
    shortcutManager_->RegisterShortcut("End", {DIK_LALT, DIK_F4}, [this]() {
        winApp_->ClosedWindow();
    });
    // シーンセーブ
    shortcutManager_->RegisterShortcut("SceneSave", {DIK_LCONTROL, DIK_LSHIFT, DIK_S}, [this]() {
        baseObjectManager_->OpenSceneSaveModal();
    });
    // シーン読み込み
    shortcutManager_->RegisterShortcut("SceneLoad", {DIK_LCONTROL, DIK_LSHIFT, DIK_L}, [this]() {
        baseObjectManager_->OpenSceneLoadModal();
    });
    // モデル作成
    shortcutManager_->RegisterShortcut("CreateModel", {DIK_LCONTROL, DIK_LSHIFT, DIK_N}, [this]() {
        baseObjectManager_->OpenObjectCreationModal();
    });
    // タイトル
    shortcutManager_->RegisterShortcut("TitleScene", {DIK_LCONTROL, DIK_1}, [this]() {
        sceneManager_->SceneSelection("TITLE");
    });
    // セレクト
    shortcutManager_->RegisterShortcut("SelectScene", {DIK_LCONTROL, DIK_2}, [this]() {
        sceneManager_->SceneSelection("SELECT");
    });
    // ゲーム
    shortcutManager_->RegisterShortcut("GameScene", {DIK_LCONTROL, DIK_3}, [this]() {
        sceneManager_->SceneSelection("GAME");
    });
    // クリア
    shortcutManager_->RegisterShortcut("ClearScene", {DIK_LCONTROL, DIK_4}, [this]() {
        sceneManager_->SceneSelection("CLEAR");
    });
    // デモ
    shortcutManager_->RegisterShortcut("DemoScene", {DIK_LCONTROL, DIK_5}, [this]() {
        sceneManager_->SceneSelection("DEMO");
    });
    // ゲームデバッグ画面切り替え
    shortcutManager_->RegisterShortcut("SwichMode", DIK_F5, [this]() {
        imGuiManager_->GetIsShowMainUI() = !imGuiManager_->GetIsShowMainUI();
    });
    // コピー
    shortcutManager_->RegisterShortcut("Copy", {DIK_LCONTROL, DIK_C}, [this]() {
        imGuizmoManager_->CopySelectedObjects();
    });
    // ペースト
    shortcutManager_->RegisterShortcut("Paste", {DIK_LCONTROL, DIK_V}, [this]() {
        imGuizmoManager_->PasteObjects();
    });
    // デリート
    shortcutManager_->RegisterShortcut("Delete", DIK_DELETE, [this]() {
        imGuizmoManager_->DeleteSelectedObjects();
    });

#endif // _DEBUG
}

void Framework::Update() {

    /// deltaTimeの更新
    Frame::Update();

    sceneManager_->Update();

    baseObjectManager_->Update();

    spriteManager_->UpdateAll(Frame::DeltaTime());

    collisionManager_->Update();

    LightGroup::GetInstance()->Update(*sceneManager_->GetBaseScene()->GetViewProjection());

    input_->Update();

    shortcutManager_->Update();

    endRequest_ = winApp_->ProcessMessage();
}

void Framework::LoadResource() {
}

void Framework::PlaySounds() {
}

void Framework::Draw() {
}

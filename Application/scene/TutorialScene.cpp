#include "TutorialScene.h"
#include <Application/camera/follow/FollowCameraFactory.h>
#include "utility/scene/SceneManager.h"
#include "edit/motion/MotionEditor.h"
#include <Frame.h>
#include <shadow/ShadowMap.h>

REGISTER_SCENE("TUTORIAL", TutorialScene)

using namespace Hagine;
void TutorialScene::Initialize()
{
    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    BaseScene::Initialize();
    pLightGroup_->LoadLightData("GameLight");
    debugCamera_ = std::make_unique<DebugCamera>();
    player_ = std::make_unique<Player>();
    enemy_ = std::make_unique<Enemy>();
    followCamera_ = FollowCameraFactory::Create();
    ground_ = std::make_unique<Ground>();
    pSkyBox_ = SkyBox::GetInstance();
    playerUI_ = std::make_unique<PlayerUI>();
    enemyUI_ = std::make_unique<EnemyUI>();
    fadeOut_ = std::make_unique<FadeOut>();
    gameUI_ = std::make_unique<GameUI>();
    aroundField_ = std::make_unique<AroundField>();
    tutorialSystem_ = std::make_unique<TutorialSystem>();
    tutorialUI_ = std::make_unique<TutorialUI>();
    gamePad_ = std::make_unique<GamePad>();

    /// ===================================================
    /// 初期化
    /// ===================================================
    debugCamera_->Initialize();
    player_->Init("player");
    enemy_->Init("pEnemy");
    ground_->Init("Ground");
    aroundField_->Init("Around_Field");
    followCamera_->Init();
    pSkyBox_->Initialize("game/skybox.dds");
    gamePad_->Init(0);
    gameUI_->Initialize();

    /// ===================================================
    /// セット
    /// ===================================================
    followCamera_->SetPlayer(player_.get());
    player_->SetCamera(followCamera_.get());
    player_->SetEnemy(enemy_.get());
    player_->InitializeShake();
    enemy_->InitializeShake();
    enemy_->SetTarget(player_.get());
    ground_->GetLighting() = false;
    gameUI_->SetIsTutorial(true);

    /// ===================================================
    /// ポインタ共有
    /// ===================================================
    pEnemy_ = enemy_.get();
    pPlayer_ = player_.get();

    playerUI_->Init(pPlayer_);
    enemyUI_->Init(pEnemy_);

    /// ===================================================
    /// オブジェクトマネージャに登録（非所有）
    /// ===================================================
    pObjectManager_->RegisterExternal(player_.get());
    pObjectManager_->RegisterExternal(enemy_.get());

    /// ===================================================
    /// エネミーを非表示にする
    /// ===================================================
    pEnemy_->GetAlive() = false;

    /// ===================================================
    /// チュートリアルシステム初期化
    /// ===================================================
    tutorialSystem_->Initialize(pPlayer_);
    tutorialUI_->Initialize(tutorialSystem_.get());
    fadeOut_->Initialize();

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    // GPU パーティクル（ParticleCSSpawner 所有）の Compute/Graphics はエンジンが自動で回すため、
    // シーン側で Compute フェーズを登録する必要はない。CPU パーティクルは各 DrawParticle 内で描画する。
    pDrawSystem_->Register("TutorialScene_3D", DrawLayer::PreEffect, [this](const ViewProjection &vp) {
        pObjectManager_->Draw(vp);
        pSkyBox_->Draw(vp);
        ground_->Draw(vp);
        aroundField_->Draw(vp);
        pPlayer_->DrawParticle(vp);
        pEnemy_->DrawParticle(vp);
        followCamera_->DrawFrustum();
        pEnemy_->DrawFrustum();
    });
    pDrawSystem_->Register("TutorialScene_UI", DrawLayer::PostEffect, [this](const ViewProjection &) {
        gameUI_->Draw();
        pSpriteManager_->DrawAll();
        if (sceneStarted_)
        {
            playerUI_->Draw();
            enemyUI_->Draw();
            tutorialUI_->Draw();
        }
    });
}

void TutorialScene::Finalize()
{
    /// ===================================================
    /// 終了処理
    /// ===================================================
    tutorialUI_->Finalize();
    tutorialSystem_->Finalize();

    fadeOut_->Finalize();
    aroundField_->Finalize();
    if (pPlayer_->GetIsAlive())
    {
        pSceneManager_->SetHP(pPlayer_->GetHP());
    }
    BaseScene::Finalize();
}

void TutorialScene::Update()
{
    /// ===================================================
    /// 更新処理
    /// ===================================================

    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();

    // 環境オブジェクトの更新
    ground_->Update();
    aroundField_->Update();
    fadeOut_->Update(vp());
    pPlayer_->SetActiveDebugCamera(debugCamera_->GetActive());

    // シャドウマップをプレイヤーに追従
    Vector3 p = pPlayer_->GetWorldPosition();
    ShadowMap::GetInstance()->SetLightTarget({p.x, 0.0f, p.z});

    // 入力の更新
    gamePad_->Update();
    gameUI_->Update();

    // シーン開始遅延（パーティクル遷移の完了を待つ）。
    // 完了後に説明書きのフェードインを開始することで、遷移より先に出るのを防ぐ
    if (!sceneStarted_)
    {
        if (fadeOut_->IsFinish())
        {
            sceneStarted_ = true;
            tutorialUI_->BeginIntroFadeIn();
        }
        return;
    }

    // 遅延経過後の更新
    pPlayer_->SetStart(sceneStarted_);
    playerUI_->Update();
    enemyUI_->Update();

    // ポーズ中でなければチュートリアル進行
    if (!gameUI_->GetIsPause())
    {
        float dt = Frame::DeltaTime();
        tutorialSystem_->Update(dt);
        tutorialUI_->Update(dt);

        // プレイヤーに現在のステップを通知
        pPlayer_->SetTutorialStep(tutorialSystem_->GetCurrentStep());

        // 敵の出現/消滅リクエストを処理
        HandleEnemySpawnRequest();
    }
}

void TutorialScene::Draw()
{
    // 描画は DrawSystem が管理
}

void TutorialScene::DrawForOffScreen()
{
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void TutorialScene::AddSceneSetting()
{
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->DrawImGui();
    followCamera_->DrawImGui();
    camera_->ShowDebugWindow();
    MotionEditor::GetInstance()->DrawImGui();
    tutorialSystem_->DrawImGui();
}

void TutorialScene::AddObjectSetting()
{
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
    pPlayer_->Debug();
    pEnemy_->Debug();
    enemyUI_->Debug();
    tutorialUI_->DrawImGui();
}

void TutorialScene::AddParticleSetting()
{
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
    fadeOut_->DrawImGui();
}

void TutorialScene::CameraUpdate()
{
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    // どのカメラで描くかは CameraManager のアクティブ切り替えで決める
    debugCamera_->Update(); // 有効中は自動でデバッグカメラへ切り替わる
    if (pPlayer_->GetIsAlive() && !debugCamera_->GetActive())
    {
        followCamera_->Update();
        pCameraManager_->SetActive(followCamera_->GetCamera());
    }
}

void TutorialScene::ChangeScene()
{
    /// ===================================================
    /// シーン切り替え
    /// ===================================================
    if (tutorialUI_->IsFinished())
    {
        pSceneManager_->NextSceneReservation("GAME");
    }

    // スキップ操作
    if (gamePad_->IsConnected())
    {
        if (gamePad_->IsTrigger(XINPUT_GAMEPAD_START))
        {
            pSceneManager_->NextSceneReservation("GAME");
        }
    }
    else
    {
        if (pInput_->TriggerKey(DIK_RETURN))
        {
            pSceneManager_->NextSceneReservation("GAME");
        }
    }
}

void TutorialScene::HandleEnemySpawnRequest()
{
    /// ===================================================
    /// エネミー出現管理
    /// ===================================================
    if (tutorialSystem_->ShouldSpawnEnemy())
    {
        pEnemy_->GetAlive() = true;
        tutorialSystem_->ConsumeSpawnRequest();
    }

    if (tutorialSystem_->ShouldDespawnEnemy())
    {
        pEnemy_->GetAlive() = false;
        tutorialSystem_->ConsumeDespawnRequest();
    }
}

#include "GameScene.h"

#include "utility/scene/SceneManager.h"
#include "edit/motion/MotionEditor.h"
#include <Frame.h>
#include <shadow/ShadowMap.h>

REGISTER_SCENE("GAME", GameScene)

using namespace Hagine;
static constexpr const char *kBTFolder = "BehaviorTree";
static constexpr const char *kBTFileName = "EnemyBehavior";

void GameScene::Initialize()
{
    /// ===================================================
    /// 初期化
    /// ===================================================
    BaseScene::Initialize();
    pLightGroup_->LoadLightData("GameLight");
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    debugCamera_ = std::make_unique<DebugCamera>();
    player_ = std::make_unique<Player>();
    enemy_ = std::make_unique<Enemy>();
    followCamera_ = std::make_unique<FollowCamera>();
    startCamera_ = std::make_unique<StartCamera>();
    deathCamera_ = std::make_unique<DeathCamera>();
    ground_ = std::make_unique<Ground>();
    pSkyBox_ = SkyBox::GetInstance();
    playerUI_ = std::make_unique<PlayerUI>();
    enemyUI_ = std::make_unique<EnemyUI>();
    gameUI_ = std::make_unique<GameUI>();
    aroundField_ = std::make_unique<AroundField>();

#ifdef _DEBUG
    behaviorTreeEditor_ = std::make_unique<BehaviorTreeEditor>();
#endif

    /// ===================================================
    /// 初期化
    /// ===================================================
    debugCamera_->Initialize(&vp_);
    player_->Init("player");
    enemy_->Init("enemy");
    ground_->Init("Ground");
    aroundField_->Init("Around_Field");
    followCamera_->Init();
    startCamera_->Init();
    deathCamera_->Init();
    pSkyBox_->Initialize("game/skybox.dds");
    gameUI_->Initialize();

    /// ===================================================
    /// セット
    /// ===================================================
    followCamera_->SetPlayer(player_.get());
    player_->SetCamera(followCamera_.get());
    player_->SetEnemy(enemy_.get());
    player_->SetVp(&vp_);
    enemy_->SetVp(&vp_);
    enemy_->SetTarget(player_.get());

    /// ===================================================
    /// ポインタ共有
    /// ===================================================
    enemy_ptr = enemy_.get();
    player_ptr = player_.get();

    playerUI_->Init(player_ptr);
    enemyUI_->Init(enemy_ptr);

    /// ===================================================
    /// オブジェクトマネージャに登録（非所有）
    /// ===================================================
    pObjectManager_->RegisterExternal(player_.get());
    pObjectManager_->RegisterExternal(enemy_.get());

#ifdef _DEBUG
    behaviorTreeEditor_->SetDebugTargets(enemy_ptr, player_ptr);
#else
    /// ===================================================
    /// BehaviorTreeのロード
    /// ===================================================
    behaviorTreeRoot_ = BehaviorTreeLoader::LoadAndBuild(kBTFolder, kBTFileName);
    if (behaviorTreeRoot_)
    {
        enemy_ptr->SetBehaviorTree(behaviorTreeRoot_);
    }
#endif

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    // GPU パーティクル Compute フェーズ
    pDrawSystem_->Register("GameScene_Particle_Compute", DrawSystem::kGPUParticleCompute, [this](const ViewProjection &vp) {
        player_ptr->DrawParticleCompute(vp);
        enemy_ptr->DrawParticleCompute(vp);
        aroundField_->DrawParticleCompute(vp);
    });
    pDrawSystem_->Register("GameScene_3D", DrawLayer::PreEffect, [this](const ViewProjection &vp) {
        pObjectManager_->Draw(vp);
        pSkyBox_->Draw(vp);
        aroundField_->Draw(vp);
        player_ptr->DrawParticle(vp); // Graphics フェーズのみ実行される
        enemy_ptr->DrawParticle(vp);
        aroundField_->DrawParticle(vp);
        followCamera_->DrawFrustum();
    });
    pDrawSystem_->Register("GameScene_UI", DrawLayer::PostEffect, [this](const ViewProjection &) {
        playerUI_->Draw();
        enemyUI_->Draw();
        gameUI_->Draw();
        pSpriteManager_->DrawAll();
    });
}

void GameScene::Finalize()
{
    /// ===================================================
    /// 終了処理
    /// ===================================================
    aroundField_->Finalize();
    pSceneManager_->SetClearTime(ClearTimer_);
    if (player_ptr->GetIsAlive())
    {
        pSceneManager_->SetHP(player_ptr->GetHP());
    }
    BaseScene::Finalize();
}

void GameScene::Update()
{
    /// ===================================================
    /// 更新処理
    /// ===================================================

    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();

#ifdef _DEBUG
    // ビヘイビアツリーの更新
    {
        auto runtimeRoot = behaviorTreeEditor_->GetRuntimeRoot();
        if (runtimeRoot)
        {
            enemy_ptr->SetBehaviorTree(runtimeRoot);
        }
    }
#endif

    aroundField_->Update();
    playerUI_->Update();
    enemyUI_->Update();

    // シャドウマップをプレイヤーに追従
    Vector3 p = player_ptr->GetWorldPosition();
    ShadowMap::GetInstance()->SetLightTarget({p.x, 0.0f, p.z});
    player_ptr->SetActiveDebugCamera(debugCamera_->GetActive());

#ifdef _DEBUG
    player_ptr->SetStart(true);
    enemy_ptr->SetStart(true);
#else
    // 開始演出待ち
    if (startCamera_->IsComplete())
    {
        player_ptr->SetStart(true);
        enemy_ptr->SetStart(true);
        if (enemy_ptr->GetIsAlive())
        {
            ClearTimer_ += Frame::DeltaTime();
        }
    }
#endif

    // 死亡演出中のモデル非表示
    if (!player_ptr->GetIsAlive() && deathCamera_->IsHalfway())
    {
        enemy_ptr->SetIsModelDraw(false);
    }

    // UIの更新
    gameUI_->Update();
    player_ptr->SetPause(gameUI_->GetIsPause());
    enemy_ptr->SetPause(gameUI_->GetIsPause());
}

void GameScene::Draw()
{
    // 描画は DrawSystem が管理
}

void GameScene::DrawForOffScreen()
{
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void GameScene::AddSceneSetting()
{
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->imgui();
    followCamera_->imgui();
    startCamera_->imgui();
    vp_.ShowDebugInfo();
    MotionEditor::GetInstance()->DrawImGui();
}

void GameScene::AddObjectSetting()
{
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
    player_ptr->Debug();
    enemy_ptr->Debug();
    enemyUI_->Debug();

#ifdef USE_IMGUI
    ImGui::Begin("BehaviorTreeEditor");
    behaviorTreeEditor_->OnImGuiRender();
    ImGui::End();
#endif
}

void GameScene::AddParticleSetting()
{
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
    aroundField_->Debug();
}

void GameScene::CameraUpdate()
{
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    if (player_ptr->GetIsAlive())
    {
        if (debugCamera_->GetActive())
        {
            debugCamera_->Update();
        }
        else
        {
            followCamera_->Update();
#ifndef _DEBUG
            if (!startCamera_->IsComplete())
            {
                startCamera_->Move();
                startCamera_->SetTargetVp(followCamera_->GetViewProjection());
                startCamera_->Update();
                vp_.matWorld_ = startCamera_->GetViewProjection().matWorld_;
                vp_.matView_ = startCamera_->GetViewProjection().matView_;
                vp_.matProjection_ = startCamera_->GetViewProjection().matProjection_;
            }
            else
            {
#endif
                vp_.matWorld_ = followCamera_->GetViewProjection().matWorld_;
                vp_.matView_ = followCamera_->GetViewProjection().matView_;
                vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
#ifndef _DEBUG
            }
#endif
        }
    }
    else
    {
        if (!deathCamera_->IsComplete() && !deathCameraStarted_)
        {
            deathCamera_->StartEasing(
                followCamera_->GetViewProjection(),
                player_ptr->GetWorldPosition());
            deathCameraStarted_ = true;
        }
        deathCamera_->Update();
        vp_.matWorld_ = deathCamera_->GetViewProjection().matWorld_;
        vp_.matView_ = deathCamera_->GetViewProjection().matView_;
        vp_.matProjection_ = deathCamera_->GetViewProjection().matProjection_;
    }
}

void GameScene::ChangeScene()
{
    /// ===================================================
    /// シーン切り替え
    /// ===================================================
    if (!player_ptr->GetIsAlive() && deathCamera_->IsComplete())
    {
        GameOverTimer_ += Frame::DeltaTime();
        player_ptr->SetIsDeathStaging(true);
        if (GameOverTimer_ >= 2.0f && !isGameOver_)
        {
            pSceneManager_->NextSceneReservation("CLEAR");
            isGameOver_ = true;
        }
    }

    if (!enemy_ptr->GetIsAlive())
    {
        pSceneManager_->NextSceneReservation("CLEAR");
    }

    if (gameUI_->GetIsBackTitle())
    {
        pSceneManager_->NextSceneReservation("TITLE");
    }
}

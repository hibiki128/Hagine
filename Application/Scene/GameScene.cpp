#include "GameScene.h"

#include "Engine/Utility/Scene/SceneManager.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Frame.h>

static constexpr const char *kBTFolder = "BehaviorTree";
static constexpr const char *kBTFileName = "EnemyBehavior";

void GameScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    LightGroup::GetInstance()->LoadLightData("GameLight");
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
    skyBox_ = SkyBox::GetInstance();
    playerUI_ = std::make_unique<PlayerUI>();
    enemyUI_ = std::make_unique<EnemyUI>();
    fadeOut_ = std::make_unique<FadeOut>();
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
    skyBox_->Initialize("game/skybox.dds");
    fadeOut_->Initialize();
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
    ground_->GetLighting() = false;

    /// ===================================================
    /// ポインタ共有
    /// ===================================================
    enemy_ptr = enemy_.get();
    player_ptr = player_.get();
    MotionEditor::GetInstance()->Register(player_ptr);
    MotionEditor::GetInstance()->Register(enemy_ptr);

    playerUI_->Init(player_ptr);
    enemyUI_->Init(enemy_ptr);

    /// ===================================================
    /// オブジェクトマネージャに追加
    /// ===================================================
    BaseObjectManager::GetInstance()->AddObject(std::move(player_));
    BaseObjectManager::GetInstance()->AddObject(std::move(enemy_));

#ifdef _DEBUG
    behaviorTreeEditor_->SetDebugTargets(enemy_ptr, player_ptr);
#else
    // ===============================================================
    // Release: JSONからビヘイビアツリーを自動ロードして敵にセット
    // ===============================================================
    m_BehaviorTreeRoot = BehaviorTreeLoader::LoadAndBuild(kBTFolder, kBTFileName);
    if (m_BehaviorTreeRoot) {
        enemy_ptr->SetBehaviorTree(m_BehaviorTreeRoot);
    }
#endif
}

void GameScene::Finalize() {
    BaseScene::Finalize();
    fadeOut_->Finalize();
    sceneManager_->SetClearTime(ClearTimer_);
    if (player_ptr->GetIsAlive()) {
        sceneManager_->SetHP(player_ptr->GetHP());
    }
}

void GameScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

#ifdef _DEBUG
    // Debug: エディタのビルド＆実行ボタンで生成されたランタイムツリーを毎フレーム取得してセット
    {
        auto runtimeRoot = behaviorTreeEditor_->GetRuntimeRoot();
        if (runtimeRoot) {
            enemy_ptr->SetBehaviorTree(runtimeRoot);
        }
    }
#endif
    // Release: Initialize()でセット済みのツリーをそのまま使用
    // enemy_ptr->SetBehaviorTree() は Initialize() で1回だけ呼ぶ

    ground_->Update();
    aroundField_->Update();
    playerUI_->Update();
    enemyUI_->Update();
    fadeOut_->Update();

#ifdef _DEBUG
    player_ptr->SetStart(true);
    enemy_ptr->SetStart(true);
#else
    if (startCamera_->IsComplete()) {
        player_ptr->SetStart(true);
        enemy_ptr->SetStart(true);
        if (enemy_ptr->GetIsAlive()) {
            ClearTimer_ += Frame::DeltaTime();
        }
    }
#endif

    if (!player_ptr->GetIsAlive() && deathCamera_->IsHalfway()) {
        enemy_ptr->SetIsModelDraw(false);
        enemy_ptr->SetDrawShadow(false);
    }

    gameUI_->Update();
    player_ptr->SetPause(gameUI_->GetIsPause());
    enemy_ptr->SetPause(gameUI_->GetIsPause());
}

void GameScene::Draw() {
    playerUI_->Draw();
    enemyUI_->Draw();

    BaseObjectManager::GetInstance()->Draw(vp_);

    skyBox_->Draw(vp_);
    ground_->Draw(vp_);
    aroundField_->Draw(vp_);

    player_ptr->DrawParticle(vp_);
    enemy_ptr->DrawParticle(vp_);

    fadeOut_->Draw(vp_);
    gameUI_->Draw();

    SpriteManager::GetInstance()->DrawAll();
}

void GameScene::DrawForOffScreen() {
    // 必要に応じてオフスクリーン描画
}

void GameScene::AddSceneSetting() {
    debugCamera_->imgui();
    followCamera_->imgui();
    startCamera_->imgui();
    vp_.ShowDebugInfo();
    MotionEditor::GetInstance()->DrawImGui();
}

void GameScene::AddObjectSetting() {

    player_ptr->Debug();
    enemy_ptr->Debug();
    enemyUI_->Debug();

#ifdef USE_IMGUI
    ImGui::Begin("BehaviorTreeEditor");
    behaviorTreeEditor_->OnImGuiRender();
    ImGui::End();
#endif
}

void GameScene::AddParticleSetting() {
    fadeOut_->ImGui();
}

void GameScene::CameraUpdate() {
    if (player_ptr->GetIsAlive()) {
        if (debugCamera_->GetActive()) {
            debugCamera_->Update();
        } else {
            followCamera_->Update();
#ifndef _DEBUG
            if (!startCamera_->IsComplete()) {
                if (fadeOut_->IsFinish()) {
                    startCamera_->Move();
                }
                startCamera_->SetTargetVp(followCamera_->GetViewProjection());
                startCamera_->Update();
                vp_.matWorld_ = startCamera_->GetViewProjection().matWorld_;
                vp_.matView_ = startCamera_->GetViewProjection().matView_;
                vp_.matProjection_ = startCamera_->GetViewProjection().matProjection_;
            } else {
#endif
                vp_.matWorld_ = followCamera_->GetViewProjection().matWorld_;
                vp_.matView_ = followCamera_->GetViewProjection().matView_;
                vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
#ifndef _DEBUG
            }
#endif
        }
    } else {
        if (!deathCamera_->IsComplete() && !deathCameraStarted_) {
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

void GameScene::ChangeScene() {
    if (!player_ptr->GetIsAlive() && deathCamera_->IsComplete()) {
        GameOverTimer_ += Frame::DeltaTime();
        player_ptr->SetIsDeathStaging(true);
        if (GameOverTimer_ >= 2.0f && !isGameOver_) {
            sceneManager_->NextSceneReservation("CLEAR");
            isGameOver_ = true;
        }
    }

    if (!enemy_ptr->GetIsAlive()) {
        sceneManager_->NextSceneReservation("CLEAR");
    }

    if (gameUI_->GetIsBackTitle()) {
        sceneManager_->NextSceneReservation("TITLE");
    }
}
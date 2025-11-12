#include "GameScene.h"

#include "Engine/Utility/Scene/SceneManager.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Frame.h>

void GameScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    LightGroup::GetInstance()->LoadLightData("GameLight");
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

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
    behaviorTreeEditor_ = std::make_unique<BehaviorTreeEditor>();
    fadeOut_ = std::make_unique<FadeOut>();

    /// ===================================================
    /// 初期化
    /// ===================================================
    debugCamera_->Initialize(&vp_);
    player_->Init("player");
    enemy_->Init("enemy");
    ground_->Init("Ground");
    followCamera_->Init();
    startCamera_->Init();
    deathCamera_->Init();
    skyBox_->Initialize("game/skybox.dds");
    fadeOut_->Initialize();

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
    MotionEditor::GetInstance()->Register(player_ptr);
    MotionEditor::GetInstance()->Register(enemy_ptr);

    playerUI_->Init(player_ptr);
    enemyUI_->Init(enemy_ptr);

    /// ===================================================
    /// オブジェクトマネージャに追加
    /// ===================================================
    BaseObjectManager::GetInstance()->AddObject(std::move(player_));
    BaseObjectManager::GetInstance()->AddObject(std::move(enemy_));
}

void GameScene::Finalize() {
    BaseScene::Finalize();
    fadeOut_->Finalize();
}

void GameScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    //  skyDome_->Update();
    ground_->Update();

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
    }
#endif // _DEBUG

    if (!player_ptr->GetIsAlive() && deathCamera_->IsHalfway()) {
        enemy_ptr->SetIsModelDraw(false);
        enemy_ptr->SetDrawShadow(false);
    }

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
}

void GameScene::Draw() {
    /// -------描画処理開始-------

    playerUI_->Draw();
    enemyUI_->Draw();

    BaseObjectManager::GetInstance()->Draw(vp_);

    skyBox_->Draw(vp_);

    ground_->Draw(vp_);

    player_ptr->DrawParticle(vp_);
    enemy_ptr->DrawParticle(vp_);

    fadeOut_->Draw(vp_);

    /// -------描画処理終了-------
}

void GameScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// -------描画処理終了-------
}

void GameScene::AddSceneSetting() {
    debugCamera_->imgui();
    followCamera_->imgui();
    startCamera_->imgui();
    vp_.ShowDebugInfo();
    MotionEditor::GetInstance()->DrawImGui();
}

void GameScene::AddObjectSetting() {
#ifdef USE_IMGUI
    playerUI_->Debug();
    enemyUI_->Debug();
    player_ptr->Debug();
    enemy_ptr->Debug();
    // ビヘイビアツリーエディターの表示
    if (ImGui::CollapsingHeader("BehaviorTree")) {
        enemy_ptr->DrawBehaviorTreeEditor();
    }

    for (auto &bullet : player_ptr->GetBullets()) {
        bullet->ImGui();
    }
#endif // USE_IMGUI
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
#endif // !_DEBUG

                vp_.matWorld_ = followCamera_->GetViewProjection().matWorld_;
                vp_.matView_ = followCamera_->GetViewProjection().matView_;
                vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
#ifndef _DEBUG
            }
#endif // !_DEBUG
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
    /* if () {
         sceneManager_->NextSceneReservation("GAME");
     }*/
}
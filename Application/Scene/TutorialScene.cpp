#include "TutorialScene.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Frame.h>
#include "Engine/Utility/Scene/SceneManager.h"

void TutorialScene::Initialize() {
    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    BaseScene::Initialize();
    debugCamera_ = std::make_unique<DebugCamera>();
    player_ = std::make_unique<Player>();
    enemy_ = std::make_unique<Enemy>();
    followCamera_ = std::make_unique<FollowCamera>();
    ground_ = std::make_unique<Ground>();
    skyBox_ = SkyBox::GetInstance();
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
    debugCamera_->Initialize(&vp_);
    player_->Init("player");
    enemy_->Init("enemy");
    ground_->Init("Ground");
    aroundField_->Init("Around_Field");
    followCamera_->Init();
    skyBox_->Initialize("game/skybox.dds");
    gamePad_->Init(0);
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
    gameUI_->SetIsTutorial(true);

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
    objectManager_->AddObject(std::move(player_));
    objectManager_->AddObject(std::move(enemy_));

    /// ===================================================
    /// チュートリアル開始時はエネミーを非表示にする
    ///   ※ TutorialSystem が必要なタイミングで
    ///      ShouldSpawnEnemy() フラグを立てて出現を通知する
    /// ===================================================
    enemy_ptr->GetAlive() = false; // BaseObject::GetAlive() で非表示

    /// ===================================================
    /// チュートリアルシステム初期化
    /// ===================================================
    tutorialSystem_->Initialize(player_ptr);
    tutorialUI_->Initialize(tutorialSystem_.get());
    fadeOut_->Initialize();
}

// ============================================================
void TutorialScene::Finalize() {
    tutorialUI_->Finalize();
    tutorialSystem_->Finalize();

    fadeOut_->Finalize();
    aroundField_->Finalize();
    if (player_ptr->GetIsAlive()) {
        sceneManager_->SetHP(player_ptr->GetHP());
    }
    BaseScene::Finalize();
}

// ============================================================
void TutorialScene::Update() {
    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();

    // 環境オブジェクトの更新
    ground_->Update();
    aroundField_->Update();
    fadeOut_->Update();
    player_ptr->SetActiveDebugCamera(debugCamera_->GetActive());

    // 入力の更新
    gamePad_->Update();
    gameUI_->Update();

    // ─── シーン開始遅延 ───
    // kStartDelay_ 秒が経過するまでプレイヤー・UIの更新を行わない
    if (!sceneStarted_) {
        startDelayTimer_ += Frame::DeltaTime();
        if (startDelayTimer_ >= kStartDelay_) {
            sceneStarted_ = true;
        }
        return;
    }

    // ─── 遅延経過後の更新 ───
    player_ptr->SetStart(sceneStarted_);
    playerUI_->Update();
    enemyUI_->Update();

    // ポーズ中でなければチュートリアル進行
    if (!gameUI_->GetIsPause()) {
        float dt = Frame::DeltaTime();
        tutorialSystem_->Update(dt);
        tutorialUI_->Update(dt);

        // プレイヤーに現在のステップを通知
        player_ptr->SetTutorialStep(tutorialSystem_->GetCurrentStep());

        // 敵の出現/消滅リクエストを処理
        HandleEnemySpawnRequest();
    }
}

void TutorialScene::Draw() {
    /// ===================================================
    /// 描画処理開始
    /// ===================================================

    // 3Dオブジェクトの描画
    objectManager_->Draw(vp_);

    // 環境オブジェクトの描画
    skyBox_->Draw(vp_);
    ground_->Draw(vp_);
    aroundField_->Draw(vp_);

    // パーティクルの描画
    player_ptr->DrawParticle(vp_);
    enemy_ptr->DrawParticle(vp_);
    aroundField_->DrawParticle(vp_);

    // フェード・UIの描画
    fadeOut_->Draw(vp_);
    gameUI_->Draw();

    // デバッグ用視錐台の描画
    followCamera_->DrawFrustum();
    enemy_ptr->DrawFrustum();

    // 2Dスプライトの描画
    spriteManager_->DrawAll();

    // 遅延終了後にUIを表示
    if (sceneStarted_) {
        playerUI_->Draw();
        enemyUI_->Draw();
        tutorialUI_->Draw();
    }

    /// ===================================================
    /// 描画処理終了
    /// ===================================================
}

void TutorialScene::DrawForOffScreen() {
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void TutorialScene::AddSceneSetting() {
    // デバッグ表示
    debugCamera_->imgui();
    followCamera_->imgui();
    vp_.ShowDebugInfo();
    MotionEditor::GetInstance()->DrawImGui();
    tutorialSystem_->DrawImGui();
}

void TutorialScene::AddObjectSetting() {
    // オブジェクトのデバッグ表示
    player_ptr->Debug();
    enemy_ptr->Debug();
    enemyUI_->Debug();
    tutorialUI_->DrawImGui();
}

void TutorialScene::AddParticleSetting() {
    // フェードのデバッグ表示
    fadeOut_->ImGui();
}

void TutorialScene::CameraUpdate() {
    if (player_ptr->GetIsAlive()) {
        // 通常時のカメラ更新
        if (debugCamera_->GetActive()) {
            debugCamera_->Update();
        } else {
            followCamera_->Update();
            vp_.matWorld_ = followCamera_->GetViewProjection().matWorld_;
            vp_.matView_ = followCamera_->GetViewProjection().matView_;
            vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
        }
    }
}

void TutorialScene::ChangeScene() {
    // チュートリアル終了時にゲームシーンへ
    if (tutorialUI_->IsFinished()) {
        sceneManager_->NextSceneReservation("GAME");
    }

    // スキップ操作（STARTボタンまたはENTERキー）
    if (gamePad_->IsConnected()) {
        if (gamePad_->IsTrigger(XINPUT_GAMEPAD_START)) {
            sceneManager_->NextSceneReservation("GAME");
        }
    } else {
        if (input_->TriggerKey(DIK_RETURN)) {
            sceneManager_->NextSceneReservation("GAME");
        }
    }
}

void TutorialScene::HandleEnemySpawnRequest() {
    // 出現リクエスト
    if (tutorialSystem_->ShouldSpawnEnemy()) {
        enemy_ptr->GetAlive() = true;
        tutorialSystem_->ConsumeSpawnRequest();
    }

    // 消滅リクエスト
    if (tutorialSystem_->ShouldDespawnEnemy()) {
        enemy_ptr->GetAlive() = false;
        tutorialSystem_->ConsumeDespawnRequest();
    }
}
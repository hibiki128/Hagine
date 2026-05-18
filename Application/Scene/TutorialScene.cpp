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
    //fadeOut_ = std::make_unique<FadeOut>();
    gameUI_ = std::make_unique<GameUI>();
    aroundField_ = std::make_unique<AroundField>();
    tutorialSystem_ = std::make_unique<TutorialSystem>();
    tutorialUI_ = std::make_unique<TutorialUI>();

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
   // fadeOut_->Initialize();
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
}

// ============================================================
void TutorialScene::Finalize() {
    tutorialUI_->Finalize();
    tutorialSystem_->Finalize();

  //  fadeOut_->Finalize();
    aroundField_->Finalize();
    if (player_ptr->GetIsAlive()) {
        sceneManager_->SetHP(player_ptr->GetHP());
    }
    BaseScene::Finalize();
}

// ============================================================
void TutorialScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    ground_->Update();
    aroundField_->Update();
    playerUI_->Update();
    enemyUI_->Update();
    //fadeOut_->Update();
    player_ptr->SetActiveDebugCamera(debugCamera_->GetActive());
    player_ptr->SetStart(true);

    gameUI_->Update();
    player_ptr->SetPause(gameUI_->GetIsPause());
    enemy_ptr->SetPause(gameUI_->GetIsPause());

    // ─── チュートリアル更新 ───
    if (!gameUI_->GetIsPause()) {
        float dt = Frame::DeltaTime();
        tutorialSystem_->Update(dt);
        tutorialUI_->Update(dt);

        // 現在のチュートリアルステップをプレイヤーに伝える
        // EnergyCharge ステップ切替時のエネルギーリセット・自動回復抑制はPlayer側で管理する
        player_ptr->SetTutorialStep(tutorialSystem_->GetCurrentStep());

        // エネミーのスポーン/デスポーンリクエスト処理
        HandleEnemySpawnRequest();
    }
}

// ============================================================
void TutorialScene::Draw() {
    playerUI_->Draw();
    enemyUI_->Draw();

    objectManager_->Draw(vp_);

    skyBox_->Draw(vp_);
    ground_->Draw(vp_);
    aroundField_->Draw(vp_);

    player_ptr->DrawParticle(vp_);
    enemy_ptr->DrawParticle(vp_);
    aroundField_->DrawParticle(vp_);

    //fadeOut_->Draw(vp_);
    gameUI_->Draw();

    tutorialUI_->Draw();

    followCamera_->DrawFrustum();
    enemy_ptr->DrawFrustum();

    spriteManager_->DrawAll();
}

// ============================================================
void TutorialScene::DrawForOffScreen() {
}

// ============================================================
void TutorialScene::AddSceneSetting() {
    debugCamera_->imgui();
    followCamera_->imgui();
    vp_.ShowDebugInfo();
    MotionEditor::GetInstance()->DrawImGui();
    tutorialSystem_->DrawImGui();
}

// ============================================================
void TutorialScene::AddObjectSetting() {
    player_ptr->Debug();
    enemy_ptr->Debug();
    enemyUI_->Debug();
}

// ============================================================
void TutorialScene::AddParticleSetting() {
}

// ============================================================
void TutorialScene::CameraUpdate() {
    if (player_ptr->GetIsAlive()) {
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

// ============================================================
void TutorialScene::ChangeScene() {
}

// ============================================================
//  HandleEnemySpawnRequest
//  TutorialSystem からのリクエストを受けてエネミーを表示/非表示にする
// ============================================================
void TutorialScene::HandleEnemySpawnRequest() {
    if (tutorialSystem_->ShouldSpawnEnemy()) {
        // エネミーを出現させる
        // ※ enemy_ptr に対してご使用の非表示API（SetVisible等）に合わせて差し替えてください
        enemy_ptr->GetAlive() = true;
        tutorialSystem_->ConsumeSpawnRequest();
    }

    if (tutorialSystem_->ShouldDespawnEnemy()) {
        // エネミーを非表示にする
        enemy_ptr->GetAlive() = false;
        tutorialSystem_->ConsumeDespawnRequest();
    }
}
#include "GameScene.h"

#include "Engine/Utility/Scene/SceneManager.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>

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
    /// ビヘイビアツリーの構築
    /// ===================================================
    // enemy_->InitializeBehaviorTree();

    // #ifdef _DEBUG
    //     // エディターとビヘイビアツリーを連携
    //     enemy_->SetBehaviorTreeEditor(behaviorTreeEditor_.get());
    //      if (enemy_->GetBehaviorRoot()) {
    //          behaviorTreeEditor_->LoadSettings("DefaultTree", enemy_->GetBehaviorRoot());
    //      }
    // #endif

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

    player_ptr->SetStart(startCamera_->IsComplete());

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
    playerUI_->Debug();
    enemyUI_->Debug();
    player_ptr->Debug();
    enemy_ptr->Debug();
    for (auto &bullet : player_ptr->GetBullets()) {
        bullet->ImGui();
    }
    // #ifdef _DEBUG
    //     // ビヘイビアツリーエディターの表示
    //     if (ImGui::CollapsingHeader("Behavior Tree Editor")) {
    //         behaviorTreeEditor_->DrawEditor(enemy_ptr->GetBehaviorRoot());
    //     }
    // #endif // _DEBUG
}

void GameScene::AddParticleSetting() {

    fadeOut_->ImGui();
}

void GameScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        followCamera_->Update();
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
            vp_.matWorld_ = followCamera_->GetViewProjection().matWorld_;
            vp_.matView_ = followCamera_->GetViewProjection().matView_;
            vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
        }
    }
}

void GameScene::ChangeScene() {
    if (!enemy_ptr->GetAlive()) {
        sceneManager_->NextSceneReservation("CLEAR");
    }
}
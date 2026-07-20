#include "TrainingScene.h"
#include "utility/scene/SceneManager.h"
#include <Frame.h>
#include <shadow/ShadowMap.h>

REGISTER_SCENE("TRAINING", TrainingScene)

using namespace Hagine;
void TrainingScene::Initialize()
{
    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    BaseScene::Initialize();
    pLightGroup_->LoadLightData("GameLight");
    debugCamera_ = std::make_unique<DebugCamera>();
    player_ = std::make_unique<Player>();
    enemy_ = std::make_unique<Enemy>();
    followCamera_ = std::make_unique<FollowCamera>();
    ground_ = std::make_unique<Ground>();
    pSkyBox_ = SkyBox::GetInstance();
    playerUI_ = std::make_unique<PlayerUI>();
    enemyUI_ = std::make_unique<EnemyUI>();
    gameUI_ = std::make_unique<GameUI>();
    aroundField_ = std::make_unique<AroundField>();
    inputDisplay_ = std::make_unique<InputDisplayUI>();
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
    pSkyBox_->Initialize("game/skybox.dds");
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

    /// ===================================================
    /// ダミー敵の設定：AI停止・常時表示・被弾のみ有効・HP0で復活
    /// ===================================================
    enemy_ptr->SetDummy(true);
    enemy_ptr->SetStart(true);
    enemy_ptr->GetAlive() = true;

    /// ===================================================
    /// プレイヤーを操作可能にする
    /// ===================================================
    player_ptr->SetStart(true);

    /// ===================================================
    /// 入力表示UI
    /// ===================================================
    inputDisplay_->Initialize(player_ptr, gamePad_.get());

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    // GPU パーティクル Compute フェーズ
    pDrawSystem_->Register("TrainingScene_Particle_Compute", DrawSystem::kGPUParticleCompute, [this](const ViewProjection &vp) {
        player_ptr->DrawParticleCompute(vp);
        enemy_ptr->DrawParticleCompute(vp);
        aroundField_->DrawParticleCompute(vp);
    });
    pDrawSystem_->Register("TrainingScene_3D", DrawLayer::PreEffect, [this](const ViewProjection &vp) {
        pObjectManager_->Draw(vp);
        pSkyBox_->Draw(vp);
        ground_->Draw(vp);
        aroundField_->Draw(vp);
        player_ptr->DrawParticle(vp); // Graphics フェーズのみ実行される
        enemy_ptr->DrawParticle(vp);
        aroundField_->DrawParticle(vp);
        followCamera_->DrawFrustum();
        enemy_ptr->DrawFrustum();
    });
    pDrawSystem_->Register("TrainingScene_UI", DrawLayer::PostEffect, [this](const ViewProjection &) {
        gameUI_->Draw();
        pSpriteManager_->DrawAll();
        playerUI_->Draw();
        enemyUI_->Draw();
        inputDisplay_->Draw();
    });
}

void TrainingScene::Finalize()
{
    /// ===================================================
    /// 終了処理
    /// ===================================================
    inputDisplay_->Finalize();
    aroundField_->Finalize();
    BaseScene::Finalize();
}

void TrainingScene::Update()
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
    player_ptr->SetActiveDebugCamera(debugCamera_->GetActive());

    // シャドウマップをプレイヤーに追従
    Vector3 p = player_ptr->GetWorldPosition();
    ShadowMap::GetInstance()->SetLightTarget({p.x, 0.0f, p.z});

    // 入力の更新
    gamePad_->Update();
    gameUI_->Update();

    // ポーズ状態をプレイヤー・敵へ伝搬する（伝えないとポーズ中も行動し続ける）
    player_ptr->SetPause(gameUI_->GetIsPause());
    enemy_ptr->SetPause(gameUI_->GetIsPause());

    // UIの更新
    playerUI_->Update();
    enemyUI_->Update();

    // 入力表示UI（gamePad_ 更新後に呼ぶ）
    inputDisplay_->Update();
}

void TrainingScene::Draw()
{
    // 描画は DrawSystem が管理
}

void TrainingScene::DrawForOffScreen()
{
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void TrainingScene::AddSceneSetting()
{
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->imgui();
    followCamera_->imgui();
    vp_.ShowDebugInfo();
}

void TrainingScene::AddObjectSetting()
{
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
    player_ptr->Debug();
    enemy_ptr->Debug();
    enemyUI_->Debug();
}

void TrainingScene::AddParticleSetting()
{
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
    DrawParticleEditorUI();
}

void TrainingScene::CameraUpdate()
{
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    if (debugCamera_->GetActive())
    {
        debugCamera_->Update();
    }
    else
    {
        followCamera_->Update();
        vp_.matWorld_ = followCamera_->GetViewProjection().matWorld_;
        vp_.matView_ = followCamera_->GetViewProjection().matView_;
        vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
    }
}

void TrainingScene::ChangeScene()
{
    /// ===================================================
    /// シーン切り替え：入力でタイトルへ戻る
    /// ===================================================
    if (gameUI_->GetIsBackTitle())
    {
        pSceneManager_->NextSceneReservation("TITLE");
    }
}

#include "TrainingScene.h"
#include <Application/camera/follow/FollowCameraFactory.h>
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
    player_ = std::make_unique<Player>();
    enemy_ = std::make_unique<Enemy>();
    followCamera_ = FollowCameraFactory::Create();
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
    /// ダミー敵の設定：AI停止・常時表示・被弾のみ有効・HP0で復活
    /// ===================================================
    pEnemy_->SetDummy(true);
    pEnemy_->SetStart(true);
    pEnemy_->GetAlive() = true;

    /// ===================================================
    /// プレイヤーを操作可能にする
    /// ===================================================
    pPlayer_->SetStart(true);

    /// ===================================================
    /// 入力表示UI
    /// ===================================================
    inputDisplay_->Initialize(pPlayer_, gamePad_.get());

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    // GPU パーティクル（ParticleCSSpawner 所有）の Compute/Graphics はエンジンが自動で回すため、
    // シーン側で Compute フェーズを登録する必要はない。CPU パーティクルは各 DrawParticle 内で描画する。
    pDrawSystem_->Register("TrainingScene_3D", DrawLayer::PreEffect, [this](const ViewProjection &vp) {
        pObjectManager_->Draw(vp);
        pSkyBox_->Draw(vp);
        ground_->Draw(vp);
        aroundField_->Draw(vp);
        pPlayer_->DrawParticle(vp);
        pEnemy_->DrawParticle(vp);
        followCamera_->DrawFrustum();
        pEnemy_->DrawFrustum();
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
    pPlayer_->SetActiveDebugCamera(IsDebugCameraActive());

    // シャドウマップをプレイヤーに追従
    Vector3 p = pPlayer_->GetWorldPosition();
    ShadowMap::GetInstance()->SetLightTarget({p.x, 0.0f, p.z});

    // 入力の更新
    gamePad_->Update();
    gameUI_->Update();

    // ポーズ状態をプレイヤー・敵へ伝搬する（伝えないとポーズ中も行動し続ける）
    pPlayer_->SetPause(gameUI_->GetIsPause());
    pEnemy_->SetPause(gameUI_->GetIsPause());

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
    DrawDebugCameraImGui();
    followCamera_->DrawImGui();
    camera_->ShowDebugWindow();
}

void TrainingScene::AddObjectSetting()
{
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
    pPlayer_->Debug();
    pEnemy_->Debug();
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
    // どのカメラで描くかは CameraManager のアクティブ切り替えで決める
    UpdateDebugCamera(); // 有効中は自動でデバッグカメラへ切り替わる
    if (!IsDebugCameraActive())
    {
        followCamera_->Update();
        pCameraManager_->SetActive(followCamera_->GetCamera());
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

#include "GameScene.h"

#include <Application/camera/follow/FollowCameraFactory.h>
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
    camera_->SetPosition({0.0f, 0.0f, -30.0f});

    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    player_ = std::make_unique<Player>();
    enemy_ = std::make_unique<Enemy>();
    followCamera_ = FollowCameraFactory::Create();
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
    player_->Init("player");
    enemy_->Init("pEnemy");
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
    player_->InitializeShake();
    enemy_->InitializeShake();
    enemy_->SetTarget(player_.get());

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

#ifdef _DEBUG
    behaviorTreeEditor_->SetDebugTargets(pEnemy_, pPlayer_);
#else
    /// ===================================================
    /// BehaviorTreeのロード
    /// ===================================================
    behaviorTreeRoot_ = BehaviorTreeLoader::LoadAndBuild(kBTFolder, kBTFileName);
    if (behaviorTreeRoot_)
    {
        pEnemy_->SetBehaviorTree(behaviorTreeRoot_);
    }
#endif

    /// ===================================================
    /// シーン開始時のパーティクル遷移
    /// タイトル演出経由（通常トランジション無効で遷移してきた）のときだけ使う。
    /// チュートリアル等から通常トランジションで来た場合はそのまま開始する
    /// ===================================================
    if (!SceneManager::GetInstance()->GetSceneTransition()->GetUseTransition())
    {
        fadeOut_ = std::make_unique<FadeOut>();
        fadeOut_->Initialize(); // 内部で通常トランジションを再有効化する
    }

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    // GPU パーティクル（ParticleCSSpawner 所有）の Compute/Graphics はエンジンが自動で回すため、
    // シーン側で Compute フェーズを登録する必要はない。CPU パーティクルは各 DrawParticle 内で描画する。
    pDrawSystem_->Register("GameScene_3D", DrawLayer::PreEffect, [this](const ViewProjection &vp) {
        pObjectManager_->Draw(vp);
        pSkyBox_->Draw(vp);
        aroundField_->Draw(vp);
        pPlayer_->DrawParticle(vp);
        pEnemy_->DrawParticle(vp);
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
    if (fadeOut_)
    {
        fadeOut_->Finalize();
    }
    aroundField_->Finalize();
    pSceneManager_->SetClearTime(clearTimer_);
    pSceneManager_->SetIsGameOver(!pPlayer_->GetIsAlive());
    if (pPlayer_->GetIsAlive())
    {
        pSceneManager_->SetHP(pPlayer_->GetHP());
    }
    else
    {
        // ゲームオーバー時は残りHPを0として扱う（リザルト表示用）
        pSceneManager_->SetHP(0.0f);
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
            pEnemy_->SetBehaviorTree(runtimeRoot);
        }
    }
#endif

    aroundField_->Update();
    if (fadeOut_)
    {
        fadeOut_->Update(vp());
    }
    playerUI_->Update();
    enemyUI_->Update();

    // シャドウマップをプレイヤーに追従
    Vector3 p = pPlayer_->GetWorldPosition();
    ShadowMap::GetInstance()->SetLightTarget({p.x, 0.0f, p.z});
    pPlayer_->SetActiveDebugCamera(IsDebugCameraActive());

#ifdef _DEBUG
    // パーティクル遷移中は操作開始しない（遷移中にカメラが動くのを防ぐ）。遷移なしなら即開始
    if (!fadeOut_ || fadeOut_->IsFinish())
    {
        pPlayer_->SetStart(true);
        pEnemy_->SetStart(true);
    }
#else
    // 開始演出待ち（パーティクル遷移→開始カメラ演出の順に進む）
    if (startCamera_->IsComplete())
    {
        pPlayer_->SetStart(true);
        pEnemy_->SetStart(true);
        if (pEnemy_->GetIsAlive())
        {
            clearTimer_ += Frame::DeltaTime();
        }
    }
#endif

    // 死亡演出中のモデル非表示
    if (!pPlayer_->GetIsAlive() && deathCamera_->IsHalfway())
    {
        pEnemy_->SetIsModelDraw(false);
    }

    // UIの更新
    gameUI_->Update();
    pPlayer_->SetPause(gameUI_->GetIsPause());
    pEnemy_->SetPause(gameUI_->GetIsPause());
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
    DrawDebugCameraImGui();
    followCamera_->DrawImGui();
    startCamera_->DrawImGui();
    camera_->ShowDebugWindow();
    MotionEditor::GetInstance()->DrawImGui();
}

void GameScene::AddObjectSetting()
{
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
    pPlayer_->Debug();
    pEnemy_->Debug();
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
    if (fadeOut_)
    {
        fadeOut_->DrawImGui();
    }
}

void GameScene::CameraUpdate()
{
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    // どのカメラで描くかは CameraManager のアクティブ切り替えで決める
    // （行列のコピーは不要。アクティブなカメラがそのまま描画に使われる）
    UpdateDebugCamera(); // 有効中は自動でデバッグカメラへ切り替わる

    if (pPlayer_->GetIsAlive())
    {
        if (!IsDebugCameraActive())
        {
            followCamera_->Update();
#ifndef _DEBUG
            // パーティクル遷移中はスタートカメラの初期視点で静止する。
            // 遷移パーティクル（長方形）はカメラ正面に追従配置されるため、
            // スタートカメラ初期位置の正面にパーティクルが発生し、
            // 発生停止から待機時間を置いてカメラ演出（Move）を開始する
            if (fadeOut_ && !fadeOut_->IsCameraStartReady())
            {
                startCamera_->Update(); // Move()を呼ばないため初期位置に静止したまま
                pCameraManager_->SetActive(startCamera_->GetCamera());
            }
            else if (!startCamera_->IsComplete())
            {
                startCamera_->Move();
                startCamera_->SetTargetCamera(*followCamera_->GetCamera());
                startCamera_->Update();
                pCameraManager_->SetActive(startCamera_->GetCamera());
            }
            else
            {
#endif
                pCameraManager_->SetActive(followCamera_->GetCamera());
#ifndef _DEBUG
            }
#endif
        }
    }
    else
    {
        if (!deathCamera_->IsComplete() && !deathCameraStarted_)
        {
            // 空中で死んだ場合は地面まで落下してから倒れるため、
            // カメラの注視点は現在位置ではなく落下先（接地位置）に合わせる
            Vector3 deathPos = pPlayer_->GetWorldPosition();
            deathPos.y = Ground::GetStandingY(deathPos.x, deathPos.z);
            deathCamera_->StartEasing(*followCamera_->GetCamera(), deathPos);
            deathCameraStarted_ = true;
        }
        deathCamera_->Update();
        if (!IsDebugCameraActive())
        {
            pCameraManager_->SetActive(deathCamera_->GetCamera());
        }
    }
}

void GameScene::ChangeScene()
{
    /// ===================================================
    /// シーン切り替え
    /// ===================================================
    if (!pPlayer_->GetIsAlive() && deathCamera_->IsComplete())
    {
        gameOverTimer_ += Frame::DeltaTime();
        pPlayer_->SetIsDeathStaging(true);
        // 死亡アニメーション(約2.6秒)を再生し終えてから粒子化演出が始まるため、
        // 演出を見届けられるだけの猶予を取ってからシーンを切り替える
        if (gameOverTimer_ >= kGameOverWaitTime && !isGameOver_)
        {
            pSceneManager_->NextSceneReservation("CLEAR");
            isGameOver_ = true;
        }
    }

    if (!pEnemy_->GetIsAlive())
    {
        // 敵の死亡アニメーション(約2.6秒)→粒子化して消える演出を見届けてから
        // リザルトへ切り替える
        enemyDownTimer_ += Frame::DeltaTime();
        if (enemyDownTimer_ >= kEnemyDeathWaitTime)
        {
            pSceneManager_->NextSceneReservation("CLEAR");
        }
    }

    if (gameUI_->GetIsBackTitle())
    {
        pSceneManager_->NextSceneReservation("TITLE");
    }
}

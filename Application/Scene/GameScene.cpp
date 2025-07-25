#include "GameScene.h"

#include <Application/Utility/MotionEditor/MotionEditor.h>

void GameScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    /// ===================================================
    /// 生成
    /// ===================================================
    debugCamera_ = std::make_unique<DebugCamera>();
    player_ = std::make_unique<Player>();
    enemy_ = std::make_unique<Enemy>();
    followCamera_ = std::make_unique<FollowCamera>();
    skyDome_ = std::make_unique<SkyDome>();
    ground_ = std::make_unique<Ground>();

    /// ===================================================
    /// 初期化
    /// ===================================================
    debugCamera_->Initialize(&vp_);
    player_->Init("player");
    enemy_->Init("enemy");
    skyDome_->Init("SkyDome");
    ground_->Init("Ground");
    followCamera_->Init();

    /// ===================================================
    /// ポインタ共有
    /// ===================================================
    enemy_ptr = enemy_.get();
    player_ptr = player_.get();
    MotionEditor::GetInstance()->Register(player_ptr);
    MotionEditor::GetInstance()->Register(enemy_ptr);

    /// ===================================================
    /// セット
    /// ===================================================
    followCamera_->SetPlayer(player_.get());
    player_->SetCamera(followCamera_.get());
    player_->SetEnemy(enemy_.get());
    BaseObjectManager::GetInstance()->AddObject(std::move(player_));
    BaseObjectManager::GetInstance()->AddObject(std::move(enemy_));
}

void GameScene::Finalize() {
    BaseScene::Finalize();
}

void GameScene::Update() {

   /* if (!debugCamera_->GetActive()) {
        BaseObjectManager::GetInstance()->Update();
    }*/

    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    skyDome_->Update();
    ground_->Update();
}

void GameScene::Draw() {
    /// -------描画処理開始-------

    BaseObjectManager::GetInstance()->Draw(vp_);

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //-------------------------

    //-----3DObjectの開始-----
    skyDome_->Draw(vp_);
    ground_->Draw(vp_);
    DrawLine3D::GetInstance()->DrawGrid(-0.95f, 64, 1000, {0.0f, 0.0f, 1.0f, 0.75f});
    //-----------------------

    //------Particleの描画開始-------
    enemy_ptr->DrawParticle(vp_);
    player_ptr->DrawParticle(vp_);
    //-----------------------------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    /// -------描画処理終了-------
}

void GameScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    //-----3DObjectの描画-----

    //-----------------------

    //------Particleの描画開始-------

    //-----------------------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void GameScene::AddSceneSetting() {
    debugCamera_->imgui();
    followCamera_->imgui();

    MotionEditor::GetInstance()->DrawImGui();

}

void GameScene::AddObjectSetting() {
    player_ptr->Debug();
}

void GameScene::AddParticleSetting() {
}

void GameScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        followCamera_->Update();
        vp_.matWorld_ = followCamera_->GetViewProjection().matWorld_;
        vp_.matView_ = followCamera_->GetViewProjection().matView_;
        vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
    }
}

void GameScene::ChangeScene() {
}
#include "ClearScene.h"
#include "Engine/Utility/Scene/SceneManager.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
void ClearScene::Initialize() {
    audio_ = Audio::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.translation_ = {0.0f, 0.0f, -30.0f};

    /// ===================================================
    /// インスタンス生成
    /// ===================================================
    debugCamera_ = std::make_unique<DebugCamera>();
    ground_ = std::make_unique<Ground>();
    skyBox_ = SkyBox::GetInstance();

    /// ===================================================
    /// 初期化
    /// ===================================================
    debugCamera_->Initialize(&vp_);
    ground_->Init("Ground");
    skyBox_->Initialize("game/skybox.dds");
    vp_.Initialize("P_StartCamera");

    /// ===================================================
    /// ロード
    /// ===================================================
    BaseObjectManager::GetInstance()->LoadAll("ClearScene");
    LightGroup::GetInstance()->LoadLightData("GameLight");
    SpriteManager::GetInstance()->SetSaveFolder("Result");
    SpriteManager::GetInstance()->LoadAllSprites();

    /// ===================================================
    /// ポインタ共有
    /// ===================================================
    RightHand_ = BaseObjectManager::GetInstance()->GetObjectByName("sphere_1");
    LeftHand_ = BaseObjectManager::GetInstance()->GetObjectByName("sphere_2");

    /// ===================================================
    /// 登録
    /// ===================================================
    MotionEditor::GetInstance()->Register(RightHand_);
    MotionEditor::GetInstance()->Register(LeftHand_);
}

void ClearScene::Finalize() {
    BaseScene::Finalize();
}

void ClearScene::Update() {
    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    ground_->Update();

    if (!secondMove_ && !motionStarted_) {
        MotionEditor::GetInstance()->PlayFromFile(LeftHand_, "LeftPunch");
        MotionEditor::GetInstance()->PlayFromFile(RightHand_, "RightBack");
        motionStarted_ = true;
    }

    if (!secondMove_ &&
        MotionEditor::GetInstance()->IsAttackFinished(LeftHand_) &&
        MotionEditor::GetInstance()->IsAttackFinished(RightHand_)) {
        secondMove_ = true;
        MotionEditor::GetInstance()->PlayFromFile(LeftHand_, "LeftBack");
        MotionEditor::GetInstance()->PlayFromFile(RightHand_, "RightPunch");
    }
}

void ClearScene::Draw() {
    /// -------描画処理開始-------

    BaseObjectManager::GetInstance()->Draw(vp_);

    skyBox_->Draw(vp_);

    ground_->Draw(vp_);

    SpriteManager::GetInstance()->DrawAll();

    /// -------描画処理終了-------
}

void ClearScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// -------描画処理終了-------
}

void ClearScene::AddSceneSetting() {
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void ClearScene::AddObjectSetting() {
}

void ClearScene::AddParticleSetting() {
}

void ClearScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
        vp_.EaseCameraMove(EasingType::InCubic, "P_EndCamera", 1.5f);
    }
}

void ClearScene::ChangeScene() {
    // if (input_->TriggerKey(DIK_SPACE)) {
    //     // シーンを変更
    //     sceneManager_->NextSceneReservation("TITLE");
    // }
}
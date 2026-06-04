#include "DemoScene.h"
#include "Engine/Utility/Scene/SceneManager.h"

using namespace Hagine;
void DemoScene::Initialize() {
    /// ===================================================
    /// 初期化
    /// ===================================================
    BaseScene::Initialize();
    vp_.Initialize("DemoCamera");
    lightGroup_->LoadLightData("DemoLight");

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    /// ===================================================
    /// GPU パーティクルエフェクト登録
    /// ===================================================
    //ptCSEditor_->AddParticleEmitter("flamePillar");   // 炎の柱
    //ptCSEditor_->AddParticleEmitter("magicArray");    // 魔法陣
    //ptCSEditor_->AddParticleEmitter("burstFlash");    // 爆裂閃光（一回発生ボタンで起爆）
    //ptCSEditor_->AddParticleEmitter("lightningBolt"); // 電光柱
    //ptCSEditor_->AddParticleEmitter("soulVortex");    // 魂の渦

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    // GPU パーティクル（ptCSEditor_）の Compute/Graphics は DrawSystem が全体駆動するため
    // シーン側では登録しない（全シーンでプレビュー・確認できるようにするため）。
    drawSystem_->Register("DemoScene_All", DrawLayer::kPreEffect, [this](const ViewProjection &vp) {
        spriteManager_->DrawAll();
        objectManager_->Draw(vp);
        ptEditor_->DrawAll(vp);
    });
}

void DemoScene::Finalize() {
    /// ===================================================
    /// 終了処理
    /// ===================================================
    BaseScene::Finalize();
}

void DemoScene::Update() {
    /// ===================================================
    /// 更新処理
    /// ===================================================

    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();
}

void DemoScene::Draw() {
    // 描画は DrawSystem が管理
}

void DemoScene::DrawForOffScreen() {
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void DemoScene::AddSceneSetting() {
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void DemoScene::AddObjectSetting() {
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
}

void DemoScene::AddParticleSetting() {
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
    DrawParticleEditorUI();
}

void DemoScene::CameraUpdate() {
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void DemoScene::ChangeScene() {
    /// ===================================================
    /// シーン切り替え
    /// ===================================================
}

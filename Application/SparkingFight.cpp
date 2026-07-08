#include "SparkingFight.h"
#include <Debug/CpuProfiler/CpuProfiler.h>
#include <Frame.h>

using namespace Hagine;

void SparkingFight::Initialize() {
    Framework::Initialize();
    Framework::LoadResource();
    LoadGameResources();
    Framework::PlaySounds();
    Framework::RegisterShortcutKey();

    // 最初のシーンを予約（シーンは各 .cpp の REGISTER_SCENE で自己登録済み）
    sceneManager_->NextSceneReservation("TITLE");
}

void SparkingFight::LoadGameResources() {
    // CPUパーティクルのエミッター
    particleEditor_->AddParticleEmitter("hitEmitter");
    particleEditor_->AddParticleEmitter("bulletEmitter");
    particleEditor_->AddParticleEmitter("enemyBulletEmitter");
    particleEditor_->AddParticleEmitter("chageBullet");
    particleEditor_->AddParticleEmitter("RushEmitter");
    particleEditor_->AddParticleEmitter("punchEmitter");
    particleEditor_->AddParticleEmitter("smokeEmitter");

    // GPU(CS)パーティクルのエミッター
    particleCSEditor_->AddParticleEmitter("playerAura");
    particleCSEditor_->AddParticleEmitter("FadeOut");
    particleCSEditor_->AddParticleEmitter("death");
    particleCSEditor_->AddParticleEmitter("death_arm");
    particleCSEditor_->AddParticleEmitter("makan_main");
    particleCSEditor_->AddParticleEmitter("makan_around");
    particleCSEditor_->AddParticleEmitter("chargeEmitter");
    particleCSEditor_->AddParticleEmitter("fireWork_explosion");
    particleCSEditor_->AddParticleEmitter("fireWork_Trail");
    particleCSEditor_->AddParticleEmitter("ChargeAura");
    particleCSEditor_->AddParticleEmitter("enemyChargeAura");
    particleCSEditor_->AddParticleEmitter("AroundField");

    // パーティクルフィールド
    particleCSFieldManager_->CreateField("GeneratedField", "GeneratedField");
}

void SparkingFight::Finalize() {
    // -----ゲーム固有の処理-----

    // -----------------------

    Framework::Finalize();
}

void SparkingFight::Update() {
    // フレーム先頭：前フレームの計測結果を確定し、当フレームの計測を開始する
#ifdef _DEBUG
    CpuProfiler::GetInstance()->BeginFrame();
#endif

    Framework::Update();

    // -----ゲーム固有の処理-----
#ifdef _DEBUG
    {
        HAGINE_CPU_PROFILE("Update/ImGuiUI(build)");
        if (imGuiManager_->GetEditorMode()) {
            input_->UpdateRay(*sceneManager_->GetBaseScene()->GetViewProjection(), {imGuiManager_->GetScenePos(), imGuiManager_->GetSceneSize()}, 10000.0f);
        } else {
            input_->UpdateRay(*sceneManager_->GetBaseScene()->GetViewProjection(), {Vector2(0, 0), Vector2(float(WinApp::kClientWidth), float(WinApp::kClientHeight))}, 10000.0f);
        }

        imGuiManager_->Begin();
        imGuizmoManager_->BeginFrame();
        imGuizmoManager_->SetViewProjection(sceneManager_->GetBaseScene()->GetViewProjection());
        imGuiManager_->UpdateIni();
        imGuiManager_->SetCurrentScene(sceneManager_->GetBaseScene());
        imGuiManager_->ShowMainMenu();
        if (imGuiManager_->GetIsShowMainUI()) {
            imGuiManager_->ShowDockSpace();
            imGuiManager_->ShowSceneWindow(offscreen_.get(), sceneManager_->GetCurrentSceneName());
        }
        imGuiManager_->ShowMainUI(offscreen_.get());
        imGuiManager_->End();
    }
#endif // _DEBUG
#ifndef _DEBUG
    input_->UpdateRay(*sceneManager_->GetBaseScene()->GetViewProjection(), {Vector2(0, 0), Vector2(float(WinApp::kClientWidth), float(WinApp::kClientHeight))});
#endif // _DEBUG

    {
        HAGINE_CPU_PROFILE("Update/MotionEditor");
        motionEditor_->Update(Frame::DeltaTime());
    }

    // -----------------------
}

void SparkingFight::Draw() {
    {
        HAGINE_CPU_PROFILE("Draw/DrawSystem(rec)");
        drawSystem_->Draw(*sceneManager_->GetBaseScene()->GetViewProjection());
    }

#ifdef _DEBUG
    {
        HAGINE_CPU_PROFILE("Draw/ImGuiRender");
        imGuiManager_->Draw();
    }
#endif // _DEBUG

    {
        // PostDraw は Present(VSync)・FPS固定スリープ・GPU完了待ちを含む。
        // ここが大きくCPUフェーズ計が予算内なら描画(GPU)/VSync待ちが支配的。
        HAGINE_CPU_PROFILE("Draw/Present(待ちVSync/GPU)");
        dxCommon_->PostDraw();
    }

#ifdef _DEBUG
    {
        HAGINE_CPU_PROFILE("Draw/MultiViewport");
        // メインの Present 後に、独立OSウィンドウへ切り離したImGuiウィンドウを描画する
        imGuiManager_->RenderMultiViewport();
    }
#endif // _DEBUG
}

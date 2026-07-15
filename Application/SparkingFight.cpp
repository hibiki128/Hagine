#include "SparkingFight.h"
#include <debug/profiler/CpuProfiler.h>
#include <Frame.h>

using namespace Hagine;

void SparkingFight::Initialize()
{
    Framework::Initialize();
    Framework::LoadResource();
    LoadGameResources();
    Framework::PlaySounds();
    Framework::RegisterShortcutKey();

    // 最初のシーンを予約（シーンは各 .cpp の REGISTER_SCENE で自己登録済み）
    pSceneManager_->NextSceneReservation("TITLE");
}

void SparkingFight::LoadGameResources()
{
    // CPUパーティクルのエミッター
    pParticleEditor_->AddParticleEmitter("hitEmitter");
    pParticleEditor_->AddParticleEmitter("bulletEmitter");
    pParticleEditor_->AddParticleEmitter("enemyBulletEmitter");
    pParticleEditor_->AddParticleEmitter("chargeBullet");
    pParticleEditor_->AddParticleEmitter("RushEmitter");
    pParticleEditor_->AddParticleEmitter("punchEmitter");
    pParticleEditor_->AddParticleEmitter("smokeEmitter");

    // GPU(CS)パーティクルのエミッター
    pParticleCSEditor_->AddParticleEmitter("playerAura");
    pParticleCSEditor_->AddParticleEmitter("FadeOut");
    pParticleCSEditor_->AddParticleEmitter("die");
    pParticleCSEditor_->AddParticleEmitter("makan_main");
    pParticleCSEditor_->AddParticleEmitter("makan_around");
    pParticleCSEditor_->AddParticleEmitter("chargeEmitter");
    pParticleCSEditor_->AddParticleEmitter("fireWork_explosion");
    pParticleCSEditor_->AddParticleEmitter("fireWork_Trail");
    pParticleCSEditor_->AddParticleEmitter("ChargeAura");
    pParticleCSEditor_->AddParticleEmitter("enemyChargeAura");
    pParticleCSEditor_->AddParticleEmitter("AroundField");

    // パーティクルフィールド
    pParticleCSFieldManager_->CreateField("GeneratedField", "GeneratedField");
}

void SparkingFight::Finalize()
{
    Framework::Finalize();
}

void SparkingFight::Update()
{
    // フレーム先頭：前フレームの計測結果を確定し、当フレームの計測を開始する
#ifdef _DEBUG
    CpuProfiler::GetInstance()->BeginFrame();
#endif

    Framework::Update();

    // -----ゲーム固有の処理-----
#ifdef _DEBUG
    {
        HAGINE_CPU_PROFILE("Update/ImGuiUI(build)");
        if (imGuiManager_->GetEditorMode())
        {
            pInput_->UpdateRay(*pSceneManager_->GetBaseScene()->GetViewProjection(), {imGuiManager_->GetScenePos(), imGuiManager_->GetSceneSize()}, 10000.0f);
        }
        else
        {
            pInput_->UpdateRay(*pSceneManager_->GetBaseScene()->GetViewProjection(), {Vector2(0, 0), Vector2(float(WinApp::kClientWidth), float(WinApp::kClientHeight))}, 10000.0f);
        }

        imGuiManager_->Begin();
        pImGuizmoManager_->BeginFrame();
        pImGuizmoManager_->SetViewProjection(pSceneManager_->GetBaseScene()->GetViewProjection());
        imGuiManager_->UpdateIni();
        imGuiManager_->SetCurrentScene(pSceneManager_->GetBaseScene());
        imGuiManager_->ShowMainMenu();
        if (imGuiManager_->GetIsShowMainUI())
        {
            imGuiManager_->ShowDockSpace();
            imGuiManager_->ShowSceneWindow(offscreen_.get(), pSceneManager_->GetCurrentSceneName());
        }
        imGuiManager_->ShowMainUI(offscreen_.get());
        imGuiManager_->End();
    }
#endif // _DEBUG
#ifndef _DEBUG
    pInput_->UpdateRay(*pSceneManager_->GetBaseScene()->GetViewProjection(), {Vector2(0, 0), Vector2(float(WinApp::kClientWidth), float(WinApp::kClientHeight))});
#endif // _DEBUG

    {
        HAGINE_CPU_PROFILE("Update/MotionEditor");
        pMotionEditor_->Update(Frame::DeltaTime());
    }

    // -----------------------
}

void SparkingFight::Draw()
{
    {
        HAGINE_CPU_PROFILE("Draw/DrawSystem(rec)");
        pDrawSystem_->Draw(*pSceneManager_->GetBaseScene()->GetViewProjection());
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
        pDxCommon_->PostDraw();
    }

#ifdef _DEBUG
    {
        HAGINE_CPU_PROFILE("Draw/MultiViewport");
        // メインの Present 後に、独立OSウィンドウへ切り離したImGuiウィンドウを描画する
        imGuiManager_->RenderMultiViewport();
    }
#endif // _DEBUG
}

#define NOMINMAX
#include "TitleScene.h"
#include "2d/SpriteManager.h"
#include "2d/text/TextRenderer.h"
#include "utility/scene/SceneManager.h"
#include "MyMath.h"
#include <Frame.h>
#include <graphics/texture/TextureManager.h>
#include <algorithm>
#include <cmath>

REGISTER_SCENE("TITLE", TitleScene)

using namespace Hagine;
void TitleScene::Initialize()
{
    /// ===================================================
    /// 初期化
    /// ===================================================
    BaseScene::Initialize();
    pLightGroup_->LoadLightData("TitleScene");
    vp_.eulerRotation_ = {
        degreesToRadians(26.3f),
        degreesToRadians(-122.7f),
        degreesToRadians(0.0f)};
    vp_.Initialize("CurrentCamera");
    pObjectManager_->LoadAll("TitleScene");
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
    pSkyBox_ = SkyBox::GetInstance();
    pSkyBox_->Initialize("game/skybox.dds");

    titleUI_ = std::make_unique<TitleUI>();
    titleUI_->Initialize();
    firstMove_ = false;
    secondMove_ = false;
    titlePhase_ = TitlePhase::WaitStart;
    menuIndex_ = 0;

    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);

    // チュートリアル/トレーニング選択メニューのスプライトを生成
    CreateMenuSprites();

    /// ===================================================
    /// DrawSystem 登録
    /// ===================================================
    pDrawSystem_->Register("TitleScene_3D", DrawLayer::PreEffect, [this](const ViewProjection &vp) {
        pSkyBox_->Draw(vp);
        pObjectManager_->Draw(vp);
        titleUI_->Draw(vp_);
    });
    pDrawSystem_->Register("TitleScene_UI", DrawLayer::PostEffect, [this](const ViewProjection &) {
        pSpriteManager_->DrawAll();
        DrawMenu();
    });
}

void TitleScene::Finalize()
{
    /// ===================================================
    /// 終了処理
    /// ===================================================
    BaseScene::Finalize();
}

void TitleScene::Update()
{
    /// ===================================================
    /// 更新処理
    /// ===================================================

    // ゲームパッドの更新
    gamePad_->Update();

    // カメラの更新
    CameraUpdate();

    // シーン切り替えの更新
    ChangeScene();

    // 経過時間の更新
    time_ += Frame::DeltaTime();

    // 一定時間経過後にカメラを移動
    if (time_ >= kMaxTime_ && !firstMove_)
    {
        vp_.EaseCameraMove(EasingType::InCubic, "TitleMovedCamera", 1.0f);
        firstMove_ = true;
    }

    // 進行フェーズごとの入力処理
    switch (titlePhase_)
    {
    case TitlePhase::WaitStart:
        // Press Start でメニューを開く
        if (time_ >= 3.0f && !vp_.GetIsCameraMove() && PressStartInput())
        {
            titlePhase_ = TitlePhase::Menu;
            menuIndex_ = 0;
            menuAnimTimer_ = 0.0f;  // 出現アニメを最初から
            menuSelectLerp_ = 0.0f; // カーソルは上（チュートリアル）から
            titleUI_->HidePressStart();
        }
        break;

    case TitlePhase::Menu: {
        const float dt = Frame::DeltaTime();

        // 出現アニメの経過時間を進める
        menuAnimTimer_ += dt;

        // カーソル移動（チュートリアル/ゲーム/トレーニングを上下で選択）
        if (UpInput() && menuIndex_ > 0)
        {
            --menuIndex_;
        }
        if (DownInput() && menuIndex_ < kMenuItemCount - 1)
        {
            ++menuIndex_;
        }

        // カーソル/ハイライトを選択項目へ滑らかに補間する
        const float selT = std::clamp(kMenuSelectLerp * dt, 0.0f, 1.0f);
        menuSelectLerp_ += (static_cast<float>(menuIndex_) - menuSelectLerp_) * selT;

        // 決定 → 退場アニメへ（即遷移せず、メニューをスムーズに閉じてから分岐する）
        if (ConfirmInput())
        {
            titlePhase_ = TitlePhase::MenuClosing;
            menuCloseTimer_ = 0.0f;
        }
        break;
    }

    case TitlePhase::MenuClosing:
        // 退場アニメを進め、完了したら選択に応じて分岐する
        menuCloseTimer_ += Frame::DeltaTime();
        if (menuCloseTimer_ >= kMenuCloseDuration)
        {
            if (menuIndex_ == kMenuIndexTraining)
            {
                // トレーニング: カメラ演出なし、通常フェードで遷移
                pSceneManager_->GetSceneTransition()->SetUseTransition(true);
                pSceneManager_->NextSceneReservation("TRAINING");
                titlePhase_ = TitlePhase::Training;
            }
            else
            {
                // チュートリアル / ゲーム: カメラ演出→トランジションなしで遷移
                // （遷移先シーン側のパーティクル遷移で画面が現れる）
                cinematicNextScene_ = (menuIndex_ == kMenuIndexTutorial) ? "TUTORIAL" : "GAME";
                vp_.EaseCameraMove(EasingType::InQuint, "EnemyEyeCamera", 1.0f);
                titleUI_->RequestStartCinematic();
                secondMove_ = true;
                titlePhase_ = TitlePhase::Cinematic;
            }
        }
        break;

    default:
        break;
    }

    // UIの更新
    titleUI_->Update();
}

void TitleScene::Draw()
{
    // 描画は DrawSystem が管理
}

void TitleScene::DrawForOffScreen()
{
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void TitleScene::AddSceneSetting()
{
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void TitleScene::AddObjectSetting()
{
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
}

void TitleScene::AddParticleSetting()
{
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
    DrawParticleEditorUI();
}

void TitleScene::CameraUpdate()
{
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    debugCamera_->Update();
}

void TitleScene::ChangeScene()
{
    /// ===================================================
    /// シーン切り替え（開始演出の完了を待って遷移）
    /// ===================================================
    if (titlePhase_ == TitlePhase::Cinematic &&
        !vp_.GetIsCameraMove() && titleUI_->GetIsFinish())
    {
        pSceneManager_->GetSceneTransition()->SetUseTransition(false);
        pSceneManager_->NextSceneReservation(cinematicNextScene_);
    }
}

// 選択メニュー
void TitleScene::CreateMenuSprites()
{
    auto keys = TextureManager::GetInstance()->GetAllFontKeys();
    if (keys.empty())
    {
        return;
    }
    const std::string &fontKey = keys[0];

    auto make = [&](std::unique_ptr<Sprite> &out, const std::string &name,
                    const std::string &text, Vector2 pos) {
        TextRenderer::GetInstance()->CreateTextSprite(
            name, text, fontKey, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
            true, 5.0f, {0.0f, 0.0f, 0.0f, 1.0f});
        SpriteManager::GetInstance()->UnregisterSprite(name);

        out = std::make_unique<Sprite>();
        out->Initialize("Text/" + name + ".png", pos, {1.0f, 1.0f, 1.0f, 1.0f});
        Vector2 native = out->GetSize();
        float scale = (native.y > 0.0f) ? kMenuTextHeight / native.y : 1.0f;
        out->SetSize({native.x * scale, kMenuTextHeight});
        out->SetPosition(pos);
    };

    menuTutorialPos_ = {kMenuX, kMenuTutorialY};
    menuGamePos_ = {kMenuX, kMenuGameY};
    menuTrainingPos_ = {kMenuX, kMenuTrainingY};
    make(menuTutorial_, "title_menu_tutorial", "チュートリアル", menuTutorialPos_);
    make(menuGame_, "title_menu_game", "ゲーム", menuGamePos_);
    make(menuTraining_, "title_menu_training", "トレーニング", menuTrainingPos_);
    make(menuCursor_, "title_menu_cursor", "▶", {kMenuX - kMenuCursorGap, kMenuTutorialY});
}

void TitleScene::DrawMenu()
{
    if (titlePhase_ != TitlePhase::Menu && titlePhase_ != TitlePhase::MenuClosing)
    {
        return;
    }

    const float dur = kMenuSlideDuration;

    Sprite *sprites[kMenuItemCount] = {menuTutorial_.get(), menuGame_.get(), menuTraining_.get()};
    const Vector2 endPos[kMenuItemCount] = {menuTutorialPos_, menuGamePos_, menuTrainingPos_};

    // 各項目を画面右外から最終位置へ OutCubic でスライドイン（後の項目ほど少し遅らせる）
    // ApplyEasing はクランプしないので、時間は dur を超えないよう自前でクランプする
    // （超えると終点を越えて外挿し、メニューが画面外へ飛んでしまう）
    Vector2 pos[kMenuItemCount];
    float slide[kMenuItemCount];
    for (int i = 0; i < kMenuItemCount; ++i)
    {
        const float t = std::clamp(menuAnimTimer_ - kMenuStagger * static_cast<float>(i), 0.0f, dur);
        const Vector2 start = {kMenuStartX, endPos[i].y};
        pos[i] = ApplyEasing(EasingType::OutCubic, start, endPos[i], t, dur);
        slide[i] = std::clamp(t / dur, 0.0f, 1.0f);
    }

    // 決定後は右へずらしながらフェードアウト（ease-in）する
    float closeFade = 1.0f;
    if (titlePhase_ == TitlePhase::MenuClosing)
    {
        const float closeT = std::clamp(menuCloseTimer_ / kMenuCloseDuration, 0.0f, 1.0f);
        const float closeSlide = kMenuCloseSlide * (closeT * closeT); // ease-in
        for (auto &p : pos)
        {
            p.x += closeSlide;
        }
        closeFade = 1.0f - closeT;
    }

    for (int i = 0; i < kMenuItemCount; ++i)
    {
        if (!sprites[i])
        {
            continue;
        }
        // 選択ハイライトを補間値から算出（選択=1.0 / 非選択=0.55 を滑らかに）
        const float dist = std::min(std::abs(menuSelectLerp_ - static_cast<float>(i)), 1.0f);
        const float highlight = 0.55f + 0.45f * (1.0f - dist);
        sprites[i]->SetPosition(pos[i]);
        sprites[i]->SetAlpha(highlight * slide[i] * closeFade);
        sprites[i]->Draw();
    }

    if (menuCursor_)
    {
        // カーソルは補間値に応じて隣り合う項目の間を滑らかに移動する
        const float lerp = std::clamp(menuSelectLerp_, 0.0f, static_cast<float>(kMenuItemCount - 1));
        const int lower = std::min(static_cast<int>(lerp), kMenuItemCount - 2);
        const float frac = lerp - static_cast<float>(lower);
        const float cursorX = pos[lower].x + (pos[lower + 1].x - pos[lower].x) * frac;
        const float cursorY = pos[lower].y + (pos[lower + 1].y - pos[lower].y) * frac;
        menuCursor_->SetPosition({cursorX - kMenuCursorGap, cursorY});
        menuCursor_->SetAlpha(slide[kMenuItemCount - 1] * closeFade);
        menuCursor_->Draw();
    }
}

// 入力ヘルパ
bool TitleScene::PressStartInput()
{
    if (gamePad_->IsConnected())
    {
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_A);
    }
    return pInput_->TriggerKey(DIK_SPACE);
}

bool TitleScene::ConfirmInput()
{
    if (gamePad_->IsConnected())
    {
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_A);
    }
    return pInput_->TriggerKey(DIK_SPACE) || pInput_->TriggerKey(DIK_RETURN);
}

bool TitleScene::UpInput()
{
    if (gamePad_->IsConnected())
    {
        // スティックは倒した瞬間のみ反応させる（押しっぱなしで連続移動しない）
        const bool stickUp = gamePad_->GetLeftStickY() > 0.5f;
        const bool moved = gamePad_->IsTrigger(XINPUT_GAMEPAD_DPAD_UP) || (stickUp && !prevStickUp_);
        prevStickUp_ = stickUp;
        return moved;
    }
    return pInput_->TriggerKey(DIK_W) || pInput_->TriggerKey(DIK_UP);
}

bool TitleScene::DownInput()
{
    if (gamePad_->IsConnected())
    {
        // スティックは倒した瞬間のみ反応させる（押しっぱなしで連続移動しない）
        const bool stickDown = gamePad_->GetLeftStickY() < -0.5f;
        const bool moved = gamePad_->IsTrigger(XINPUT_GAMEPAD_DPAD_DOWN) || (stickDown && !prevStickDown_);
        prevStickDown_ = stickDown;
        return moved;
    }
    return pInput_->TriggerKey(DIK_S) || pInput_->TriggerKey(DIK_DOWN);
}

#define NOMINMAX
#include "TitleScene.h"
#include "2d/SpriteManager.h"
#include "2d/Text/TextRenderer.h"
#include "Utility/Scene/SceneManager.h"
#include "myMath.h"
#include <Frame.h>
#include <Graphics/Texture/TextureManager.h>
#include <algorithm>
#include <cmath>

REGISTER_SCENE("TITLE", TitleScene)

using namespace Hagine;
void TitleScene::Initialize() {
    /// ===================================================
    /// 初期化
    /// ===================================================
    BaseScene::Initialize();
    lightGroup_->LoadLightData("TitleScene");
    vp_.eulerRotation_ = {
        degreesToRadians(26.3f),
        degreesToRadians(-122.7f),
        degreesToRadians(0.0f)};
    vp_.Initialize("CurrentCamera");
    objectManager_->LoadAll("TitleScene");
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
    skyBox_ = SkyBox::GetInstance();
    skyBox_->Initialize("game/skybox.dds");

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
    drawSystem_->Register("TitleScene_3D", DrawLayer::kPreEffect, [this](const ViewProjection &vp) {
        skyBox_->Draw(vp);
        objectManager_->Draw(vp);
        titleUI_->Draw(vp_);
    });
    drawSystem_->Register("TitleScene_UI", DrawLayer::kPostEffect, [this](const ViewProjection &) {
        spriteManager_->DrawAll();
        DrawMenu();
    });
}

void TitleScene::Finalize() {
    /// ===================================================
    /// 終了処理
    /// ===================================================
    BaseScene::Finalize();
}

void TitleScene::Update() {
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
    if (time_ >= kMaxTime_ && !firstMove_) {
        vp_.EaseCameraMove(EasingType::InCubic, "TitleMovedCamera", 1.0f);
        firstMove_ = true;
    }

    // 進行フェーズごとの入力処理
    switch (titlePhase_) {
    case TitlePhase::WaitStart:
        // Press Start でメニューを開く
        if (time_ >= 3.0f && !vp_.GetIsCameraMove() && PressStartInput()) {
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

        // カーソル移動（上=チュートリアル / 下=トレーニング）
        if (UpInput()) {
            menuIndex_ = 0;
        }
        if (DownInput()) {
            menuIndex_ = 1;
        }

        // カーソル/ハイライトを選択項目へ滑らかに補間する
        const float selT = std::clamp(kMenuSelectLerp * dt, 0.0f, 1.0f);
        menuSelectLerp_ += (static_cast<float>(menuIndex_) - menuSelectLerp_) * selT;

        // 決定 → 退場アニメへ（即遷移せず、メニューをスムーズに閉じてから分岐する）
        if (ConfirmInput()) {
            titlePhase_ = TitlePhase::MenuClosing;
            menuCloseTimer_ = 0.0f;
        }
        break;
    }

    case TitlePhase::MenuClosing:
        // 退場アニメを進め、完了したら選択に応じて分岐する
        menuCloseTimer_ += Frame::DeltaTime();
        if (menuCloseTimer_ >= kMenuCloseDuration) {
            if (menuIndex_ == 0) {
                // チュートリアル: 従来のカメラ演出→トランジションなしで遷移
                vp_.EaseCameraMove(EasingType::InQuint, "EnemyEyeCamera", 1.0f);
                titleUI_->RequestStartCinematic();
                secondMove_ = true;
                titlePhase_ = TitlePhase::TutorialCinematic;
            } else {
                // トレーニング: カメラ演出なし、通常フェードで遷移
                sceneManager_->GetSceneTransition()->SetUseTransition(true);
                sceneManager_->NextSceneReservation("TRAINING");
                titlePhase_ = TitlePhase::Training;
            }
        }
        break;

    default:
        break;
    }

    // UIの更新
    titleUI_->Update();
}

void TitleScene::Draw() {
    // 描画は DrawSystem が管理
}

void TitleScene::DrawForOffScreen() {
    /// ===================================================
    /// オフスクリーン描画処理
    /// ===================================================
}

void TitleScene::AddSceneSetting() {
    /// ===================================================
    /// シーン設定（デバッグ）
    /// ===================================================
    debugCamera_->imgui();
    vp_.ShowDebugInfo();
}

void TitleScene::AddObjectSetting() {
    /// ===================================================
    /// オブジェクト設定（デバッグ）
    /// ===================================================
}

void TitleScene::AddParticleSetting() {
    /// ===================================================
    /// パーティクル設定（デバッグ）
    /// ===================================================
    DrawParticleEditorUI();
}

void TitleScene::CameraUpdate() {
    /// ===================================================
    /// カメラ更新
    /// ===================================================
    debugCamera_->Update();
}

void TitleScene::ChangeScene() {
    /// ===================================================
    /// シーン切り替え（チュートリアル演出の完了を待って遷移）
    /// ===================================================
    if (titlePhase_ == TitlePhase::TutorialCinematic &&
        !vp_.GetIsCameraMove() && titleUI_->GetIsFinish()) {
        sceneManager_->GetSceneTransition()->SetUseTransition(false);
        sceneManager_->NextSceneReservation("TUTORIAL");
    }
}

// 選択メニュー
void TitleScene::CreateMenuSprites() {
    auto keys = TextureManager::GetInstance()->GetAllFontKeys();
    if (keys.empty()) {
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
    menuTrainingPos_ = {kMenuX, kMenuTrainingY};
    make(menuTutorial_, "title_menu_tutorial", "チュートリアル", menuTutorialPos_);
    make(menuTraining_, "title_menu_training", "トレーニング", menuTrainingPos_);
    make(menuCursor_, "title_menu_cursor", "▶", {kMenuX - kMenuCursorGap, kMenuTutorialY});
}

void TitleScene::DrawMenu() {
    if (titlePhase_ != TitlePhase::Menu && titlePhase_ != TitlePhase::MenuClosing) {
        return;
    }

    const float dur = kMenuSlideDuration;

    // 各項目を画面右外から最終位置へ OutCubic でスライドイン（2項目目は少し遅らせる）
    // ApplyEasing はクランプしないので、時間は dur を超えないよう自前でクランプする
    // （超えると終点を越えて外挿し、メニューが画面外へ飛んでしまう）
    const float t1 = std::clamp(menuAnimTimer_, 0.0f, dur);
    const float t2 = std::clamp(menuAnimTimer_ - kMenuStagger, 0.0f, dur);

    const Vector2 tutorialStart = {kMenuStartX, menuTutorialPos_.y};
    const Vector2 trainingStart = {kMenuStartX, menuTrainingPos_.y};
    Vector2 tutorialPos = ApplyEasing(EasingType::OutCubic, tutorialStart, menuTutorialPos_, t1, dur);
    Vector2 trainingPos = ApplyEasing(EasingType::OutCubic, trainingStart, menuTrainingPos_, t2, dur);

    float tutorialSlide = std::clamp(t1 / dur, 0.0f, 1.0f);
    float trainingSlide = std::clamp(t2 / dur, 0.0f, 1.0f);

    // 決定後は右へずらしながらフェードアウト（ease-in）する
    float closeFade = 1.0f;
    if (titlePhase_ == TitlePhase::MenuClosing) {
        const float closeT = std::clamp(menuCloseTimer_ / kMenuCloseDuration, 0.0f, 1.0f);
        const float slide = kMenuCloseSlide * (closeT * closeT); // ease-in
        tutorialPos.x += slide;
        trainingPos.x += slide;
        closeFade = 1.0f - closeT;
    }

    // 選択ハイライトを補間値から算出（選択=1.0 / 非選択=0.55 を滑らかに）
    const float tutorialHi = 0.55f + 0.45f * (1.0f - menuSelectLerp_);
    const float trainingHi = 0.55f + 0.45f * menuSelectLerp_;

    if (menuTutorial_) {
        menuTutorial_->SetPosition(tutorialPos);
        menuTutorial_->SetAlpha(tutorialHi * tutorialSlide * closeFade);
        menuTutorial_->Draw();
    }
    if (menuTraining_) {
        menuTraining_->SetPosition(trainingPos);
        menuTraining_->SetAlpha(trainingHi * trainingSlide * closeFade);
        menuTraining_->Draw();
    }
    if (menuCursor_) {
        // カーソルは補間値に応じて2項目の間を滑らかに移動する
        const float cursorX = tutorialPos.x + (trainingPos.x - tutorialPos.x) * menuSelectLerp_;
        const float cursorY = tutorialPos.y + (trainingPos.y - tutorialPos.y) * menuSelectLerp_;
        menuCursor_->SetPosition({cursorX - kMenuCursorGap, cursorY});
        menuCursor_->SetAlpha(trainingSlide * closeFade);
        menuCursor_->Draw();
    }
}

// 入力ヘルパ
bool TitleScene::PressStartInput() {
    if (gamePad_->IsConnected()) {
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_A);
    }
    return input_->TriggerKey(DIK_SPACE);
}

bool TitleScene::ConfirmInput() {
    if (gamePad_->IsConnected()) {
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_A);
    }
    return input_->TriggerKey(DIK_SPACE) || input_->TriggerKey(DIK_RETURN);
}

bool TitleScene::UpInput() {
    if (gamePad_->IsConnected()) {
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_DPAD_UP) || gamePad_->GetLeftStickY() > 0.5f;
    }
    return input_->TriggerKey(DIK_W) || input_->TriggerKey(DIK_UP);
}

bool TitleScene::DownInput() {
    if (gamePad_->IsConnected()) {
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_DPAD_DOWN) || gamePad_->GetLeftStickY() < -0.5f;
    }
    return input_->TriggerKey(DIK_S) || input_->TriggerKey(DIK_DOWN);
}

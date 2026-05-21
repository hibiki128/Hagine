#define NOMINMAX
#include "TutorialUI.h"
#include "../../System/Tutorial/TutorialSystem.h"
#include "Data/DataHandler.h"
#include "Engine/2d/SpriteManager.h"
#include "Engine/2d/Text/TextRenderer.h"
#include <Graphics/Texture/TextureManager.h>
#include <algorithm>
#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG

// ============================================================
//  Initialize
// ============================================================
void TutorialUI::Initialize(TutorialSystem *system, const std::string &okFontKey) {
    system_ = system;

    // 内部状態の初期化
    displayedProgress_ = 0.0f;
    subMessageVisible_ = false;
    subMessageTimer_ = 0.0f;
    transitionState_ = UITransitionState::Idle;
    fadeTimer_ = 0.0f;
    okAlpha_ = 0.0f;
    barAlpha_ = 1.0f;
    frameAlpha_ = 1.0f;
    postCompleteTimer_ = 0.0f;
    isFinished_ = false;

    // 設定ロード
    dataHandler_ = std::make_unique<DataHandler>("SpriteDatas", "Tutorialmeter");
    LoadMeterSettings();

    // 進捗メータースプライトの初期化
    barSprite_.Initialize(
        "debug/white1x1.png",
        barPosition_,
        {1.0f, 1.0f, 0.0f, 1.0f});

    frameSprite_.Initialize(
        "debug/white1x1.png",
        {barPosition_.x - borderThickness_, barPosition_.y - borderThickness_},
        {0.0f, 0.0f, 0.0f, 1.0f});

    SkipButtonSprite_.Initialize(
        "UI/skip.png",
        skipButtonPosition_,
        {1.0f, 1.0f, 1.0f, 1.0f},
        {0.5f, 0.5f});

    SkipButtonSprite_.SetSize(skipButtonSize_);

    UpdateMeterSprites();

    // 初期ステップのスプライトをロード
    LoadStepSprites(system_->GetCurrentStep());

    // OK! スプライトの生成
    InitializeOKSprite(okFontKey);
}

void TutorialUI::InitializeOKSprite(const std::string &fontKey) {
    std::string resolvedKey = fontKey;
    if (resolvedKey.empty()) {
        auto keys = TextureManager::GetInstance()->GetAllFontKeys();
        if (!keys.empty()) {
            resolvedKey = keys[0];
        }
    }

    if (resolvedKey.empty()) {
        return;
    }

    // "OK!" テクスチャを生成
    TextRenderer::GetInstance()->CreateTextSprite(
        "Tutorial_OK",
        "OK!",
        resolvedKey,
        {0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f, 1.0f},
        true,
        4.5f,
        {0.0f, 0.0f, 0.0f, 1.0f});

    SpriteManager::GetInstance()->UnregisterSprite("Tutorial_OK");

    okSprite_.Initialize(
        "Text/Tutorial_OK.png",
        okPosition_,
        {1.0f, 1.0f, 0.0f, 0.0f});

    okSize_ = okSprite_.GetSize();
    okSpriteReady_ = true;
}

void TutorialUI::Update(float dt) {
    if (!system_) {
        return;
    }

    UpdateTransition(dt);
    UpdateProgressBar(dt);
    UpdateSubMessage();
    UpdateMeterSprites();
}

void TutorialUI::UpdateTransition(float dt) {
    switch (transitionState_) {

    case UITransitionState::Idle:
        if (system_->IsStepJustChanged()) {
            transitionState_ = UITransitionState::FadingOut;
            fadeTimer_ = 0.0f;
        }
        break;

    case UITransitionState::FadingOut: {
        fadeTimer_ += dt;
        const float t = std::clamp(fadeTimer_ / fadeOutDuration_, 0.0f, 1.0f);

        ApplyAlphaToAllManagedSprites(1.0f - t);
        okAlpha_ = t;

        if (fadeTimer_ >= fadeOutDuration_) {
            LoadStepSprites(system_->GetCurrentStep());
            ApplyAlphaToAllManagedSprites(0.0f);
            displayedProgress_ = 0.0f;

            transitionState_ = UITransitionState::FadingIn;
            fadeTimer_ = 0.0f;
        }
        break;
    }

    case UITransitionState::FadingIn: {
        fadeTimer_ += dt;
        const float t = std::clamp(fadeTimer_ / fadeInDuration_, 0.0f, 1.0f);

        ApplyAlphaToAllManagedSprites(t);

        // Complete ステップでは OK! を永続表示、それ以外はフェードアウト
        if (system_->GetCurrentStep() == TutorialStep::Complete) {
            okAlpha_ = 1.0f;
        } else {
            okAlpha_ = 1.0f - t;
        }

        if (fadeTimer_ >= fadeInDuration_) {
            ApplyAlphaToAllManagedSprites(1.0f);

            if (system_->GetCurrentStep() != TutorialStep::Complete) {
                okAlpha_ = 0.0f;
                transitionState_ = UITransitionState::Idle;
            } else {
                okAlpha_ = 1.0f;
                barAlpha_ = 1.0f;
                frameAlpha_ = 1.0f;
                transitionState_ = UITransitionState::CompleteFadeOut;
                fadeTimer_ = 0.0f;
            }
        }
        break;
    }

    case UITransitionState::CompleteFadeOut: {
        fadeTimer_ += dt;
        const float t = std::clamp(fadeTimer_ / completeFadeOutDuration_, 0.0f, 1.0f);
        const float alpha = 1.0f - t;

        ApplyAlphaToAllManagedSprites(alpha);
        okAlpha_ = alpha;
        barAlpha_ = alpha;
        frameAlpha_ = alpha;

        if (fadeTimer_ >= completeFadeOutDuration_) {
            ApplyAlphaToAllManagedSprites(0.0f);
            okAlpha_ = 0.0f;
            barAlpha_ = 0.0f;
            frameAlpha_ = 0.0f;

            transitionState_ = UITransitionState::CompleteDone;
            fadeTimer_ = 0.0f;
            postCompleteTimer_ = 0.0f;
        }
        break;
    }

    case UITransitionState::CompleteDone:
        if (!isFinished_) {
            postCompleteTimer_ += dt;
            if (postCompleteTimer_ >= postCompleteDelay_) {
                isFinished_ = true;
            }
        }
        break;
    }
}

void TutorialUI::UpdateProgressBar(float dt) {
    float target = 0.0f;

    switch (transitionState_) {
    case UITransitionState::Idle:
        target = system_->GetProgress();
        break;
    case UITransitionState::FadingOut:
        target = 1.0f;
        break;
    case UITransitionState::FadingIn:
        target = system_->GetProgress();
        break;
    case UITransitionState::CompleteFadeOut:
    case UITransitionState::CompleteDone:
        target = displayedProgress_;
        break;
    }

    displayedProgress_ += (target - displayedProgress_) * progressLerpSpeed_ * dt;
    displayedProgress_ = std::clamp(displayedProgress_, 0.0f, 1.0f);
}

void TutorialUI::UpdateSubMessage() {
    const bool shouldShow = system_->IsShowingReturnToAirMessage();
    if (shouldShow != subMessageVisible_) {
        subMessageVisible_ = shouldShow;
    }
}

void TutorialUI::UpdateMeterSprites() {
    const float barWidth = std::max(1.0f, barMaxWidth_ * displayedProgress_);
    barSprite_.SetPosition(barPosition_);
    barSprite_.SetSize({barWidth, barHeight_});

    frameSprite_.SetPosition({barPosition_.x - borderThickness_,
                              barPosition_.y - borderThickness_});
    frameSprite_.SetSize({barMaxWidth_ + borderThickness_ * 2.0f,
                          barHeight_ + borderThickness_ * 2.0f});
}

void TutorialUI::ApplyAlphaToAllManagedSprites(float alpha) {
    for (SpriteData *sd : SpriteManager::GetInstance()->GetAllSprites()) {
        if (sd && sd->sprite) {
            sd->sprite->SetAlpha(alpha);
        }
    }
}

void TutorialUI::Draw() {
    // 枠 -> バー の順で描画
    frameSprite_.SetAlpha(frameAlpha_);
    frameSprite_.Draw();

    barSprite_.SetAlpha(barAlpha_);
    barSprite_.Draw();

    SkipButtonSprite_.SetAlpha(1.0f);
    SkipButtonSprite_.Draw();

    // OK! スプライトの描画
    if (okSpriteReady_ && okAlpha_ > 0.001f) {
        okSprite_.SetAlpha(okAlpha_);
        okSprite_.SetPosition(okPosition_);
        okSprite_.SetRotation(okRotation_);
        okSprite_.SetSize(okSize_);
        okSprite_.Draw();
    }
}

void TutorialUI::Finalize() {
    dataHandler_.reset();
    system_ = nullptr;
    okSpriteReady_ = false;
}

// ============================================================
//  DrawImGui
// ============================================================
void TutorialUI::DrawImGui() {
#ifdef _DEBUG
    ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f), ImGuiCond_Once);
    if (!ImGui::Begin("チュートリアル UI 設定")) {
        ImGui::End();
        return;
    }

    // ── メーター設定 ─────────────────────────────────────────
    ImGui::SeparatorText("進捗メーター  位置 / サイズ");

    float pos[2] = {barPosition_.x, barPosition_.y};
    if (ImGui::DragFloat2("位置##meter", pos, 1.0f)) {
        barPosition_ = {pos[0], pos[1]};
    }
    ImGui::DragFloat("高さ", &barHeight_, 1.0f, 4.0f, 200.0f, "%.1f px");
    ImGui::DragFloat("最大幅", &barMaxWidth_, 1.0f, 50.0f, 1280.0f, "%.1f px");
    ImGui::DragFloat("枠の太さ", &borderThickness_, 0.5f, 1.0f, 20.0f, "%.1f px");

    // ── トランジション設定 ───────────────────────────────────
    ImGui::SeparatorText("フェードトランジション");

    ImGui::DragFloat("フェードアウト秒数", &fadeOutDuration_, 0.01f, 0.05f, 2.0f, "%.2f s");
    ImGui::DragFloat("フェードイン秒数", &fadeInDuration_, 0.01f, 0.05f, 2.0f, "%.2f s");
    ImGui::DragFloat("バー補間速度", &progressLerpSpeed_, 0.1f, 0.5f, 20.0f, "%.1f");

    // ── 完了フェードアウト設定 ───────────────────────────────
    ImGui::SeparatorText("完了時フェードアウト");

    ImGui::DragFloat("完了フェードアウト秒数", &completeFadeOutDuration_, 0.05f, 0.1f, 5.0f, "%.2f s");
    ImGui::DragFloat("完了フラグ待機秒数", &postCompleteDelay_, 0.1f, 0.0f, 10.0f, "%.1f s");

    // ── OK! スプライト設定 ───────────────────────────────────
    ImGui::SeparatorText("OK! スプライト");

    float okPos[2] = {okPosition_.x, okPosition_.y};
    if (ImGui::DragFloat2("位置##ok", okPos, 1.0f)) {
        okPosition_ = {okPos[0], okPos[1]};
    }
    ImGui::DragFloat("回転 (rad)##ok", &okRotation_, 0.005f, -3.14159f, 3.14159f, "%.3f");
    float okSz[2] = {okSize_.x, okSize_.y};
    if (ImGui::DragFloat2("サイズ##ok", okSz, 1.0f, 1.0f, 2000.0f)) {
        okSize_ = {okSz[0], okSz[1]};
    }
    ImGui::Text("表示アルファ : %.3f", okAlpha_);
    ImGui::Text("初期化済み   : %s", okSpriteReady_ ? "YES" : "NO");

    // ── デバッグ情報 ─────────────────────────────────────────
    ImGui::SeparatorText("デバッグ");

    static const char *kStateNames[] = {
        "Idle", "FadingOut", "FadingIn", "CompleteFadeOut", "CompleteDone"};
    ImGui::Text("トランジション状態 : %s", kStateNames[static_cast<int>(transitionState_)]);
    ImGui::Text("フェードタイマー   : %.3f s", fadeTimer_);
    ImGui::Text("表示進捗           : %.3f", displayedProgress_);
    ImGui::ProgressBar(displayedProgress_, ImVec2(-1.0f, 0.0f));
    ImGui::Text("バーアルファ       : %.3f", barAlpha_);
    ImGui::Text("完了後タイマー     : %.3f s / %.1f s", postCompleteTimer_, postCompleteDelay_);
    ImGui::Text("IsFinished         : %s", isFinished_ ? "TRUE" : "false");

    // ── 設定の保存 ───────────────────────────────────────────
    ImGui::SeparatorText("設定の保存");
    if (ImGui::Button("保存 (DataHandler::Flush)")) {
        SaveMeterSettings();
    }

    ImGui::End();
#endif // _DEBUG
}

// ============================================================
//  GetFolderNameForStep
// ============================================================
const char *TutorialUI::GetFolderNameForStep(TutorialStep step) const {
    if (step == TutorialStep::Complete) {
        return "TutorialFinish";
    }

    static const char *kFolderNames[] = {
        "TutorialStep1",  // Move
        "TutorialStep2",  // Jump
        "TutorialStep3",  // FlyTransition
        "TutorialStep4",  // Ascend
        "TutorialStep5",  // Descend
        "TutorialStep6",  // AirMove
        "TutorialStep7",  // Dash
        "TutorialStep8",  // Rush
        "TutorialStep9",  // Landing
        "TutorialStep10", // MeleeAttack
        "TutorialStep11", // RangedAttack
        "TutorialStep12", // ChargeAttack
        "TutorialStep13", // EnergyCharge
        "TutorialStep14", // SpecialAttack
    };

    const int index = static_cast<int>(step);
    const int validCount = static_cast<int>(TutorialStep::StepCount) - 1;

    if (index < 0 || index >= validCount) {
        return nullptr;
    }
    return kFolderNames[index];
}

// ============================================================
//  LoadStepSprites
// ============================================================
void TutorialUI::LoadStepSprites(TutorialStep step) {
    const char *folder = GetFolderNameForStep(step);
    if (!folder) {
        return;
    }

    SpriteManager *sm = SpriteManager::GetInstance();
    sm->Clear();
    sm->SetSaveFolder(folder);
    sm->LoadAllSprites();
}

// ============================================================
//  LoadMeterSettings
// ============================================================
void TutorialUI::LoadMeterSettings() {
    if (!dataHandler_) {
        return;
    }

    barPosition_ = dataHandler_->Load("bar_position", barPosition_);
    barHeight_ = dataHandler_->Load("bar_height", barHeight_);
    barMaxWidth_ = dataHandler_->Load("bar_max_width", barMaxWidth_);
    borderThickness_ = dataHandler_->Load("border_thickness", borderThickness_);
    okPosition_ = dataHandler_->Load("ok_position", okPosition_);
    okRotation_ = dataHandler_->Load("ok_rotation", okRotation_);
    okSize_ = dataHandler_->Load("ok_size", okSize_);
    fadeOutDuration_ = dataHandler_->Load("fade_out_duration", fadeOutDuration_);
    fadeInDuration_ = dataHandler_->Load("fade_in_duration", fadeInDuration_);
    progressLerpSpeed_ = dataHandler_->Load("progress_lerp_speed", progressLerpSpeed_);
    completeFadeOutDuration_ = dataHandler_->Load("complete_fade_out_duration", completeFadeOutDuration_);
    postCompleteDelay_ = dataHandler_->Load("post_complete_delay", postCompleteDelay_);
}

// ============================================================
//  SaveMeterSettings
// ============================================================
void TutorialUI::SaveMeterSettings() {
    if (!dataHandler_) {
        return;
    }

    dataHandler_->Save("bar_position", barPosition_);
    dataHandler_->Save("bar_height", barHeight_);
    dataHandler_->Save("bar_max_width", barMaxWidth_);
    dataHandler_->Save("border_thickness", borderThickness_);
    dataHandler_->Save("ok_position", okPosition_);
    dataHandler_->Save("ok_rotation", okRotation_);
    dataHandler_->Save("ok_size", okSize_);
    dataHandler_->Save("fade_out_duration", fadeOutDuration_);
    dataHandler_->Save("fade_in_duration", fadeInDuration_);
    dataHandler_->Save("progress_lerp_speed", progressLerpSpeed_);
    dataHandler_->Save("complete_fade_out_duration", completeFadeOutDuration_);
    dataHandler_->Save("post_complete_delay", postCompleteDelay_);
    dataHandler_->Flush();
}
#include "TutorialUI.h"
#include <algorithm>
#include"../../System/Tutorial/TutorialSystem.h"

// ============================================================
void TutorialUI::Initialize(TutorialSystem *system) {
    system_ = system;
    displayedProgress_ = 0.0f;
    subMessageVisible_ = false;
    subMessageTimer_ = 0.0f;

    // ──────────────────────────────────────────
    // TODO: スプライトの生成・初期位置設定
    // ──────────────────────────────────────────
    // 例（Sprite API が決まったら差し替えてください）:
    //
    // instructionBgSprite_ = new Sprite();
    // instructionBgSprite_->Initialize("tutorial/instruction_bg.png");
    // instructionBgSprite_->SetPosition({50.0f, 600.0f});
    //
    // progressBarBgSprite_ = new Sprite();
    // progressBarBgSprite_->Initialize("tutorial/bar_bg.png");
    // progressBarBgSprite_->SetPosition({50.0f, 660.0f});
    //
    // progressBarFillSprite_ = new Sprite();
    // progressBarFillSprite_->Initialize("tutorial/bar_fill.png");
    // progressBarFillSprite_->SetPosition({50.0f, 660.0f});
    //
    // subMessageSprite_ = new Sprite();
    // subMessageSprite_->Initialize("tutorial/sub_message.png");
    // subMessageSprite_->SetPosition({400.0f, 550.0f});
}

// ============================================================
void TutorialUI::Update(float dt) {
    if (!system_) {
        return;
    }

    UpdateProgressBar(dt);
    UpdateInstructionDisplay();
    UpdateSubMessage();
}

// ============================================================
void TutorialUI::Draw() {
    if (!system_) {
        return;
    }

    // ──────────────────────────────────────────
    // TODO: スプライトの描画
    // ──────────────────────────────────────────
    // 例（Sprite API が決まったら差し替えてください）:
    //
    // instructionBgSprite_->Draw();
    // buttonIconSprite_->Draw();
    // progressBarBgSprite_->Draw();
    // progressBarFillSprite_->Draw();
    //
    // if (subMessageVisible_) {
    //     subMessageSprite_->Draw();
    // }
}

// ============================================================
void TutorialUI::Finalize() {
    // ──────────────────────────────────────────
    // TODO: スプライトの破棄
    // ──────────────────────────────────────────
    system_ = nullptr;
}

// ============================================================
//  UpdateProgressBar  進行度バーをスムーズに補間して更新
// ============================================================
void TutorialUI::UpdateProgressBar(float dt) {
    float targetProgress = system_->GetProgress();

    // Lerp で滑らかに追従させる
    displayedProgress_ += (targetProgress - displayedProgress_) * progressLerpSpeed_ * dt;
    displayedProgress_ = std::clamp(displayedProgress_, 0.0f, 1.0f);

    // ──────────────────────────────────────────
    // TODO: バー塗り部分の幅に反映する
    // ──────────────────────────────────────────
    // 例:
    // constexpr float kBarMaxWidth = 400.0f;
    // constexpr float kBarHeight   = 20.0f;
    // progressBarFillSprite_->SetSize({kBarMaxWidth * displayedProgress_, kBarHeight});
}

// ============================================================
//  UpdateInstructionDisplay  ステップ切替時にテキスト・アイコンを更新
// ============================================================
void TutorialUI::UpdateInstructionDisplay() {
    if (!system_->IsStepJustChanged()) {
        return;
    }

    // ステップが切り替わった瞬間のみ更新する
    const char *text = system_->GetInstructionText();
    TutorialStep step = system_->GetCurrentStep();

    // ──────────────────────────────────────────
    // TODO: テキストおよびアイコン画像の更新
    // ──────────────────────────────────────────
    // 例:
    // instructionTextSprite_->SetText(text);
    //
    // ステップごとにアイコンを切り替える例:
    // const char* iconPath = GetIconPathForStep(step);
    // buttonIconSprite_->SetTexture(iconPath);
    //
    // ステップ達成エフェクトのリセット（任意）:
    // stepClearEffectTimer_ = 0.3f;
    (void)text;
    (void)step;
}

// ============================================================
//  UpdateSubMessage  補足メッセージの表示/非表示を制御
// ============================================================
void TutorialUI::UpdateSubMessage() {
    bool shouldShow = system_->IsShowingReturnToAirMessage();

    if (shouldShow != subMessageVisible_) {
        subMessageVisible_ = shouldShow;

        // ──────────────────────────────────────────
        // TODO: 補足メッセージスプライトの表示/非表示切替
        // ──────────────────────────────────────────
        // 例:
        // subMessageSprite_->SetVisible(subMessageVisible_);
        // if (subMessageVisible_) {
        //     const char* subText = system_->GetSubText();
        //     subMessageSprite_->SetText(subText);
        // }
    }

    // Dash のサブフェーズ切替時も指示テキストを即時更新する
    // （IsStepJustChanged は立たないため、ここで毎フレーム確認）
    if (system_->GetCurrentStep() == TutorialStep::Dash) {
        const char *dashText = system_->GetInstructionText();
        // ──────────────────────────────────────────
        // TODO: ダッシュ指示テキストの更新
        // ──────────────────────────────────────────
        // instructionTextSprite_->SetText(dashText);
        (void)dashText;
    }
}
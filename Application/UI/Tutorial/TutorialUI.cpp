#include "TutorialUI.h"
#include "../../System/Tutorial/TutorialSystem.h"
#include <algorithm>
#include"Engine/2d/SpriteManager.h"

// ============================================================
void TutorialUI::Initialize(TutorialSystem *system) {
    system_ = system;
    displayedProgress_ = 0.0f;
    subMessageVisible_ = false;
    subMessageTimer_ = 0.0f;

    // 初期ステップ（Move）のスプライトをロードする
    LoadStepSprites(system_->GetCurrentStep());
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
    // 描画は SpriteManager 側で一括処理するため、ここでは何もしない
}

// ============================================================
void TutorialUI::Finalize() {
    system_ = nullptr;
}

// ============================================================
//  GetFolderNameForStep  ステップ番号をフォルダ名文字列に変換する
// ============================================================
const char *TutorialUI::GetFolderNameForStep(TutorialStep step) const {
    // Complete ステップはチュートリアル完了用の専用フォルダを使う
    if (step == TutorialStep::Complete) {
        return "TutorialFinish";
    }

    // Move(0) 〜 SpecialAttack(13) を TutorialStep1 〜 TutorialStep14 に対応させる
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

    int index = static_cast<int>(step);
    int validCount = static_cast<int>(TutorialStep::StepCount) - 1; // Complete の分を除く

    if (index < 0 || index >= validCount) {
        return nullptr;
    }

    return kFolderNames[index];
}

// ============================================================
//  LoadStepSprites  指定ステップに対応するスプライト群を SpriteManager でロードする
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
//  UpdateProgressBar  進行度バーをスムーズに補間して更新
// ============================================================
void TutorialUI::UpdateProgressBar(float dt) {
    float targetProgress = system_->GetProgress();

    // Lerp で滑らかに追従させる
    displayedProgress_ += (targetProgress - displayedProgress_) * progressLerpSpeed_ * dt;
    displayedProgress_ = std::clamp(displayedProgress_, 0.0f, 1.0f);
}

// ============================================================
//  UpdateInstructionDisplay  ステップ切替時にスプライトをロードし直す
// ============================================================
void TutorialUI::UpdateInstructionDisplay() {
    // ステップが切り替わった瞬間のみ処理する
    if (!system_->IsStepJustChanged()) {
        return;
    }

    LoadStepSprites(system_->GetCurrentStep());
}

// ============================================================
//  UpdateSubMessage  補足メッセージの表示/非表示を制御
// ============================================================
void TutorialUI::UpdateSubMessage() {
    bool shouldShow = system_->IsShowingReturnToAirMessage();

    // 表示状態が変化したときのみ更新する
    if (shouldShow != subMessageVisible_) {
        subMessageVisible_ = shouldShow;
    }
}
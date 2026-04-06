#include "TutorialSystem.h"
#include "Application/GameObject/Player/Player.h"
#include <Input.h>
#include <algorithm>
#include <cassert>
#include <implot.h>

// ============================================================
//  ステップ設定テーブル
//  TutorialStep の順序と 1:1 対応させること
// ============================================================
const TutorialStepConfig TutorialSystem::kConfigs[static_cast<int>(TutorialStep::StepCount)] = {
    //  instructionText                                                  subText                                             reqTime  reqCount  needsEnemy
    {"[WASD / 左スティック] で移動しよう", nullptr, 3.0f, 0, false},                                      // Move
    {"[SPACE / Aボタン] でジャンプしよう", nullptr, 0.0f, 1, false},                                      // Jump
    {"ジャンプ後に [SPACE / RBボタン] でホバリング！", nullptr, 0.0f, 1, false},                          // FlyTransition
    {"[SPACE保持 / RT] で上昇しよう", nullptr, 2.0f, 0, false},                                           // Ascend
    {"[LSHIFT保持 / LT] で下降しよう", "着地してしまったら、もう一度空中状態になろう！", 2.0f, 0, false}, // Descend
    {"[WASD / 左スティック] で空中を移動しよう", nullptr, 3.0f, 0, false},                                // AirMove
    {nullptr /* DashSubPhase に応じて GetInstructionText() で切替 */, nullptr, 0.0f, 1, false},           // Dash
    {"[チャージ中に 移動入力 / Aボタン+スティック] でダミーへ急接近！", nullptr, 0.0f, 1, true},          // Rush
    {"[LSHIFT×2 / LT長押し] で空中状態を解除して着地しよう", nullptr, 0.0f, 1, false},                    // Landing
    {"[K / Bボタン] でダミーを近接攻撃！（3回）", nullptr, 0.0f, 3, true},                                // MeleeAttack
    {"[J / Yボタン] で通常射撃！（3回）", nullptr, 0.0f, 3, true},                                        // RangedAttack
    {"[J長押し → 離す / Y長押し → 離す] でチャージ攻撃！", nullptr, 0.0f, 1, true},                       // ChargeAttack
    {"[C保持 / LT保持] でエネルギーをチャージしよう！（3秒）", nullptr, 3.0f, 0, false},                  // EnergyCharge
    {"エネルギーが溜まったら [G / Yボタン] で必殺技！", nullptr, 0.0f, 1, false},                         // SpecialAttack
    {"チュートリアル完了！", nullptr, 0.0f, 0, false},                                                    // Complete
};

// ============================================================
void TutorialSystem::Initialize(Player *player) {
    assert(player && "TutorialSystem: player が nullptr です");
    player_ = player;
    input_ = Input::GetInstance();
    currentStep_ = TutorialStep::Move;
    prevStateName_ = player_->GetCurrentStateName();

    ResetStepState();
}

// ============================================================
void TutorialSystem::Update(float dt) {
    stepJustChanged_ = false;

    if (currentStep_ == TutorialStep::Complete) {
        return;
    }

    UpdateCurrentStep(dt);

    // フレーム末に状態を保存（次フレームの「前フレーム情報」として使う）
    prevStateName_ = player_->GetCurrentStateName();
    wasGroundedLastFrame_ = player_->GetIsGrounded();
}

// ============================================================
void TutorialSystem::Finalize() {
    player_ = nullptr;
    input_ = nullptr;
}

// ============================================================
//  GetInstructionText
//  ダッシュのサブフェーズだけ動的に切り替える
// ============================================================
const char *TutorialSystem::GetInstructionText() const {
    if (currentStep_ == TutorialStep::Dash) {
        switch (dashSubPhase_) {
        case DashSubPhase::WaitForCharge:
            return "[C / LT] を長押しして「チャージ状態」になろう";
        case DashSubPhase::WaitForDash:
            return "チャージ中に [WASD / Aボタン+スティック] でダッシュ！";
        }
    }
    return kConfigs[static_cast<int>(currentStep_)].instructionText;
}

// ============================================================
const char *TutorialSystem::GetSubText() const {
    // 着地補正メッセージを最優先で返す
    if (showReturnToAirMessage_) {
        return kConfigs[static_cast<int>(TutorialStep::Descend)].subText;
    }
    return kConfigs[static_cast<int>(currentStep_)].subText;
}

// ============================================================
//  AdvanceStep  次のステップへ進む
// ============================================================
void TutorialSystem::AdvanceStep() {
    const bool currNeedsEnemy = kConfigs[static_cast<int>(currentStep_)].needsEnemy;
    const int nextIdx = static_cast<int>(currentStep_) + 1;

    if (nextIdx < static_cast<int>(TutorialStep::StepCount)) {
        const bool nextNeedsEnemy = kConfigs[nextIdx].needsEnemy;

        // 境界をまたぐときだけリクエストを発行する
        if (!currNeedsEnemy && nextNeedsEnemy) {
            RequestSpawnEnemy();
        } else if (currNeedsEnemy && !nextNeedsEnemy) {
            RequestDespawnEnemy();
        }

        currentStep_ = static_cast<TutorialStep>(nextIdx);
    } else {
        currentStep_ = TutorialStep::Complete;
    }

    ResetStepState();
    stepJustChanged_ = true;
}

// ============================================================
void TutorialSystem::ResetStepState() {
    timer_ = 0.0f;
    count_ = 0;
    progress_ = 0.0f;
    showReturnToAirMessage_ = false;
    dashSubPhase_ = DashSubPhase::WaitForCharge;
    chargeHoldTimer_ = 0.0f;
    chargeInputActive_ = false;
}

// ============================================================
void TutorialSystem::RequestSpawnEnemy() { spawnEnemyRequested_ = true; }
void TutorialSystem::RequestDespawnEnemy() { despawnEnemyRequested_ = true; }

// ============================================================
//  UpdateCurrentStep  各ステップの進行チェックと進捗更新
// ============================================================
void TutorialSystem::UpdateCurrentStep(float dt) {
    bool conditionMet = false;

    switch (currentStep_) {
    case TutorialStep::Move: // ステップ1: 地上移動
        conditionMet = CheckMove();
        break;
    case TutorialStep::Jump: // ステップ2: ジャンプ
        conditionMet = CheckJump();
        break;
    case TutorialStep::FlyTransition: // ステップ3: 空中 → ホバリング移行
        conditionMet = CheckFlyTransition();
        break;
    case TutorialStep::Ascend: // ステップ4: 上昇
        conditionMet = CheckAscend();
        break;
    case TutorialStep::Descend: // ステップ5: 下降
        conditionMet = CheckDescend();
        break;
    case TutorialStep::AirMove: // ステップ6: 空中移動
        conditionMet = CheckAirMove();
        break;
    case TutorialStep::Dash: // ステップ7: ダッシュ
        conditionMet = CheckDash();
        break;
    case TutorialStep::Rush: // ステップ8: 急接近
        conditionMet = CheckRush();
        break;
    case TutorialStep::Landing: // ステップ9: 着地
        conditionMet = CheckLanding();
        break;
    case TutorialStep::MeleeAttack: // ステップ10: 近接攻撃
        conditionMet = CheckMeleeAttack();
        break;
    case TutorialStep::RangedAttack: // ステップ11: 遠距離攻撃
        conditionMet = CheckRangedAttack();
        break;
    case TutorialStep::ChargeAttack: // ステップ12: チャージ攻撃
        conditionMet = CheckChargeAttack(dt);
        break;
    case TutorialStep::EnergyCharge: // ステップ13: エネルギーチャージ
        conditionMet = CheckEnergyCharge();
        break;
    case TutorialStep::SpecialAttack: // ステップ14: 必殺技
        conditionMet = CheckSpecialAttack();
        break;
    default:
        return;
    }

    const TutorialStepConfig &cfg = kConfigs[static_cast<int>(currentStep_)];

    if (cfg.requiredTime > 0.0f) {
        // ── 時間方式: 条件を満たしている間だけ timer を蓄積 ──
        if (conditionMet) {
            timer_ += dt;
        }
        progress_ = std::min(timer_ / cfg.requiredTime, 1.0f);
        if (timer_ >= cfg.requiredTime) {
            AdvanceStep();
        }
    } else {
        // ── 回数方式: 達成した瞬間（トリガー）に count++ ──
        if (conditionMet) {
            count_++;
        }
        progress_ = std::min(static_cast<float>(count_) / static_cast<float>(cfg.requiredCount), 1.0f);
        if (count_ >= cfg.requiredCount) {
            AdvanceStep();
        }
    }
}

// ============================================================
//  ステップ個別チェック
// ============================================================

bool TutorialSystem::CheckMove() {
    // 地上での移動入力を 3 秒間継続
    const std::string &state = player_->GetCurrentStateName();
    bool isOnGround = (state == "Idle" || state == "Move");
    return isOnGround && IsMoveInput();
}

bool TutorialSystem::CheckJump() {
    // 地上でジャンプトリガー（1回）
    const std::string &state = player_->GetCurrentStateName();
    bool isOnGround = (state == "Idle" || state == "Move");
    return isOnGround && IsJumpTrigger();
}

bool TutorialSystem::CheckFlyTransition() {
    // "FlyIdle" 状態に初めて入った瞬間を検出
    // 前フレームと比較して遷移した瞬間のみ true
    const std::string &current = player_->GetCurrentStateName();
    return (current == "FlyIdle" && prevStateName_ != "FlyIdle");
}

bool TutorialSystem::CheckAscend() {
    // 飛行中かつ上昇入力を 2 秒間継続
    const std::string &state = player_->GetCurrentStateName();
    bool inFly = (state == "FlyIdle" || state == "FlyMove");
    return inFly && IsAscendInput();
}

bool TutorialSystem::CheckDescend() {
    const std::string &state = player_->GetCurrentStateName();
    bool inFly = (state == "FlyIdle" || state == "FlyMove");
    bool isGrounded = player_->GetIsGrounded();

    // ── 着地補正ロジック ──
    // 飛行状態から着地した瞬間を検出
    if (!inFly && isGrounded && !wasGroundedLastFrame_) {
        showReturnToAirMessage_ = true;
    }

    if (showReturnToAirMessage_) {
        // FlyIdle/FlyMove に戻るまで待機、timer は進めない
        if (inFly) {
            showReturnToAirMessage_ = false;
        }
        return false;
    }

    // 正常判定: 飛行中かつ下降入力
    return inFly && IsDescendInput();
}

bool TutorialSystem::CheckAirMove() {
    // 飛行中の移動入力を 3 秒間継続
    const std::string &state = player_->GetCurrentStateName();
    bool inFly = (state == "FlyIdle" || state == "FlyMove");
    return inFly && IsMoveInput();
}

bool TutorialSystem::CheckDash() {
    // ── 2 段階サブフェーズ ──
    // 【補正理由】ダッシュはチャージ状態中の派生アクションだが、
    // チャージ操作を教えるのはステップ13。ここでは Dash ステップ内で
    // チャージ→移動 の流れを簡易的に体験させる。
    const std::string &state = player_->GetCurrentStateName();

    switch (dashSubPhase_) {
    case DashSubPhase::WaitForCharge:
        // EnergyCharge 状態に入ったらフェーズ 2 へ
        if (state == "EnergyCharge") {
            dashSubPhase_ = DashSubPhase::WaitForDash;
        }
        return false; // まだカウントしない

    case DashSubPhase::WaitForDash:
        // チャージ中に移動入力 → ダッシュ開始と判断
        if (state == "EnergyCharge" && IsMoveInput()) {
            return true; // count++
        }
        // チャージが解除されたらフェーズ 1 に戻す
        if (state != "EnergyCharge") {
            dashSubPhase_ = DashSubPhase::WaitForCharge;
        }
        return false;
    }
    return false;
}

bool TutorialSystem::CheckRush() {
    // "Rush" 状態への遷移を検出（前フレームとの比較）
    const std::string &current = player_->GetCurrentStateName();
    return (current == "Rush" && prevStateName_ != "Rush");
}

bool TutorialSystem::CheckLanding() {
    // 空中状態解除の遷移フロー: FlyState → "Air" → 着地（Idle/Move）
    // LSHIFT×2 / LT長押し で FlyMove が "Air" ステートへ切り替わり、
    // その後 Air ステートが着地を検出して Idle/Move に遷移する。
    // よって「前フレームが Air かつ、今フレームで着地した瞬間」を正解条件とする。
    bool wasInAir = (prevStateName_ == "Air");
    bool justLanded = (player_->GetIsGrounded() && !wasGroundedLastFrame_);
    return wasInAir && justLanded;
}

bool TutorialSystem::CheckMeleeAttack() {
    // 近接攻撃トリガー（3 回）
    return IsMeleeInput();
}

bool TutorialSystem::CheckRangedAttack() {
    // 通常射撃トリガー（3 回）
    return IsRangedTrigger();
}

bool TutorialSystem::CheckChargeAttack(float dt) {
    // J / Y ボタンの「長押し → 離す」を検出する
    // kYButtonChargeThreshold(0.15f) 以上押し続けてから離した場合のみカウント
    GamePad *pad = player_->GetGamePad();

    bool isHolding = false;
    bool isReleased = false;

    if (!pad->IsConnected()) {
        isHolding = input_->PushKey(DIK_J);
        isReleased = input_->ReleaseMomentKey(DIK_J);
    } else {
        isHolding = pad->IsPress(XINPUT_GAMEPAD_Y);
        isReleased = pad->IsRelease(XINPUT_GAMEPAD_Y);
    }

    // 押し込み中: タイマー蓄積
    if (isHolding) {
        chargeHoldTimer_ += dt;
        chargeInputActive_ = true;
    }

    // 離した瞬間: チャージショットが成立していたか判定
    if (isReleased && chargeInputActive_) {
        bool isChargeShot = (chargeHoldTimer_ >= player_->GetChargeThreshold());
        chargeHoldTimer_ = 0.0f;
        chargeInputActive_ = false;
        return isChargeShot; // 長押し成立 → count++
    }

    // 入力なし: タイマーをリセット
    if (!isHolding && !isReleased) {
        chargeHoldTimer_ = 0.0f;
        chargeInputActive_ = false;
    }

    return false;
}

bool TutorialSystem::CheckEnergyCharge() {
    // EnergyCharge 状態中に timer を 3 秒蓄積
    return (player_->GetCurrentStateName() == "EnergyCharge");
}

bool TutorialSystem::CheckSpecialAttack() {
    // チャージ状態 + エネルギー 65 以上 + 必殺技入力
    bool isCharging = (player_->GetCurrentStateName() == "EnergyCharge");
    bool hasEnergy = (player_->GetEnergy() >= 65.0f);
    return isCharging && hasEnergy && IsSpecialTrigger();
}

// ============================================================
//  入力判定ヘルパー
// ============================================================

bool TutorialSystem::IsMoveInput() const {
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->PushKey(DIK_W) || input_->PushKey(DIK_A) ||
               input_->PushKey(DIK_S) || input_->PushKey(DIK_D);
    }
    return (pad->GetLeftStickX() != 0.0f || pad->GetLeftStickY() != 0.0f);
}

bool TutorialSystem::IsJumpTrigger() const {
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->TriggerKey(DIK_SPACE);
    }
    return pad->IsTrigger(XINPUT_GAMEPAD_A);
}

bool TutorialSystem::IsAirTransTrigger() const {
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->TriggerKey(DIK_SPACE);
    }
    return pad->IsTrigger(XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

bool TutorialSystem::IsAscendInput() const {
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->PushKey(DIK_SPACE);
    }
    return (pad->GetRightTrigger() > 0.25f);
}

bool TutorialSystem::IsDescendInput() const {
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->PushKey(DIK_LSHIFT);
    }
    // LT は EnergyCharge 状態以外で下降として扱う（疑似コード仕様）
    bool notCharging = (player_->GetCurrentStateName() != "EnergyCharge");
    return notCharging && (pad->GetLeftTrigger() > 0.25f);
}

bool TutorialSystem::IsEnergyChargeInput() const {
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->PushKey(DIK_C);
    }
    return (pad->GetLeftTrigger() > 0.25f);
}

bool TutorialSystem::IsMeleeInput() const {
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->TriggerKey(DIK_H);
    }
    return pad->IsTrigger(XINPUT_GAMEPAD_B);
}

bool TutorialSystem::IsRangedTrigger() const {
    // キーボード: J トリガー
    // パッド    : Y ボタンを離した瞬間
    //   ※ Player::Shot() の短押し判定と合わせるため IsRelease を使用
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->TriggerKey(DIK_J);
    }
    return pad->IsRelease(XINPUT_GAMEPAD_Y);
}

bool TutorialSystem::IsSpecialTrigger() const {
    // キーボード: G トリガー
    // パッド    : Y ボタントリガー（チャージ中）
    //   ※ パッドの必殺技は isSkillMenu_ 連動だが、
    //      チュートリアルでは EnergyCharge 状態中の Y 入力で代用する
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected()) {
        return input_->TriggerKey(DIK_G);
    }
    return pad->IsTrigger(XINPUT_GAMEPAD_Y);
}

// ============================================================
//  DrawImGui  チュートリアル進行状況のデバッグウィンドウ
// ============================================================
void TutorialSystem::DrawImGui() {
#ifdef _DEBUG
    // ステップ番号→日本語名のテーブル（TutorialStep の順序と 1:1 対応）
    static const char *kStepNames[static_cast<int>(TutorialStep::StepCount)] = {
        "01. 地上移動",           // Move
        "02. ジャンプ",           // Jump
        "03. ホバリング移行",     // FlyTransition
        "04. 上昇",               // Ascend
        "05. 下降",               // Descend
        "06. 空中移動",           // AirMove
        "07. ダッシュ",           // Dash
        "08. 急接近",             // Rush
        "09. 着地",               // Landing
        "10. 近接攻撃",           // MeleeAttack
        "11. 遠距離攻撃",         // RangedAttack
        "12. チャージ攻撃",       // ChargeAttack
        "13. エネルギーチャージ", // EnergyCharge
        "14. 必殺技",             // SpecialAttack
        "--- 完了 ---",           // Complete
    };

    // ── 進捗履歴（implot 用スクロールバッファ）──────────────
    // UpdateCurrentStep() で progress_ が変わるたびにここへ追記される
    // 最大 kHistorySize フレーム分保持し、折れ線グラフで表示する
    static constexpr int kHistorySize = 300;
    static float sProgressHistory[kHistorySize] = {};
    static int sHistoryOffset = 0;

    // 毎フレーム現在の進捗を記録
    sProgressHistory[sHistoryOffset] = progress_;
    sHistoryOffset = (sHistoryOffset + 1) % kHistorySize;

    // ────────────────────────────────────────────────────────
    ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_Once);
    ImGui::Begin("チュートリアル デバッグ");

    // ── 現在のステップ ──────────────────────────────────────
    const int stepIdx = static_cast<int>(currentStep_);
    const bool isComplete = (currentStep_ == TutorialStep::Complete);
    const char *stepName = (stepIdx < static_cast<int>(TutorialStep::StepCount))
                               ? kStepNames[stepIdx]
                               : "不明";

    ImGui::SeparatorText("現在のステップ");
    if (isComplete) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "チュートリアル完了！");
    } else {
        ImGui::Text("ステップ番号 : %d / %d",
                    stepIdx + 1,
                    static_cast<int>(TutorialStep::StepCount) - 1); // Complete を除く
        ImGui::Text("ステップ名   : %s", stepName);
    }

    // ── 操作指示テキスト ─────────────────────────────────────
    ImGui::SeparatorText("操作指示");
    if (!isComplete) {
        const char *instruction = GetInstructionText();
        if (instruction) {
            ImGui::TextWrapped("%s", instruction);
        }
        const char *sub = GetSubText();
        if (sub) {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "補足: %s", sub);
        }
    }

    // ── 進捗（ImGui ProgressBar + implot 折れ線グラフ）───────
    if (!isComplete) {
        ImGui::SeparatorText("進捗");

        const TutorialStepConfig &cfg = kConfigs[stepIdx];

        // ImGui 標準の進行度バー
        char barLabel[32];
        if (cfg.requiredTime > 0.0f) {
            snprintf(barLabel, sizeof(barLabel), "%.2f / %.2f 秒", timer_, cfg.requiredTime);
            ImGui::ProgressBar(progress_, ImVec2(-1.0f, 0.0f), barLabel);
        } else {
            snprintf(barLabel, sizeof(barLabel), "%d / %d 回", count_, cfg.requiredCount);
            ImGui::ProgressBar(progress_, ImVec2(-1.0f, 0.0f), barLabel);
        }

        // ダッシュ: サブフェーズ
        if (currentStep_ == TutorialStep::Dash) {
            const char *phase = (dashSubPhase_ == DashSubPhase::WaitForCharge)
                                    ? "フェーズ1: チャージ待ち"
                                    : "フェーズ2: ダッシュ待ち";
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "サブフェーズ: %s", phase);
        }

        // ChargeAttack: 長押しタイマー
        if (currentStep_ == TutorialStep::ChargeAttack) {
            ImGui::Text("チャージ保持時間 : %.2f 秒", chargeHoldTimer_);
        }

        // implot: 直近 kHistorySize フレームの進捗推移グラフ
        if (ImPlot::BeginPlot("##進捗グラフ", ImVec2(-1.0f, 80.0f),
                              ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                                  ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus)) {
            ImPlot::SetupAxes(nullptr, nullptr,
                              ImPlotAxisFlags_NoDecorations,
                              ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock);
            ImPlot::SetupAxesLimits(0, kHistorySize, 0.0, 1.0, ImGuiCond_Always);

            // スクロールバッファをオフセット付きで折れ線描画
            ImPlot::PlotLine("progress",
                             sProgressHistory,
                             kHistorySize,
                             1.0, // xscale
                             0.0, // xstart
                             ImPlotLineFlags_None,
                             sHistoryOffset); // offset で循環バッファの先頭を指定
            ImPlot::EndPlot();
        }
    }

    // ── 内部フラグ ───────────────────────────────────────────
    ImGui::SeparatorText("内部フラグ");

    auto Flag = [](const char *label, bool value) {
        ImGui::TextColored(
            value ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "%s : %s", label, value ? "ON" : "OFF");
    };
    Flag("ステップ切替直後      ", stepJustChanged_);
    Flag("エネミー出現リクエスト", spawnEnemyRequested_);
    Flag("エネミー消滅リクエスト", despawnEnemyRequested_);
    Flag("「空中へ戻れ」表示中  ", showReturnToAirMessage_);

    // ── ステップ固有設定 ─────────────────────────────────────
    if (!isComplete) {
        const TutorialStepConfig &cfg = kConfigs[stepIdx];
        ImGui::SeparatorText("ステップ設定");
        ImGui::Text("エネミー必要 : %s", cfg.needsEnemy ? "はい" : "いいえ");
    }

    // ── ステップ一覧（折りたたみ）────────────────────────────
    if (ImGui::CollapsingHeader("ステップ一覧")) {
        for (int i = 0; i < static_cast<int>(TutorialStep::StepCount); ++i) {
            const bool isCurrent = (i == stepIdx);
            const bool isDone = (i < stepIdx);
            ImVec4 color = isDone      ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)  // 完了済み: グレー
                           : isCurrent ? ImVec4(0.2f, 1.0f, 0.5f, 1.0f)  // 現在: 緑
                                       : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 未到達: 白
            ImGui::TextColored(color, "%s%s",
                               isCurrent ? "▶ " : "  ",
                               kStepNames[i]);
        }
    }

    ImGui::End();
#endif // _DEBUG
}
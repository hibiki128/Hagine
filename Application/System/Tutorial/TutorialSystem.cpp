#include "TutorialSystem.h"
#include "Application/GameObject/Player/Player.h"
#include <Input.h>
#include <algorithm>
#include <cassert>
#ifdef _DEBUG
#include <implot.h>
#endif // _DEBUG

// ステップ設定テーブル（TutorialStep の順序と 1:1 対応させること）
using namespace Hagine;
const TutorialStepConfig TutorialSystem::kConfigs[static_cast<int>(TutorialStep::StepCount)] = {
    //  instructionText                                                         subText                                             reqTime  reqCount  needsEnemy
    {"[WASD / 左スティック] で移動しよう", nullptr, 3.0f, 0, false},                                      // Move
    {"[SPACE / RBボタン] でジャンプしよう", nullptr, 0.0f, 1, false},                                     // Jump
    {"ジャンプ後に [SPACE / RBボタン] でホバリング！", nullptr, 0.0f, 1, false},                          // FlyTransition
    {"[SPACE保持 / RBボタン保持] で上昇しよう", nullptr, 2.0f, 0, false},                                 // Ascend
    {"[LSHIFT保持 / RT] で下降しよう", "着地してしまったら、もう一度空中状態になろう！", 2.0f, 0, false}, // Descend
    {"[WASD / 左スティック] で空中を移動しよう", nullptr, 3.0f, 0, false},                                // AirMove
    {"[Ctrl保持 / Aボタン] ＋ [WASD / スティック] でダッシュ！", nullptr, 0.0f, 1, false},                // Dash
    {"ダッシュ中に [Ctrl×2 / Aボタン] でダミーへ急接近！", nullptr, 0.0f, 1, true},                       // Rush
    {"[LSHIFT×2 / RT] で空中状態を解除して着地しよう", nullptr, 0.0f, 1, false},                          // Landing
    {"[H / Xボタン] でダミーを近接攻撃！（3回）", nullptr, 0.0f, 3, true},                                // MeleeAttack
    {"[J / Yボタン] で通常射撃！（3回）", nullptr, 0.0f, 3, true},                                        // RangedAttack
    {"[RShift保持 / Bボタン保持] でガード！", nullptr, 0.0f, 1, true},                                    // Guard
    {"[J長押し → 離す / Y長押し → 離す] でチャージ攻撃！", nullptr, 0.0f, 1, true},                       // ChargeAttack
    {"[C保持 / LT保持] でエネルギーをチャージしよう！（3秒）", nullptr, 3.0f, 0, false},                  // EnergyCharge
    {"エネルギーが溜まったら [G / LT長押し中にYボタン] で必殺技！", nullptr, 0.0f, 1, false},             // SpecialAttack
    {"チュートリアル完了！", nullptr, 0.0f, 0, false},                                                    // Complete
};

void TutorialSystem::Initialize(Player *player)
{
    assert(player && "TutorialSystem: player が nullptr です");
    player_ = player;
    input_ = Input::GetInstance();
    currentStep_ = TutorialStep::Move;
    prevStateName_ = player_->GetCurrentStateName();

    ResetStepState();
}

void TutorialSystem::Update(float dt)
{
    stepJustChanged_ = false;

    if (currentStep_ == TutorialStep::Complete)
    {
        return;
    }

    UpdateCurrentStep(dt);

    // フレーム末に状態を保存（次フレームの「前フレーム情報」として使う）
    prevStateName_ = player_->GetCurrentStateName();
    wasGroundedLastFrame_ = player_->GetIsGrounded();
}

void TutorialSystem::Finalize()
{
    player_ = nullptr;
    input_ = nullptr;
}

const char *TutorialSystem::GetInstructionText() const
{
    return kConfigs[static_cast<int>(currentStep_)].instructionText;
}

const char *TutorialSystem::GetSubText() const
{
    // 着地補正メッセージを最優先で返す
    if (showReturnToAirMessage_)
    {
        return kConfigs[static_cast<int>(TutorialStep::Descend)].subText;
    }
    return kConfigs[static_cast<int>(currentStep_)].subText;
}

void TutorialSystem::AdvanceStep()
{
    const bool currNeedsEnemy = kConfigs[static_cast<int>(currentStep_)].needsEnemy;
    const int nextIdx = static_cast<int>(currentStep_) + 1;

    if (nextIdx < static_cast<int>(TutorialStep::StepCount))
    {
        const bool nextNeedsEnemy = kConfigs[nextIdx].needsEnemy;

        // 境界をまたぐときだけリクエストを発行する
        if (!currNeedsEnemy && nextNeedsEnemy)
        {
            RequestSpawnEnemy();
        }
        else if (currNeedsEnemy && !nextNeedsEnemy)
        {
            RequestDespawnEnemy();
        }

        currentStep_ = static_cast<TutorialStep>(nextIdx);
    }
    else
    {
        currentStep_ = TutorialStep::Complete;
    }

    ResetStepState();
    stepJustChanged_ = true;
}

void TutorialSystem::ResetStepState()
{
    timer_ = 0.0f;
    count_ = 0;
    progress_ = 0.0f;
    showReturnToAirMessage_ = false;
    chargeHoldTimer_ = 0.0f;
    chargeInputActive_ = false;
}

void TutorialSystem::RequestSpawnEnemy() { spawnEnemyRequested_ = true; }
void TutorialSystem::RequestDespawnEnemy() { despawnEnemyRequested_ = true; }

void TutorialSystem::UpdateCurrentStep(float dt)
{
    bool conditionMet = false;

    // 各ステップの達成条件をチェック
    switch (currentStep_)
    {
    case TutorialStep::Move:
        conditionMet = CheckMove();
        break;
    case TutorialStep::Jump:
        conditionMet = CheckJump();
        break;
    case TutorialStep::FlyTransition:
        conditionMet = CheckFlyTransition();
        break;
    case TutorialStep::Ascend:
        conditionMet = CheckAscend();
        break;
    case TutorialStep::Descend:
        conditionMet = CheckDescend();
        break;
    case TutorialStep::AirMove:
        conditionMet = CheckAirMove();
        break;
    case TutorialStep::Dash:
        conditionMet = CheckDash();
        break;
    case TutorialStep::Rush:
        conditionMet = CheckRush();
        break;
    case TutorialStep::Landing:
        conditionMet = CheckLanding();
        break;
    case TutorialStep::MeleeAttack:
        conditionMet = CheckMeleeAttack();
        break;
    case TutorialStep::RangedAttack:
        conditionMet = CheckRangedAttack();
        break;
    case TutorialStep::Guard:
        conditionMet = CheckGuard();
        break;
    case TutorialStep::ChargeAttack:
        conditionMet = CheckChargeAttack(dt);
        break;
    case TutorialStep::EnergyCharge:
        conditionMet = CheckEnergyCharge();
        break;
    case TutorialStep::SpecialAttack:
        conditionMet = CheckSpecialAttack();
        break;
    default:
        return;
    }

    const TutorialStepConfig &cfg = kConfigs[static_cast<int>(currentStep_)];

    if (cfg.requiredTime > 0.0f)
    {
        // 時間方式: 条件を満たしている間だけ timer を蓄積
        if (conditionMet)
        {
            timer_ += dt;
        }
        progress_ = std::min(timer_ / cfg.requiredTime, 1.0f);
        if (timer_ >= cfg.requiredTime)
        {
            // 必要時間に達したら次のステップへ
            AdvanceStep();
        }
    }
    else
    {
        // 回数方式: 達成した瞬間（トリガー）に count++
        if (conditionMet)
        {
            count_++;
        }
        progress_ = std::min(static_cast<float>(count_) / static_cast<float>(cfg.requiredCount), 1.0f);
        if (count_ >= cfg.requiredCount)
        {
            // 必要回数に達したら次のステップへ
            AdvanceStep();
        }
    }
}

// ステップ個別チェック
bool TutorialSystem::CheckMove()
{
    // Move または Idle ステートにいる間、移動入力を 3 秒間継続
    const std::string &state = player_->GetCurrentStateName();
    bool isOnGround = (state == "Idle" || state == "Move");
    return isOnGround && IsMoveInput();
}

bool TutorialSystem::CheckJump()
{
    // Jump ステートへ遷移した瞬間を検出（前フレームとの比較）
    const std::string &current = player_->GetCurrentStateName();
    return (current == "Jump" && prevStateName_ != "Jump");
}

bool TutorialSystem::CheckFlyTransition()
{
    // FlyIdle ステートへ遷移した瞬間を検出（前フレームとの比較）
    const std::string &current = player_->GetCurrentStateName();
    return (current == "FlyIdle" && prevStateName_ != "FlyIdle");
}

bool TutorialSystem::CheckAscend()
{
    // 飛行ステートにいる間、上昇入力を 2 秒間継続
    const std::string &state = player_->GetCurrentStateName();
    bool inFly = (state == "FlyIdle" || state == "FlyMove");
    return inFly && IsAscendInput();
}

bool TutorialSystem::CheckDescend()
{
    const std::string &state = player_->GetCurrentStateName();
    bool inFly = (state == "FlyIdle" || state == "FlyMove");
    bool isGrounded = player_->GetIsGrounded();

    // 飛行状態から着地した瞬間を検出して補正メッセージを表示
    if (!inFly && isGrounded && !wasGroundedLastFrame_)
    {
        showReturnToAirMessage_ = true;
    }

    if (showReturnToAirMessage_)
    {
        // FlyIdle/FlyMove に戻るまで待機し、timer は進めない
        if (inFly)
        {
            showReturnToAirMessage_ = false;
        }
        return false;
    }

    // 正常判定: 飛行ステートにいる間、下降入力を 2 秒間継続
    return inFly && IsDescendInput();
}

bool TutorialSystem::CheckAirMove()
{
    // FlyMove ステートにいる間、3 秒間継続
    return (player_->GetCurrentStateName() == "FlyMove");
}

bool TutorialSystem::CheckDash()
{
    // ダッシュが開始したフレームを検出してカウント
    return player_->GetDashStartedThisFrame();
}

bool TutorialSystem::CheckRush()
{
    // Rush ステートへ遷移した瞬間を検出（前フレームとの比較）
    const std::string &current = player_->GetCurrentStateName();
    return (current == "Rush" && prevStateName_ != "Rush");
}

bool TutorialSystem::CheckLanding()
{
    // 空中状態解除の遷移フロー: FlyState → "Air" → 着地（Idle/Move）
    // LSHIFT×2 / RT で FlyMove が "Air" ステートへ切り替わり、
    // その後 Air ステートが着地を検出して Idle/Move に遷移する。
    // 「前フレームが Air かつ、今フレームで着地した瞬間」を正解条件とする。
    bool wasInAir = (prevStateName_ == "Air");
    bool justLanded = (player_->GetIsGrounded() && !wasGroundedLastFrame_);
    return wasInAir && justLanded;
}

bool TutorialSystem::CheckMeleeAttack()
{
    // 近接攻撃トリガー（3 回）
    return IsMeleeInput();
}

bool TutorialSystem::CheckRangedAttack()
{
    // 通常射撃トリガー（3 回）
    return IsRangedTrigger();
}

bool TutorialSystem::CheckGuard()
{
    // Guard ステートへ遷移した瞬間を検出（前フレームとの比較）
    const std::string &current = player_->GetCurrentStateName();
    return (current == "Guard" && prevStateName_ != "Guard");
}

bool TutorialSystem::CheckChargeAttack(float dt)
{
    // J / Y ボタンの「長押し → 離す」を検出する
    // kYButtonChargeThreshold(0.15f) 以上押し続けてから離した場合のみカウント
    GamePad *pad = player_->GetGamePad();

    bool isHolding = false;
    bool isReleased = false;

    if (!pad->IsConnected())
    {
        isHolding = input_->PushKey(DIK_J);
        isReleased = input_->ReleaseMomentKey(DIK_J);
    }
    else
    {
        isHolding = pad->IsPress(XINPUT_GAMEPAD_Y);
        isReleased = pad->IsRelease(XINPUT_GAMEPAD_Y);
    }

    // 押し込み中: タイマー蓄積
    if (isHolding)
    {
        chargeHoldTimer_ += dt;
        chargeInputActive_ = true;
    }

    // 離した瞬間: チャージショットが成立していたか判定
    if (isReleased && chargeInputActive_)
    {
        bool isChargeShot = (chargeHoldTimer_ >= player_->GetChargeThreshold());
        chargeHoldTimer_ = 0.0f;
        chargeInputActive_ = false;
        return isChargeShot;
    }

    // 入力なし: タイマーをリセット
    if (!isHolding && !isReleased)
    {
        chargeHoldTimer_ = 0.0f;
        chargeInputActive_ = false;
    }

    return false;
}

bool TutorialSystem::CheckEnergyCharge()
{
    // EnergyCharge ステートにいる間、timer を 3 秒蓄積
    return (player_->GetCurrentStateName() == "EnergyCharge");
}

bool TutorialSystem::CheckSpecialAttack()
{
    return player_->GetIsSkillActive();
}

// 入力判定ヘルパー
bool TutorialSystem::IsMoveInput() const
{
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected())
    {
        return input_->PushKey(DIK_W) || input_->PushKey(DIK_A) ||
               input_->PushKey(DIK_S) || input_->PushKey(DIK_D);
    }
    return (pad->GetLeftStickX() != 0.0f || pad->GetLeftStickY() != 0.0f);
}

bool TutorialSystem::IsJumpTrigger() const
{
    // SPACE トリガー / RBボタン トリガー
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected())
    {
        return input_->TriggerKey(DIK_SPACE);
    }
    return pad->IsTrigger(XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

bool TutorialSystem::IsAirTransTrigger() const
{
    // SPACE トリガー / RBボタン トリガー（空中）
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected())
    {
        return input_->TriggerKey(DIK_SPACE);
    }
    return pad->IsTrigger(XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

bool TutorialSystem::IsAscendInput() const
{
    // SPACE 保持 / RBボタン 保持
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected())
    {
        return input_->PushKey(DIK_SPACE);
    }
    return pad->IsPress(XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

bool TutorialSystem::IsDescendInput() const
{
    // LSHIFT 保持 / RT 押し込み
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected())
    {
        return input_->PushKey(DIK_LSHIFT);
    }
    return (pad->GetRightTrigger() > 0.25f);
}

bool TutorialSystem::IsEnergyChargeInput() const
{
    // C 保持 / LT 押し込み
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected())
    {
        return input_->PushKey(DIK_C);
    }
    return (pad->GetLeftTrigger() > 0.25f);
}

bool TutorialSystem::IsMeleeInput() const
{
    // H トリガー / Xボタン トリガー
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected())
    {
        return input_->TriggerKey(DIK_H);
    }
    return pad->IsTrigger(XINPUT_GAMEPAD_X);
}

bool TutorialSystem::IsRangedTrigger() const
{
    // J トリガー / Yボタン 離した瞬間
    // Player::Shot() の短押し判定に合わせて IsRelease を使用
    GamePad *pad = player_->GetGamePad();
    if (!pad->IsConnected())
    {
        return input_->TriggerKey(DIK_J);
    }
    return pad->IsRelease(XINPUT_GAMEPAD_Y);
}

void TutorialSystem::DrawImGui()
{
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
        "12. ガード",             // Guard
        "13. チャージ攻撃",       // ChargeAttack
        "14. エネルギーチャージ", // EnergyCharge
        "15. 必殺技",             // SpecialAttack
        "--- 完了 ---",           // Complete
    };

    // 進捗履歴（implot 用スクロールバッファ）
    // UpdateCurrentStep() で progress_ が変わるたびに追記され、
    // 最大 kHistorySize フレーム分保持して折れ線グラフで表示する
    static constexpr int kHistorySize = 300;
    static float sProgressHistory[kHistorySize] = {};
    static int sHistoryOffset = 0;

    sProgressHistory[sHistoryOffset] = progress_;
    sHistoryOffset = (sHistoryOffset + 1) % kHistorySize;

    ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_Once);
    ImGui::Begin("チュートリアル デバッグ");

    const int stepIdx = static_cast<int>(currentStep_);
    const bool isComplete = (currentStep_ == TutorialStep::Complete);
    const char *stepName = (stepIdx < static_cast<int>(TutorialStep::StepCount))
                               ? kStepNames[stepIdx]
                               : "不明";

    ImGui::SeparatorText("現在のステップ");
    if (isComplete)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "チュートリアル完了！");
    }
    else
    {
        ImGui::Text("ステップ番号 : %d / %d",
                    stepIdx + 1,
                    static_cast<int>(TutorialStep::StepCount) - 1); // Complete を除く
        ImGui::Text("ステップ名   : %s", stepName);
    }

    ImGui::SeparatorText("操作指示");
    if (!isComplete)
    {
        const char *instruction = GetInstructionText();
        if (instruction)
        {
            ImGui::TextWrapped("%s", instruction);
        }
        const char *sub = GetSubText();
        if (sub)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.1f, 1.0f), "補足: %s", sub);
        }
    }

    if (!isComplete)
    {
        ImGui::SeparatorText("進捗");

        const TutorialStepConfig &cfg = kConfigs[stepIdx];

        char barLabel[32];
        if (cfg.requiredTime > 0.0f)
        {
            snprintf(barLabel, sizeof(barLabel), "%.2f / %.2f 秒", timer_, cfg.requiredTime);
            ImGui::ProgressBar(progress_, ImVec2(-1.0f, 0.0f), barLabel);
        }
        else
        {
            snprintf(barLabel, sizeof(barLabel), "%d / %d 回", count_, cfg.requiredCount);
            ImGui::ProgressBar(progress_, ImVec2(-1.0f, 0.0f), barLabel);
        }

        if (currentStep_ == TutorialStep::ChargeAttack)
        {
            ImGui::Text("チャージ保持時間 : %.2f 秒", chargeHoldTimer_);
        }

        // 直近 kHistorySize フレームの進捗推移グラフ
        if (ImPlot::BeginPlot("##進捗グラフ", ImVec2(-1.0f, 80.0f),
                              ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                                  ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus))
        {
            ImPlot::SetupAxes(nullptr, nullptr,
                              ImPlotAxisFlags_NoDecorations,
                              ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock);
            ImPlot::SetupAxesLimits(0, kHistorySize, 0.0, 1.0, ImGuiCond_Always);
            ImPlot::PlotLine("progress",
                             sProgressHistory,
                             kHistorySize,
                             1.0,
                             0.0,
                             ImPlotLineFlags_None,
                             sHistoryOffset);
            ImPlot::EndPlot();
        }
    }

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

    if (!isComplete)
    {
        const TutorialStepConfig &cfg = kConfigs[stepIdx];
        ImGui::SeparatorText("ステップ設定");
        ImGui::Text("エネミー必要 : %s", cfg.needsEnemy ? "はい" : "いいえ");
    }

    if (ImGui::CollapsingHeader("ステップ一覧"))
    {
        for (int i = 0; i < static_cast<int>(TutorialStep::StepCount); ++i)
        {
            const bool isCurrent = (i == stepIdx);
            const bool isDone = (i < stepIdx);
            ImVec4 color = isDone      ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
                           : isCurrent ? ImVec4(0.2f, 1.0f, 0.5f, 1.0f)
                                       : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::TextColored(color, "%s%s",
                               isCurrent ? "▶ " : "  ",
                               kStepNames[i]);
        }
    }

    ImGui::End();
#endif // _DEBUG
}
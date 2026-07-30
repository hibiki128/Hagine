#include "ComboSystem.h"
#include "edit/motion/MotionEditor.h"
#include "object/base/BaseObject.h"
#include "utility/debug/imgui/ImGuiNotification.h"
#include <algorithm>

#ifdef _DEBUG
#include "imgui.h"
#endif

using namespace Hagine;

namespace {
// 範囲外アクセス時に返す空文字（参照を返すゲッター用）
const std::string kEmptyString;
} // namespace

/// ===================================================
/// 定数定義
/// ===================================================

// ある段を繰り出してから次の段を受け付けるまでのクールダウン（秒）。
// この間にコンボ入力されても即発動はせず、先行入力としてバッファされる
const float ComboSystem::kComboInterval = 0.45f;

// クールダウン中に行われた先行入力を保持する有効時間（秒）。
// この時間内であればクールダウンが明けた瞬間に次の段が自動発動する
const float ComboSystem::kInputBufferDuration = 0.4f;

// 最終段を出し切ってから開始姿勢へ戻すまでの待機時間（秒）。
// フィニッシュモーションを見せる余韻として確保する
const float ComboSystem::kFinalReturnDelay = 0.5f;

// コンボ継続中に次の入力が来ないまま経過すると、コンボを打ち切って
// 開始姿勢へ戻すまでの放置タイムアウト時間（秒）
const float ComboSystem::kComboTimeoutDuration = 0.5f;

ComboSystem::ComboSystem()
    : comboIndex_(0), comboCooldown_(0.0f), comboStarted_(false),
      waitingForReturn_(false), returnDelay_(0.0f), comboTimeout_(0.0f),
      inputBuffered_(false), inputBufferTime_(0.0f)
{
}

ComboSystem::~ComboSystem()
{
    Clear();
}

bool ComboSystem::LoadDefinition(const std::string &fileName, BaseObject *pMotionTarget)
{
    Clear();

    if (!definition_.Load(fileName))
    {
        return false;
    }

    // 段の並び・パラメータはすべて定義データ側が持つ。ここでは実行用に写すだけ
    for (const ComboStepData &step : definition_.GetSteps())
    {
        comboData_.emplace_back(pMotionTarget, step);
    }

    finishRecovery_ = definition_.GetFinishRecovery();

    // モーション再生対象があれば、開始姿勢を戻すためのオブジェクトとして登録する
    if (pMotionTarget != nullptr)
    {
        pComboStartObjects_.push_back(pMotionTarget);
    }

    return true;
}

void ComboSystem::SaveDefinition()
{
    // ImGuiで調整した実行時の値を定義データへ書き戻してから保存する
    std::vector<ComboStepData> &steps = definition_.GetSteps();
    const size_t stepCount = (std::min)(steps.size(), comboData_.size());
    for (size_t i = 0; i < stepCount; ++i)
    {
        steps[i] = comboData_[i].step;
    }
    definition_.SetFinishRecovery(finishRecovery_);

    if (definition_.Save())
    {
        ImGuiNotification::Post("コンボ定義を保存しました: " + definition_.GetFileName(),
                                {0.2f, 0.8f, 0.2f, 1.0f});
    }
    else
    {
        ImGuiNotification::Post("コンボ定義の保存に失敗しました: " + definition_.GetFileName(),
                                {0.9f, 0.3f, 0.3f, 1.0f});
    }
}

void ComboSystem::Clear()
{
    comboData_.clear();
    pComboStartObjects_.clear();
    ResetCombo();
}

const std::string &ComboSystem::GetAttackName(int index) const
{
    if (index < 0 || index >= static_cast<int>(comboData_.size()))
    {
        return kEmptyString;
    }
    return comboData_[index].step.attackName;
}

const std::string &ComboSystem::GetAnimationPath(int index) const
{
    if (index < 0 || index >= static_cast<int>(comboData_.size()))
    {
        return kEmptyString;
    }
    return comboData_[index].step.animationPath;
}

void ComboSystem::SetAnimationPath(int index, const std::string &animationPath)
{
    if (index < 0 || index >= static_cast<int>(comboData_.size()))
    {
        return;
    }
    comboData_[index].step.animationPath = animationPath;
}

bool ComboSystem::TryExecuteCombo()
{
    if (waitingForReturn_ || comboData_.empty())
    {
        return false;
    }

    if (comboCooldown_ <= 0.0f)
    {
        ExecuteComboAttack();
        return true;
    }
    else
    {
        // 先行入力
        if (comboStarted_)
        {
            inputBuffered_ = true;
            inputBufferTime_ = kInputBufferDuration;
        }
        return false;
    }
}

void ComboSystem::Update(float deltaTime)
{
    comboCooldown_ -= deltaTime;
    returnDelay_ -= deltaTime;
    inputBufferTime_ -= deltaTime;

    bool attackExecuted = false;

    // 先行入力の処理
    if (inputBuffered_ && comboCooldown_ <= 0.0f)
    {
        inputBuffered_ = false;
        inputBufferTime_ = 0.0f;
        ExecuteComboAttack();
        attackExecuted = true;
    }

    // 先行入力有効期限切れ
    if (inputBufferTime_ <= 0.0f)
    {
        inputBuffered_ = false;
    }

    // 最終攻撃後の戻り
    if (waitingForReturn_ && returnDelay_ <= 0.0f)
    {
        ReturnAllToComboStart();
        ResetCombo();

        // 出し切った直後に1段目から殴り直せてしまわないよう、硬直を入れる。
        // ResetCombo() がクールダウンを0に戻すため、その後に設定する
        comboCooldown_ = finishRecovery_;
    }

    // タイムアウトによるコンボ終了
    if (comboStarted_ && !waitingForReturn_ && !attackExecuted)
    {
        comboTimeout_ += deltaTime;
        if (comboTimeout_ >= kComboTimeoutDuration)
        {
            ReturnAllToComboStart();
            ResetCombo();
        }
    }
}

void ComboSystem::ExecuteComboAttack()
{
    if (comboIndex_ >= static_cast<int>(comboData_.size()))
    {
        return;
    }

    const ComboData &currentCombo = comboData_[comboIndex_];
    BaseObject *pCurrentTarget = currentCombo.pTarget;

    // 以前の攻撃が終了していたら戻す
    if (comboIndex_ > 0)
    {
        for (int i = comboIndex_ - 1; i >= 0; i--)
        {
            BaseObject *pPrevTarget = comboData_[i].pTarget;
            if (pPrevTarget && pPrevTarget == pCurrentTarget)
            {
                if (MotionEditor::GetInstance()->IsAttackFinishedWithInterval(pPrevTarget))
                {
                    MotionEditor::GetInstance()->ReturnToComboStart(pPrevTarget);
                    MotionEditor::GetInstance()->ClearAttackEndInterval(pPrevTarget);
                }
                break;
            }
        }
    }

    comboCooldown_ = kComboInterval;

    // 今まさに発火した攻撃の名前を記録する（発火コールバックから参照できるよう先に確定）
    lastAttackName_ = currentCombo.step.attackName;

    if (!comboStarted_)
    {
        SaveComboStartPositions();
        comboStarted_ = true;
    }
    comboTimeout_ = 0.0f;

    // 攻撃発火コールバック実行
    if (onAttackFired_)
    {
        onAttackFired_(
            currentCombo.step.damage,
            currentCombo.step.knockbackPower,
            currentCombo.step.colliderActiveDuration,
            currentCombo.step.colliderActivateDelay);
    }

    // モーション再生
    if (pCurrentTarget != nullptr && !currentCombo.step.attackName.empty())
    {
        MotionEditor::GetInstance()->Stop(pCurrentTarget->GetName());
        MotionEditor::GetInstance()->PlayFromFile(pCurrentTarget, currentCombo.step.attackName);
    }

    comboIndex_++;
    // 全コンボ終了時の設定
    if (comboIndex_ >= static_cast<int>(comboData_.size()))
    {
        waitingForReturn_ = true;
        returnDelay_ = kFinalReturnDelay;
        comboIndex_ = 0;
    }
}

void ComboSystem::ResetCombo()
{
    comboStarted_ = false;
    waitingForReturn_ = false;
    comboIndex_ = 0;
    comboTimeout_ = 0.0f;
    inputBuffered_ = false;
    inputBufferTime_ = 0.0f;
    comboCooldown_ = 0.0f;
}

void ComboSystem::SaveComboStartPositions()
{
    for (BaseObject *pObject : pComboStartObjects_)
    {
        if (pObject != nullptr)
        {
            MotionEditor::GetInstance()->SetComboStartPosition(pObject);
        }
    }
}

void ComboSystem::ReturnAllToComboStart()
{
    for (BaseObject *pObject : pComboStartObjects_)
    {
        if (pObject != nullptr)
        {
            MotionEditor::GetInstance()->ReturnToComboStart(pObject);
        }
    }
}

bool ComboSystem::IsObjectAttackCompleted(BaseObject *pTarget) const
{
    if (!pTarget)
    {
        return true;
    }
    return !MotionEditor::GetInstance()->IsAttackFinishedWithInterval(pTarget);
}

bool ComboSystem::IsCurrentAttackCompleted() const
{
    if (comboIndex_ == 0 || comboIndex_ > static_cast<int>(comboData_.size()))
    {
        return true;
    }
    const ComboData &prevCombo = comboData_[comboIndex_ - 1];
    return !IsObjectAttackCompleted(prevCombo.pTarget);
}

#ifdef _DEBUG
void ComboSystem::DrawImGui()
{
    if (comboData_.empty())
    {
        ImGui::TextDisabled("コンボデータなし");
        return;
    }

    ImGui::Text("コンボ名: %s", definition_.GetFileName().c_str());
    ImGui::Text("現在のインデックス: %d / %d",
                comboIndex_, static_cast<int>(comboData_.size()));
    ImGui::Separator();

    // 攻撃パラメータ編集
    for (size_t i = 0; i < comboData_.size(); ++i)
    {
        ComboStepData &step = comboData_[i].step;

        std::string nodeLabel = std::to_string(i + 1) + "段目: " + step.attackName;
        if (ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (comboStarted_ && comboIndex_ > 0 &&
                static_cast<int>(i) == comboIndex_ - 1)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "<<< 実行中");
            }

            ImGui::PushID(static_cast<int>(i));

            ImGui::DragFloat("ダメージ##dmg", &step.damage, 0.01f);
            ImGui::DragFloat("ノックバック##kb", &step.knockbackPower, 0.01f);
            ImGui::Separator();
            ImGui::DragFloat("判定有効時間(秒)##dur", &step.colliderActiveDuration, 0.01f);
            ImGui::DragFloat("判定開始遅延(秒)##dly", &step.colliderActivateDelay, 0.01f);

            ImGui::PopID();
            ImGui::TreePop();
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    // セーブ/ロード
    if (ImGui::Button("コンボ定義を保存", ImVec2(200.0f, 0.0f)))
    {
        SaveDefinition();
    }
    ImGui::SameLine();
    if (ImGui::Button("再読み込み", ImVec2(120.0f, 0.0f)))
    {
        // 現在のモーション再生対象を保ったまま定義を読み直す
        BaseObject *pMotionTarget = comboData_.empty() ? nullptr : comboData_.front().pTarget;
        LoadDefinition(definition_.GetFileName(), pMotionTarget);
    }

    ImGui::Spacing();
    ImGui::TextDisabled("保存先: ComboDefinition/%s.json", definition_.GetFileName().c_str());
}
#endif

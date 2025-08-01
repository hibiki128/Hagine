#include "ComboSystem.h"
#include "Application/Utility/MotionEditor/MotionEditor.h"
#include "Object/Base/BaseObject.h"
#include <algorithm>

// 設定値の定義
const float ComboSystem::COMBO_INTERVAL = 0.15f;
const float ComboSystem::INPUT_BUFFER_DURATION = 0.4f;
const float ComboSystem::FINAL_RETURN_DELAY = 0.8f;
const float ComboSystem::COMBO_TIMEOUT_DURATION = 2.0f;

ComboSystem::ComboSystem()
    : comboIndex_(0), comboCooldown_(0.0f), comboStarted_(false), waitingForReturn_(false), returnDelay_(0.0f), comboTimeout_(0.0f), inputBuffered_(false), inputBufferTime_(0.0f) {
}

ComboSystem::~ComboSystem() {
    Clear();
}

ComboSystem &ComboSystem::Add(BaseObject *target, const std::string &attackData) {
    comboData_.emplace_back(target, attackData);

    // 新しいオブジェクトを初期位置保存リストに追加
    if (target != nullptr) {
        auto it = std::find(comboStartObjects_.begin(), comboStartObjects_.end(), target);
        if (it == comboStartObjects_.end()) {
            comboStartObjects_.push_back(target);
        }
    }

    return *this;
}

void ComboSystem::Clear() {
    comboData_.clear();
    comboStartObjects_.clear();
    ResetCombo();
}

bool ComboSystem::TryExecuteCombo() {
    if (waitingForReturn_ || comboData_.empty()) {
        return false;
    }

    if (comboCooldown_ <= 0.0f) {
        // すぐに攻撃を実行
        ExecuteComboAttack();
        return true;
    } else {
        if (comboStarted_) {
            inputBuffered_ = true;
            inputBufferTime_ = INPUT_BUFFER_DURATION;
        }
        return false;
    }
}

void ComboSystem::Update(float deltaTime) {
    comboCooldown_ -= deltaTime;
    returnDelay_ -= deltaTime;
    inputBufferTime_ -= deltaTime;

    bool attackExecuted = false;

    // 入力バッファの処理：猶予時間内に入力があった場合
    if (inputBuffered_ && comboCooldown_ <= 0.0f) {
        inputBuffered_ = false;
        inputBufferTime_ = 0.0f;
        ExecuteComboAttack();
        attackExecuted = true;
    }

    // 入力バッファのタイムアウト
    if (inputBufferTime_ <= 0.0f) {
        inputBuffered_ = false;
    }

    // 最後のコンボ後の戻り処理
    if (waitingForReturn_ && returnDelay_ <= 0.0f) {
        for (BaseObject *obj : comboStartObjects_) {
            if (obj != nullptr) {
                MotionEditor::GetInstance()->ReturnToComboStart(obj);
            }
        }
        ResetCombo();
    }

    // コンボがタイムアウトした場合の処理
    if (comboStarted_ && !waitingForReturn_ && !attackExecuted) {
        comboTimeout_ += deltaTime;
        if (comboTimeout_ >= COMBO_TIMEOUT_DURATION) {
            for (BaseObject *obj : comboStartObjects_) {
                if (obj != nullptr) {
                    MotionEditor::GetInstance()->ReturnToComboStart(obj);
                }
            }
            ResetCombo();
        }
    }
}

void ComboSystem::ExecuteComboAttack() {
    if (comboIndex_ >= comboData_.size()) {
        return;
    }

    // 現在の攻撃のターゲットを取得
    const ComboData &currentCombo = comboData_[comboIndex_];
    BaseObject *currentTarget = currentCombo.target;

    // 同じオブジェクトの前回の攻撃があれば、攻撃が完全に終了してから初期位置に戻す
    if (comboIndex_ > 0) {
        for (int i = comboIndex_ - 1; i >= 0; i--) {
            BaseObject *prevTarget = comboData_[i].target;
            if (prevTarget && prevTarget == currentTarget) {
                // 攻撃が完全に終了（インターバル込み）しているかチェック
                if (MotionEditor::GetInstance()->IsAttackFinishedWithInterval(prevTarget)) {
                    MotionEditor::GetInstance()->ReturnToComboStart(prevTarget);
                    MotionEditor::GetInstance()->ClearAttackEndInterval(prevTarget);
                }
                break; // 最初に見つかった同じオブジェクトだけを処理
            }
        }
    }

    comboCooldown_ = COMBO_INTERVAL;

    if (!comboStarted_) {
        // コンボ開始時に初期位置を保存
        SaveComboStartPositions();
        comboStarted_ = true;

        // コンボタイムアウトリセット
        comboTimeout_ = 0.0f;
    } else {
        // 2段目以降の攻撃でタイマーもリセットする
        comboTimeout_ = 0.0f;
    }

    if (currentTarget == nullptr || currentCombo.attackData.empty()) {
        comboIndex_++;
        if (comboIndex_ >= comboData_.size()) {
            waitingForReturn_ = true;
            returnDelay_ = FINAL_RETURN_DELAY;
            comboIndex_ = 0;
        }
        return;
    }

    MotionEditor::GetInstance()->Stop(currentTarget->GetName());
    MotionEditor::GetInstance()->PlayFromFile(currentTarget, currentCombo.attackData);

    comboIndex_++;
    if (comboIndex_ >= comboData_.size()) {
        waitingForReturn_ = true;
        returnDelay_ = FINAL_RETURN_DELAY;
        comboIndex_ = 0;
    }
}

void ComboSystem::ResetCombo() {
    comboStarted_ = false;
    waitingForReturn_ = false;
    comboIndex_ = 0;
    comboTimeout_ = 0.0f;
    inputBuffered_ = false;
    inputBufferTime_ = 0.0f;
    comboCooldown_ = 0.0f;
}

void ComboSystem::SaveComboStartPositions() {
    for (BaseObject *obj : comboStartObjects_) {
        if (obj != nullptr) {
            MotionEditor::GetInstance()->SetComboStartPosition(obj);
        }
    }
}

bool ComboSystem::IsObjectAttackCompleted(BaseObject *target) const {
    if (!target)
        return true;
    return !MotionEditor::GetInstance()->IsAttackFinishedWithInterval(target);
}

bool ComboSystem::IsCurrentAttackCompleted() const {
    if (comboIndex_ == 0 || comboIndex_ > comboData_.size())
        return true;

    // 一つ前の攻撃のターゲットをチェック
    const ComboData &prevCombo = comboData_[comboIndex_ - 1];
    return !IsObjectAttackCompleted(prevCombo.target);
}

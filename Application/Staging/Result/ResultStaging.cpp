#include "ResultStaging.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Object/Base/BaseObjectManager.h>

void ResultStaging::Initialize() {

    /// ===================================================
    /// ポインタ共有
    /// ===================================================

    RightHand_ = BaseObjectManager::GetInstance()->GetObjectByName("sphere_1");
    LeftHand_ = BaseObjectManager::GetInstance()->GetObjectByName("sphere_2");

    /// ===================================================
    /// 登録
    /// ===================================================
    MotionEditor::GetInstance()->Register(RightHand_);
    MotionEditor::GetInstance()->Register(LeftHand_);
}

void ResultStaging::Update() {

    if (!secondMove_ && !motionStarted_) {
        MotionEditor::GetInstance()->PlayFromFile(LeftHand_, "LeftPunch");
        MotionEditor::GetInstance()->PlayFromFile(RightHand_, "RightBack");
        motionStarted_ = true;
    }
    if (!secondMove_ &&
        MotionEditor::GetInstance()->IsAttackFinished(LeftHand_) &&
        MotionEditor::GetInstance()->IsAttackFinished(RightHand_)) {
        secondMove_ = true;
        MotionEditor::GetInstance()->PlayFromFile(LeftHand_, "LeftBack");
        MotionEditor::GetInstance()->PlayFromFile(RightHand_, "RightPunch");
    }
}
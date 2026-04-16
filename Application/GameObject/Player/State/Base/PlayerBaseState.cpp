#include "PlayerBaseState.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"

bool PlayerBaseState::HasMovementInput(Player &player) {
    if (!player.GetGamePad()->IsConnected()) {
        return Input::GetInstance()->PushKey(DIK_A) ||
               Input::GetInstance()->PushKey(DIK_D) ||
               Input::GetInstance()->PushKey(DIK_S) ||
               Input::GetInstance()->PushKey(DIK_W);
    }
    return player.GetGamePad()->GetLeftStickX() != 0.0f ||
           player.GetGamePad()->GetLeftStickY() != 0.0f;
}

bool PlayerBaseState::IsJumpOrFlyTriggered(Player &player) {
    if (!player.GetGamePad()->IsConnected()) {
        return Input::GetInstance()->TriggerKey(DIK_SPACE);
    }
    return player.GetGamePad()->IsTrigger(XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

bool PlayerBaseState::IsAscendInput(Player &player) {
    if (!player.GetGamePad()->IsConnected()) {
        return Input::GetInstance()->PushKey(DIK_SPACE);
    }
    return player.GetGamePad()->IsPress(XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

bool PlayerBaseState::IsDescendInput(Player &player) {
    if (!player.GetGamePad()->IsConnected()) {
        return Input::GetInstance()->PushKey(DIK_LSHIFT);
    }
    return player.GetGamePad()->GetRightTrigger() > 0.25f;
}

bool PlayerBaseState::IsDescendTriggered(Player &player) {
    if (!player.GetGamePad()->IsConnected()) {
        return Input::GetInstance()->TriggerKey(DIK_LSHIFT);
    }
    return player.GetGamePad()->IsRightTriggerTriggered(0.25f);
}

bool PlayerBaseState::HasFlyAnyInput(Player &player) {
    if (!player.GetGamePad()->IsConnected()) {
        return Input::GetInstance()->PushKey(DIK_A) ||
               Input::GetInstance()->PushKey(DIK_D) ||
               Input::GetInstance()->PushKey(DIK_S) ||
               Input::GetInstance()->PushKey(DIK_W) ||
               Input::GetInstance()->PushKey(DIK_SPACE) ||
               Input::GetInstance()->PushKey(DIK_LSHIFT);
    }
    return player.GetGamePad()->GetLeftStickX() != 0.0f ||
           player.GetGamePad()->GetLeftStickY() != 0.0f ||
           player.GetGamePad()->GetRightTrigger() > 0.25f ||
           player.GetGamePad()->IsPress(XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

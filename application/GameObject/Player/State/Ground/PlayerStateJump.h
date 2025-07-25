#pragma once
#include "../Base/PlayerBaseState.h"
class PlayerStateJump : public PlayerBaseState {
  public:
    PlayerStateJump() = default;
    void Enter(Player &player) override;
    void Update(Player &player) override;
    void Exit(Player &player) override;
};

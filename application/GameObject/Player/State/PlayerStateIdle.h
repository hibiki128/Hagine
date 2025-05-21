#pragma once
#include "PlayerBaseState.h"
class PlayerStateIdle : public PlayerBaseState {
  public:
    PlayerStateIdle() = default;
    void Enter(Player &player) override;
    void Update(Player &player) override;
    void Exit(Player &player) override;
};

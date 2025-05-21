#pragma once
#include"PlayerBaseState.h"

class PlayerStateMove : public PlayerBaseState {
  public:
    PlayerStateMove() = default;
    void Enter(Player &player) override;
    void Update(Player &player) override;
    void Exit(Player &player) override;
};

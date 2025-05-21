#pragma once
#include "PlayerBaseState.h"
class PlayerStateAir : public PlayerBaseState {
  public:
    PlayerStateAir() = default;
    void Enter(Player &player) override;
    void Update(Player &player) override;
    void Exit(Player &player) override;

  private:
    float elapsedTime_ = 0.0f;
};

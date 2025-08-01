#pragma once
#include "../Base/PlayerBaseState.h"
class PlayerStateFlyMove : public PlayerBaseState {
  public:
    PlayerStateFlyMove() = default;
    void Enter(Player &player) override;
    void Update(Player &player) override;
    void Exit(Player &player) override;

  private:
    void AirMove(Player &player);
    void ChangeState(Player &player);

    float spaceHeldTime_ = 0.0f;
    bool isBoosting_ = false;

    float fallInputTime_ = 0.0f;
    int fallInputCount_ = 0;
};
#pragma once
#include "../Base/PlayerBaseState.h"

class PlayerStateMove : public PlayerBaseState {
  public:
    void Enter(Player &player) override;
    void Update(Player &player) override;
    void Exit(Player &player) override;

private:
 
};
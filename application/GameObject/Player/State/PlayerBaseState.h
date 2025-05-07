#pragma once
class Player;
class PlayerBaseState {
  public:
    virtual ~PlayerBaseState() = default;
    virtual void Enter(Player& player) = 0;
    virtual void Update(Player &player) = 0;
    virtual void Exit(Player &player) = 0;
};

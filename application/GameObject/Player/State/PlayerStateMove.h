#pragma once
#include "PlayerBaseState.h"

class PlayerStateMove : public PlayerBaseState {
  public:
    void Enter(Player &player) override;
    void Update(Player &player) override;
    void Exit(Player &player) override;

private:
    // カメラが使用できない場合の従来の移動処理
    void DefaultMovement(Player &player);
};
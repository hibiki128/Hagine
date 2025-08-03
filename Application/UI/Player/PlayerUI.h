#pragma once
#include "Application/GameObject/Player/Player.h"
#include "Sprite.h"

class PlayerUI {
  public:
    void Init(Player *player);
    void Update();
    void Draw();
    void Debug();

  private:
    Player *player_ = nullptr; // プレイヤーへの参照
    std::unique_ptr<Sprite> hpBar_;
    std::unique_ptr<Sprite> playerIcon_;
    std::unique_ptr<Sprite> energyBar_;
    std::unique_ptr<Sprite> barFrame_;
    std::unique_ptr<Sprite> energyBarFrame_;

    Vector2 hpBarPosition_ = {200.0f, 45.0f}; // HPバーの位置
    Vector2 hpBarSize_ = {400.0f, 40.0f};         // HPバーのサイズ
    Vector2 playerIconPosition_ = {-80.0f, -30.0f}; // プレイヤーアイコンの位置
    Vector2 iconSize_ = {384.0f, 216.0f};           // プレイヤーアイコンのサイズ
    Vector2 energyBarPosition_ = {200.0f, 93.0f}; // エネルギーバーの位置
    Vector2 energyBarSize_ = {300.0f, 20.0f};     // エネルギーバーのサイズ
    Vector2 barFramePosition_ = {198.0f, 43.0f};  // バーフレームの位置
    Vector2 barSize_ = {410.0f, 50.0f};           // バーのサイズ
    Vector2 energyBarFramePosition_ = {198.0f, 91.0f}; // エネルギーバーフレームの位置
    Vector2 energyBarFrameSize_ = {310.0f, 30.0f};     // エネルギーバーフレームのサイズ
};

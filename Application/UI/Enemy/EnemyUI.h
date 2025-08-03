#pragma once
#include "Application/GameObject/Enemy/Enemy.h"
#include "Sprite.h"

class EnemyUI {
  public:
    void Init(Enemy *enemy);
    void Update();
    void Draw();
    void Debug();

  private:
    Enemy *enemy_ = nullptr; // エネミーへの参照
    std::unique_ptr<Sprite> hpBar_;
    std::unique_ptr<Sprite> enemyIcon_;
    std::unique_ptr<Sprite> energyBar_;
    std::unique_ptr<Sprite> barFrame_;
    std::unique_ptr<Sprite> energyBarFrame_;

    Vector2 hpBarPosition_ = {1160.0f, 45.0f};          // HPバーの位置（右側）
    Vector2 hpBarSize_ = {400.0f, 40.0f};               // HPバーのサイズ
    Vector2 enemyIconPosition_ = {1456.0f, -30.0f};     // エネミーアイコンの位置（右側）
    Vector2 iconSize_ = {384.0f, 216.0f};               // エネミーアイコンのサイズ
    Vector2 energyBarPosition_ = {1260.0f, 93.0f};      // エネルギーバーの位置（右側）
    Vector2 energyBarSize_ = {300.0f, 20.0f};           // エネルギーバーのサイズ
    Vector2 barFramePosition_ = {1150.0f, 43.0f};       // バーフレームの位置（右側）
    Vector2 barSize_ = {410.0f, 50.0f};                 // バーのサイズ
    Vector2 energyBarFramePosition_ = {1252.0f, 91.0f}; // エネルギーバーフレームの位置（右側）
    Vector2 energyBarFrameSize_ = {310.0f, 30.0f};      // エネルギーバーフレームのサイズ
};
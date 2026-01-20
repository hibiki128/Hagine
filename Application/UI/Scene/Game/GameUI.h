#pragma once
#include <GamePad.h>
#include <SpriteManager.h>
#include <array>
class GameUI {
  public:
    void Initialize();

    void Update();

    void Draw();

    enum SpriteIndex {
        MenuButton,
        Controller,
        AirController,
        AttackButton,
        BulletButton,
        BlackMask,
        kMaxSprite,
    };

    bool GetIsPause() const { return isPause_; }

  private:

    bool isPause_ = false;

    std::unique_ptr<GamePad> gamePad_ = nullptr;
    std::array<SpriteData *, kMaxSprite> sprites_;
};

#include "GameUI.h"
#include <Input.h>

void GameUI::Initialize() {
    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);
    SpriteManager::GetInstance()->SetSaveFolder("GameScene");
    SpriteManager::GetInstance()->LoadAllSprites();
    sprites_[MenuButton] = SpriteManager::GetInstance()->GetSprite("menuButton");
    sprites_[Controller] = SpriteManager::GetInstance()->GetSprite("controllerIcon");
    sprites_[BlackMask] = SpriteManager::GetInstance()->GetSprite("BlackMask");
    sprites_[AirController] = SpriteManager::GetInstance()->GetSprite("AirControllerIcon");
    // sprites_[AttackButton] = SpriteManager::GetInstance()->GetSprite("attackButton");
    // sprites_[BulletButton] = SpriteManager::GetInstance()->GetSprite("bulletButton");
}

void GameUI::Update() {
    gamePad_->Update();
    // ポーズボタン入力検出
    if (!gamePad_->IsConnected()) {
        // キーボード入力
        if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
            isPause_ = !isPause_;
        }
    } else {
        // ゲームパッド入力
        if (gamePad_->IsTrigger(XINPUT_GAMEPAD_START)) {
            isPause_ = !isPause_;
        }
    }

    if (isPause_) {
        sprites_[BlackMask]->sprite->SetAlpha(0.975f);
        sprites_[Controller]->sprite->SetAlpha(1.0f);
        sprites_[AirController]->sprite->SetAlpha(1.0f);
        sprites_[MenuButton]->sprite->SetAlpha(0.0f);
    } else {
        sprites_[BlackMask]->sprite->SetAlpha(0.0f);
        sprites_[Controller]->sprite->SetAlpha(0.0f);
        sprites_[AirController]->sprite->SetAlpha(0.0f);
        sprites_[MenuButton]->sprite->SetAlpha(1.0f);
    }
}

void GameUI::Draw() {
}
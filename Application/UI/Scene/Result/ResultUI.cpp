#include "ResultUI.h"
#include <Frame.h>

void ResultUI::Initialize() {
    SpriteManager::GetInstance()->SetSaveFolder("Result");
    SpriteManager::GetInstance()->LoadAllSprites();

    sprites_[kBackground] = SpriteManager::GetInstance()->GetSprite("ResultBackGround");
    sprites_[kBackground]->sprite->SetPosition({-1760.0f, 0.0f});
    positionEasings_[kBackground] = EasingData<Vector2>({-1760.0f, 0.0f}, {0.0f, 0.0f}, 1.5f, EasingType::InOutQuint);
}

void ResultUI::Update() {
    if (isStartEasing_) {
        positionEasings_[kBackground].isActive = true;
        sprites_[kBackground]->sprite->SetPosition(positionEasings_[kBackground].Update(Frame::DeltaTime()));
    }
}

void ResultUI::Draw() {
}

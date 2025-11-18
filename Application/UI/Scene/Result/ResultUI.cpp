#include "ResultUI.h"
#include <Frame.h>

void ResultUI::Initialize() {
    SpriteManager::GetInstance()->SetSaveFolder("Result");
    SpriteManager::GetInstance()->LoadAllSprites();

    sprites_[kBackground] = SpriteManager::GetInstance()->GetSprite("ResultBackGround");
    sprites_[kResult] = SpriteManager::GetInstance()->GetSprite("Result");
    sprites_[kClearTime] = SpriteManager::GetInstance()->GetSprite("ClearTime");
    sprites_[kHP] = SpriteManager::GetInstance()->GetSprite("HP");
    sprites_[kRank] = SpriteManager::GetInstance()->GetSprite("Rank");

    for (int i = 0; i < kMaxSprite; ++i) {
        endPositions_[i] = sprites_[i]->sprite->GetPosition();
    }

    sprites_[kBackground]->sprite->SetPosition({-1760.0f, 0.0f});
    sprites_[kResult]->sprite->SetPosition({-640.0f, 65.0f});
    sprites_[kClearTime]->sprite->SetPosition({-650.0f, 350.0f});
    sprites_[kHP]->sprite->SetPosition({-710.0f, 575.0f});
    sprites_[kRank]->sprite->SetPosition({-710.0f, 800.0f});

    for (int i = 0; i < kMaxSprite; ++i) {
        startPositions_[i] = sprites_[i]->sprite->GetPosition();
        positionEasings_[i] = EasingData<Vector2>(startPositions_[i], endPositions_[i], 1.5f, EasingType::InOutQuint);
        positionEasings_[i].isActive = false;
    }

    currentEasingIndex_ = 0;
    delayTimer_ = 0.0f;
}

void ResultUI::Update() {
    if (isStartEasing_) {
        // 背景のイージング
        if (currentEasingIndex_ == kBackground) {
            positionEasings_[kBackground].isActive = true;
            sprites_[kBackground]->sprite->SetPosition(positionEasings_[kBackground].Update(Frame::DeltaTime()));

            // 背景のイージングが終了したら次へ
            if (positionEasings_[kBackground].IsFinished()) {
                currentEasingIndex_++;
                delayTimer_ = 0.0f;
            }
        }
        // 背景以降のスプライト
        else if (currentEasingIndex_ < kMaxSprite) {
            // 遅延タイマーを更新
            delayTimer_ += Frame::DeltaTime();

            // 遅延時間が経過したら次のスプライトを開始
            if (delayTimer_ >= kDelayTime) {
                positionEasings_[currentEasingIndex_].isActive = true;
                delayTimer_ = 0.0f;
                currentEasingIndex_++;
            }
        }

        // アクティブな全てのイージングを更新
        for (int i = 0; i < kMaxSprite; ++i) {
            if (positionEasings_[i].isActive || !positionEasings_[i].IsFinished()) {
                sprites_[i]->sprite->SetPosition(positionEasings_[i].Update(Frame::DeltaTime()));
            }
        }
    }
}

void ResultUI::Draw() {
}

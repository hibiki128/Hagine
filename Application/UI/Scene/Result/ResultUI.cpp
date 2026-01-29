#include "ResultUI.h"
#include <Frame.h>

using namespace Hagine;
using namespace Math;
using namespace Graphics;

void ResultUI::Initialize() {
    SpriteManager::GetInstance()->SetSaveFolder("Result");
    SpriteManager::GetInstance()->LoadAllSprites();

    sprites_[kBackground] = SpriteManager::GetInstance()->GetSprite("ResultBackGround");
    sprites_[kResult] = SpriteManager::GetInstance()->GetSprite("Result");
    sprites_[kClearTime] = SpriteManager::GetInstance()->GetSprite("ClearTime");
    sprites_[kMinTens] = SpriteManager::GetInstance()->GetSprite("minTens");
    sprites_[kMinOnes] = SpriteManager::GetInstance()->GetSprite("minOnes");
    sprites_[kCoron] = SpriteManager::GetInstance()->GetSprite("coron");
    sprites_[kSecTens] = SpriteManager::GetInstance()->GetSprite("secTens");
    sprites_[kSecOnes] = SpriteManager::GetInstance()->GetSprite("secOnes");
    sprites_[kHP] = SpriteManager::GetInstance()->GetSprite("HP");
    sprites_[kHPHund] = SpriteManager::GetInstance()->GetSprite("HpHund");
    sprites_[kHPTens] = SpriteManager::GetInstance()->GetSprite("HpTens");
    sprites_[kHPOnes] = SpriteManager::GetInstance()->GetSprite("HpOnes");
    sprites_[kPercent] = SpriteManager::GetInstance()->GetSprite("percent");
    sprites_[kRank] = SpriteManager::GetInstance()->GetSprite("Rank");

    for (int i = 0; i < kMaxSprite; ++i) {
        endPositions_[i] = sprites_[i]->sprite->GetPosition();
    }

    sprites_[kBackground]->sprite->SetPosition({-1760.0f, 0.0f});
    sprites_[kResult]->sprite->SetPosition({-640.0f, 65.0f});
    sprites_[kClearTime]->sprite->SetPosition({-650.0f, 350.0f});
    sprites_[kMinTens]->sprite->SetPosition({-710.0f, 385.0f});
    sprites_[kMinOnes]->sprite->SetPosition({-610.0f, 385.0f});
    sprites_[kCoron]->sprite->SetPosition({-510.0f, 385.0f});
    sprites_[kSecTens]->sprite->SetPosition({-450.0f, 385.0f});
    sprites_[kSecOnes]->sprite->SetPosition({-350.0f, 385.0f});
    sprites_[kHP]->sprite->SetPosition({-710.0f, 575.0f});
    sprites_[kHPHund]->sprite->SetPosition({-710.0f, 610.0f});
    sprites_[kHPTens]->sprite->SetPosition({-610.0f, 610.0f});
    sprites_[kHPOnes]->sprite->SetPosition({-510.0f, 610.0f});
    sprites_[kPercent]->sprite->SetPosition({-410.0f, 610.0f});
    sprites_[kRank]->sprite->SetPosition({-710.0f, 800.0f});

    for (int i = 0; i < kMaxSprite; ++i) {
        startPositions_[i] = sprites_[i]->sprite->GetPosition();
        positionEasings_[i] = EasingData<Vector2>(startPositions_[i], endPositions_[i], kEasingDuration, EasingType::InOutQuint);
        positionEasings_[i].isActive = false;
    }

    currentEasingIndex_ = 0;
    delayTimer_ = 0.0f;

    // 数字のカウントアップ用初期化
    numberAnimState_ = kWaiting;
    animTimer_ = 0.0f;
    displayedTime_ = 0.0f;
    displayedHP_ = 0.0f;

    UpdateNumberSprites();
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
        // Result~ClearTimeまで(個別表示)
        else if (currentEasingIndex_ <= kClearTime) {
            // 遅延タイマーを更新
            delayTimer_ += Frame::DeltaTime();

            // 遅延時間が経過したら次のスプライトを開始
            if (delayTimer_ >= kDelayTime) {
                positionEasings_[currentEasingIndex_].isActive = true;
                delayTimer_ = 0.0f;
                currentEasingIndex_++;

                // ClearTimeが出たら時間の数字を全て同時に表示開始
                if (currentEasingIndex_ > kClearTime) {
                    // 時間の数字を全て同時にイージング開始
                    for (int i = kMinTens; i <= kSecOnes; ++i) {
                        positionEasings_[i].isActive = true;
                    }
                    currentEasingIndex_ = kSecOnes + 1; // 次はHPへ
                    numberAnimState_ = kWaiting;        // まだ待機状態
                    delayTimer_ = 0.0f;
                }
            }
        }
        // 時間の数字のイージング完了待ち
        else if (currentEasingIndex_ == kSecOnes + 1 && numberAnimState_ == kWaiting) {
            // 時間の数字のイージングが全て完了したかチェック
            bool allFinished = true;
            for (int i = kMinTens; i <= kSecOnes; ++i) {
                if (!positionEasings_[i].IsFinished()) {
                    allFinished = false;
                    break;
                }
            }

            // 全てのイージングが完了したらカウントアップ開始
            if (allFinished) {
                numberAnimState_ = kAnimatingTime;
                animTimer_ = 0.0f;
            }
        }
        // HPテキスト表示(タイムのカウントアップ完了待ち)
        else if (currentEasingIndex_ == kSecOnes + 1 && numberAnimState_ == kWaitingForHP) {
            // 遅延タイマーを更新
            delayTimer_ += Frame::DeltaTime();

            // 遅延時間が経過したらHPテキストと数字を同時に表示
            if (delayTimer_ >= kDelayTime) {
                positionEasings_[kHP].isActive = true;
                // HPの数字を全て同時にイージング開始
                for (int i = kHPHund; i <= kPercent; ++i) {
                    positionEasings_[i].isActive = true;
                }
                currentEasingIndex_ = kPercent + 1; // 次はRankへ
                numberAnimState_ = kWaiting;        // イージング完了待ち
                delayTimer_ = 0.0f;
            }
        }
        // HPの数字のイージング完了待ち
        else if (currentEasingIndex_ == kPercent + 1 && numberAnimState_ == kWaiting) {
            // HPの数字のイージングが全て完了したかチェック
            bool allFinished = positionEasings_[kHP].IsFinished();
            for (int i = kHPHund; i <= kPercent; ++i) {
                if (!positionEasings_[i].IsFinished()) {
                    allFinished = false;
                    break;
                }
            }

            // 全てのイージングが完了したらカウントアップ開始
            if (allFinished) {
                numberAnimState_ = kAnimatingHP;
                animTimer_ = 0.0f;
            }
        }
        // ランク(HPアニメーション後)
        else if (currentEasingIndex_ < kMaxSprite) {
            // HPのアニメーションが終わるまで待機
            if (numberAnimState_ == kFinished) {
                // 遅延タイマーを更新
                delayTimer_ += Frame::DeltaTime();

                // 遅延時間が経過したらランクを表示
                if (delayTimer_ >= kDelayTime) {
                    positionEasings_[currentEasingIndex_].isActive = true;
                    delayTimer_ = 0.0f;
                    currentEasingIndex_++;
                }
            }
        }

        // アクティブな全てのイージングを更新
        for (int i = 0; i < kMaxSprite; ++i) {
            if (positionEasings_[i].isActive || !positionEasings_[i].IsFinished()) {
                sprites_[i]->sprite->SetPosition(positionEasings_[i].Update(Frame::DeltaTime()));
            }
        }
    }

    // 数字のカウントアップアニメーション
    if (numberAnimState_ == kAnimatingTime) {
        animTimer_ += Frame::DeltaTime();
        float t = animTimer_ / kAnimDuration;

        if (t >= kNormalizeValue) {
            t = kNormalizeValue;
            displayedTime_ = ClearTime_;
            numberAnimState_ = kWaitingForHP;
            delayTimer_ = 0.0f;
        } else {
            displayedTime_ = ClearTime_ * t;
        }
    } else if (numberAnimState_ == kAnimatingHP) {
        animTimer_ += Frame::DeltaTime();
        float t = animTimer_ / kAnimDuration;

        if (t >= kNormalizeValue) {
            t = kNormalizeValue;
            displayedHP_ = HP_;
            numberAnimState_ = kFinished;
            delayTimer_ = 0.0f;
        } else {
            displayedHP_ = HP_ * t;
        }
    }

    UpdateNumberSprites();

    if (currentEasingIndex_ >= kMaxSprite && numberAnimState_ == kFinished) {
        bool allFinished = true;
        for (int i = 0; i < kMaxSprite; ++i) {
            if (!positionEasings_[i].IsFinished()) {
                allFinished = false;
                break;
            }
        }
        isAllAnimationFinished_ = allFinished;
    }
}

void ResultUI::UpdateNumberSprites() {
    // 表示用の値を使用(カウントアップアニメーション用)
    int totalSeconds = static_cast<int>(displayedTime_);
    int minutes = totalSeconds / kSecondsPerMinute;
    int seconds = totalSeconds % kSecondsPerMinute;

    // 分の十の位
    int minTens = minutes / kDigitDivisor;
    sprites_[kMinTens]->sprite->SetUVPosition({static_cast<float>(minTens) * kUVStep, 0.0f});
    if (minTens == kZeroValue) {
        sprites_[kMinTens]->sprite->SetAlpha(kAlphaInvisible);
    } else {
        sprites_[kMinTens]->sprite->SetAlpha(kAlphaVisible);
    }

    // 分の一の位
    int minOnes = minutes % kDigitDivisor;
    sprites_[kMinOnes]->sprite->SetUVPosition({static_cast<float>(minOnes) * kUVStep, 0.0f});
    sprites_[kMinOnes]->sprite->SetAlpha(kAlphaVisible);

    // 秒の十の位
    int secTens = seconds / kDigitDivisor;
    sprites_[kSecTens]->sprite->SetUVPosition({static_cast<float>(secTens) * kUVStep, 0.0f});

    // 秒の一の位
    int secOnes = seconds % kDigitDivisor;
    sprites_[kSecOnes]->sprite->SetUVPosition({static_cast<float>(secOnes) * kUVStep, 0.0f});

    // HPの各桁を計算(表示用の値を使用)
    int hp = static_cast<int>(displayedHP_);

    // HPの百の位
    int hpHund = hp / kHundredDivisor;
    sprites_[kHPHund]->sprite->SetUVPosition({static_cast<float>(hpHund) * kUVStep, 0.0f});
    if (hpHund == kZeroValue) {
        sprites_[kHPHund]->sprite->SetAlpha(kAlphaInvisible);
    } else {
        sprites_[kHPHund]->sprite->SetAlpha(kAlphaVisible);
    }

    // HPの十の位
    int hpTens = (hp % kHundredDivisor) / kDigitDivisor;
    sprites_[kHPTens]->sprite->SetUVPosition({static_cast<float>(hpTens) * kUVStep, 0.0f});
    // 百の位が0で十の位も0なら非表示
    if (hpHund == kZeroValue && hpTens == kZeroValue) {
        sprites_[kHPTens]->sprite->SetAlpha(kAlphaInvisible);
    } else {
        sprites_[kHPTens]->sprite->SetAlpha(kAlphaVisible);
    }

    // HPの一の位
    int hpOnes = hp % kDigitDivisor;
    sprites_[kHPOnes]->sprite->SetUVPosition({static_cast<float>(hpOnes) * kUVStep, 0.0f});
}

void ResultUI::Draw() {
}
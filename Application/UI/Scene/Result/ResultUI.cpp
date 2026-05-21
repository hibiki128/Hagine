#include "ResultUI.h"
#include <Frame.h>
#include <GamePad.h>
#include <Input.h>

void ResultUI::Initialize() {
    SpriteManager::GetInstance()->SetSaveFolder("Result");
    SpriteManager::GetInstance()->LoadAllSprites();

    // スプライトの取得
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

    // 終了位置(本来の位置)を保存
    for (int i = 0; i < kMaxSprite; ++i) {
        endPositions_[i] = sprites_[i]->sprite->GetPosition();
    }

    // 各スプライトの開始位置(画面外)を設定
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

    // イージングデータの初期化
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

    // スキップ管理の初期化
    skipPhase_ = kNoSkip;

    UpdateNumberSprites();

    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);

    input_ = Input::GetInstance();
}

bool ResultUI::CheckSkipInput() {
    if (!input_ || !gamePad_) {
        return false;
    }

    if (!gamePad_->IsConnected()) {
        // キーボード操作(スペースキー)
        return input_->TriggerKey(DIK_SPACE);
    } else {
        // コントローラー操作(Aボタン)
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_A);
    }
}

void ResultUI::SkipTimeAnimation() {
    // クリアタイムのカウントアップを即座に完了
    displayedTime_ = ClearTime_;
    animTimer_ = kAnimDuration;
    numberAnimState_ = kWaitingForHP;
    delayTimer_ = 0.0f;
    skipPhase_ = kSkipTime;
    UpdateNumberSprites();
}

void ResultUI::SkipHPAnimation() {
    // HP関連のイージングを即座に完了
    positionEasings_[kHP].isActive = true;
    sprites_[kHP]->sprite->SetPosition(endPositions_[kHP]);
    positionEasings_[kHP].time = kEasingDuration;

    for (int i = kHPHund; i <= kPercent; ++i) {
        positionEasings_[i].isActive = true;
        sprites_[i]->sprite->SetPosition(endPositions_[i]);
        positionEasings_[i].time = kEasingDuration;
    }

    // HPのカウントアップを即座に完了
    displayedHP_ = HP_;
    animTimer_ = kAnimDuration;
    numberAnimState_ = kFinished;
    delayTimer_ = 0.0f;
    currentEasingIndex_ = kPercent + 1;
    skipPhase_ = kSkipHP;
    UpdateNumberSprites();
}

void ResultUI::SkipRankDisplay() {
    // ランクの表示を即座に完了
    positionEasings_[kRank].isActive = true;
    sprites_[kRank]->sprite->SetPosition(endPositions_[kRank]);
    positionEasings_[kRank].time = kEasingDuration;
    currentEasingIndex_ = kMaxSprite;
    skipPhase_ = kAllSkipped;
}

void ResultUI::Update() {
    if (isStartEasing_) {
        gamePad_->Update();
        // スキップ入力チェック
        if (CheckSkipInput()) {
            if (skipPhase_ == kNoSkip) {
                // 最初の入力：クリアタイムのアニメーションをスキップ
                if (numberAnimState_ == kAnimatingTime) {
                    SkipTimeAnimation();
                }
            } else if (skipPhase_ == kSkipTime) {
                // 2回目の入力：HPのアニメーションをスキップ
                if (numberAnimState_ == kWaitingForHP || numberAnimState_ == kWaiting || numberAnimState_ == kAnimatingHP) {
                    SkipHPAnimation();
                }
            } else if (skipPhase_ == kSkipHP) {
                // 3回目の入力：ランクの表示をスキップ
                if (currentEasingIndex_ < kMaxSprite || numberAnimState_ == kFinished) {
                    SkipRankDisplay();
                }
            }
        }

        // 背景のイージング
        if (currentEasingIndex_ == kBackground) {
            positionEasings_[kBackground].isActive = true;
            sprites_[kBackground]->sprite->SetPosition(positionEasings_[kBackground].Update(Frame::DeltaTime()));

            if (positionEasings_[kBackground].IsFinished()) {
                currentEasingIndex_++;
                delayTimer_ = 0.0f;
            }
        }
        // Result～ClearTimeまで
        else if (currentEasingIndex_ <= kClearTime) {
            delayTimer_ += Frame::DeltaTime();

            // 遅延時間が経過したら次のスプライトのイージングを開始
            if (delayTimer_ >= kDelayTime) {
                positionEasings_[currentEasingIndex_].isActive = true;
                delayTimer_ = 0.0f;
                currentEasingIndex_++;

                // ClearTimeが出たら時間の数字を全て同時に表示開始
                if (currentEasingIndex_ > kClearTime) {
                    for (int i = kMinTens; i <= kSecOnes; ++i) {
                        positionEasings_[i].isActive = true;
                    }
                    currentEasingIndex_ = kSecOnes + 1;
                    numberAnimState_ = kWaiting;
                    delayTimer_ = 0.0f;
                }
            }
        }
        // 時間の数字のイージング完了待ち
        else if (currentEasingIndex_ == kSecOnes + 1 && numberAnimState_ == kWaiting) {
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
        // HPテキスト表示
        else if (currentEasingIndex_ == kSecOnes + 1 && numberAnimState_ == kWaitingForHP) {
            delayTimer_ += Frame::DeltaTime();

            // 遅延時間が経過したらHPテキストと数字を同時に表示
            if (delayTimer_ >= kDelayTime) {
                positionEasings_[kHP].isActive = true;
                for (int i = kHPHund; i <= kPercent; ++i) {
                    positionEasings_[i].isActive = true;
                }
                currentEasingIndex_ = kPercent + 1;
                numberAnimState_ = kWaiting;
                delayTimer_ = 0.0f;
            }
        }
        // HPの数字のイージング完了待ち
        else if (currentEasingIndex_ == kPercent + 1 && numberAnimState_ == kWaiting) {
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
        // ランクの表示
        else if (currentEasingIndex_ < kMaxSprite) {
            if (numberAnimState_ == kFinished) {
                delayTimer_ += Frame::DeltaTime();

                if (delayTimer_ >= kDelayTime) {
                    positionEasings_[currentEasingIndex_].isActive = true;
                    delayTimer_ = 0.0f;
                    currentEasingIndex_++;
                }
            }
        }

        // アクティブなイージングの更新
        for (int i = 0; i < kMaxSprite; ++i) {
            if (positionEasings_[i].isActive || !positionEasings_[i].IsFinished()) {
                sprites_[i]->sprite->SetPosition(positionEasings_[i].Update(Frame::DeltaTime()));
            }
        }
    }

    // 数字のカウントアップアニメーションの更新
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

    // 全てのアニメーションが終了したか判定
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
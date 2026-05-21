#include "GameUI.h"
#include <Input.h>

void GameUI::Initialize() {
    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);
    SpriteManager::GetInstance()->SetSaveFolder("GameScene");
    SpriteManager::GetInstance()->LoadAllSprites();

    // スプライトの取得
    sprites_[MenuButton] = SpriteManager::GetInstance()->GetSprite("menuButton");
    sprites_[Controller] = SpriteManager::GetInstance()->GetSprite("controllerIcon");
    sprites_[BlackMask] = SpriteManager::GetInstance()->GetSprite("BlackMask");
    sprites_[AirController] = SpriteManager::GetInstance()->GetSprite("AirControllerIcon");
    sprites_[SkillButton] = SpriteManager::GetInstance()->GetSprite("skillButton");
    sprites_[backMenuText] = SpriteManager::GetInstance()->GetSprite("backMenuText");
    sprites_[BackMenuUIBar] = SpriteManager::GetInstance()->GetSprite("BackMenuUIBar");
    sprites_[backTitleText] = SpriteManager::GetInstance()->GetSprite("backTitleText");
    sprites_[BackTitleUIBar] = SpriteManager::GetInstance()->GetSprite("BackTitleUIBar");
    sprites_[explanationText] = SpriteManager::GetInstance()->GetSprite("explanationText");
    sprites_[ExplanationUIBar] = SpriteManager::GetInstance()->GetSprite("ExplanationUIBar");
    sprites_[MenuBackGround] = SpriteManager::GetInstance()->GetSprite("MenuBackGround");
    sprites_[AButton] = SpriteManager::GetInstance()->GetSprite("AButton");
    sprites_[decision] = SpriteManager::GetInstance()->GetSprite("decision");
    sprites_[BButton] = SpriteManager::GetInstance()->GetSprite("BButton");
    sprites_[back] = SpriteManager::GetInstance()->GetSprite("back");

    // 目標位置とデフォルトサイズを保存
    for (int i = 0; i < kMaxSprite; ++i) {
        if (sprites_[i] && sprites_[i]->sprite) {
            targetPositions_[i] = sprites_[i]->sprite->GetPosition();
            defaultSizes_[i] = sprites_[i]->sprite->GetSize();
        }
    }

    // 初期状態の設定
    if (sprites_[MenuButton] && sprites_[MenuButton]->sprite) {
        sprites_[MenuButton]->sprite->SetAlpha(1.0f);
    }

    // 操作説明画面用スプライトの初期化
    std::array<SpriteIndex, 5> explanationSprites = {
        SkillButton, Controller, AirController, BButton, back};

    for (auto index : explanationSprites) {
        if (sprites_[index] && sprites_[index]->sprite) {
            sprites_[index]->sprite->SetAlpha(0.0f);
            Vector2 startPos = targetPositions_[index];
            startPos.y += kStartOffsetY;
            sprites_[index]->sprite->SetPosition(startPos);
        }
    }

    // メニューUI要素の初期化
    std::array<SpriteIndex, 9> menuElements = {
        backMenuText, BackMenuUIBar, backTitleText, BackTitleUIBar,
        explanationText, ExplanationUIBar, MenuBackGround, AButton, decision};

    for (auto index : menuElements) {
        if (sprites_[index] && sprites_[index]->sprite) {
            sprites_[index]->sprite->SetAlpha(0.0f);
            Vector2 startPos = targetPositions_[index];
            startPos.y += kStartOffsetY;
            sprites_[index]->sprite->SetPosition(startPos);
        }
    }

    // 暗転マスクの初期化
    if (sprites_[BlackMask] && sprites_[BlackMask]->sprite) {
        sprites_[BlackMask]->sprite->SetAlpha(0.0f);
    }

    prevIsPause_ = false;
    isBackTitle_ = false;
    menuState_ = MainMenu;
    transitionState_ = TransitionState::None;
    currentMenuItem_ = 0;
}

void GameUI::Update() {
    gamePad_->Update();

    // 入力クールダウン更新
    if (inputCooldown_ > 0.0f) {
        inputCooldown_ -= 1.0f / 60.0f;
    }

    // ポーズボタン入力検出
    if (!isTutorial_) {
        if (!isBackTitle_) {
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
        }
    }

    // ポーズ状態が変化したらアニメーション開始
    if (isPause_ != prevIsPause_) {
        if (isPause_) {
            // ポーズメニューを開くアニメーション
            std::array<SpriteIndex, 9> menuElements = {
                backMenuText, BackMenuUIBar, backTitleText, BackTitleUIBar,
                explanationText, ExplanationUIBar, MenuBackGround, AButton, decision};

            for (auto index : menuElements) {
                if (sprites_[index] && sprites_[index]->sprite) {
                    Vector2 startPos = targetPositions_[index];
                    startPos.y += kStartOffsetY;
                    Vector2 endPos = targetPositions_[index];

                    animations_[index].position.Reset(startPos, endPos, kAnimationDuration, EasingType::OutCubic);
                    animations_[index].alpha.Reset(0.0f, 1.0f, kAnimationDuration, EasingType::OutCubic);
                }
            }

            // 暗転マスクのフェードイン
            if (sprites_[BlackMask] && sprites_[BlackMask]->sprite) {
                animations_[BlackMask].alpha.Reset(0.0f, 0.975f, kAnimationDuration, EasingType::OutCubic);
            }

            // メニューボタンのフェードアウト
            if (sprites_[MenuButton] && sprites_[MenuButton]->sprite) {
                animations_[MenuButton].alpha.Reset(1.0f, 0.0f, kAnimationDuration, EasingType::OutCubic);
            }

            // 状態のリセット
            menuState_ = MainMenu;
            currentMenuItem_ = 0;
            isBackTitle_ = false;
            transitionState_ = TransitionState::None;

        } else {
            // ポーズメニューを閉じるアニメーション
            std::array<SpriteIndex, 9> menuElements = {
                backMenuText, BackMenuUIBar, backTitleText, BackTitleUIBar,
                explanationText, ExplanationUIBar, MenuBackGround, AButton, decision};

            for (auto index : menuElements) {
                if (sprites_[index] && sprites_[index]->sprite) {
                    Vector2 startPos = sprites_[index]->sprite->GetPosition();
                    Vector2 endPos = targetPositions_[index];
                    endPos.y += kEndOffsetY;

                    animations_[index].position.Reset(startPos, endPos, kAnimationDuration, EasingType::InCubic);
                    animations_[index].alpha.Reset(sprites_[index]->sprite->GetColor().w, 0.0f, kAnimationDuration, EasingType::InCubic);
                }
            }

            // 暗転マスクのフェードアウト
            if (sprites_[BlackMask] && sprites_[BlackMask]->sprite) {
                animations_[BlackMask].alpha.Reset(0.975f, 0.0f, kAnimationDuration, EasingType::InCubic);
            }

            // メニューボタンのフェードイン
            if (sprites_[MenuButton] && sprites_[MenuButton]->sprite) {
                animations_[MenuButton].alpha.Reset(0.0f, 1.0f, kAnimationDuration, EasingType::InCubic);
            }

            // 操作説明スプライトを非表示
            std::array<SpriteIndex, 5> explanationSprites = {
                Controller, AirController, SkillButton, BButton, back};

            for (auto index : explanationSprites) {
                if (sprites_[index] && sprites_[index]->sprite) {
                    sprites_[index]->sprite->SetAlpha(0.0f);
                }
            }
        }

        prevIsPause_ = isPause_;
    }

    // ポーズ中の処理
    if (isPause_) {
        UpdateMenuSelection();
        UpdateMenuAnimation();

        // 遷移待ち処理
        if (transitionState_ == TransitionState::ToExplanation) {
            // メニュー背景が完全に消えたら説明を表示
            if (animations_[MenuBackGround].alpha.IsFinished() &&
                sprites_[MenuBackGround] && sprites_[MenuBackGround]->sprite &&
                sprites_[MenuBackGround]->sprite->GetColor().w <= 0.01f) {

                std::array<SpriteIndex, 5> explanationSprites = {
                    Controller, AirController, SkillButton, BButton, back};

                for (auto index : explanationSprites) {
                    if (sprites_[index] && sprites_[index]->sprite) {
                        Vector2 startPos = sprites_[index]->sprite->GetPosition();
                        startPos.y = targetPositions_[index].y + kStartOffsetY;
                        Vector2 endPos = targetPositions_[index];

                        sprites_[index]->sprite->SetPosition(startPos);
                        animations_[index].position.Reset(startPos, endPos, kAnimationDuration, EasingType::OutCubic);
                        animations_[index].alpha.Reset(0.0f, 1.0f, kAnimationDuration, EasingType::OutCubic);
                    }
                }
                transitionState_ = TransitionState::None;
            }
        } else if (transitionState_ == TransitionState::ToMain) {
            // 説明画像が完全に消えたらメインメニューを表示
            if (animations_[Controller].alpha.IsFinished() &&
                sprites_[Controller] && sprites_[Controller]->sprite &&
                sprites_[Controller]->sprite->GetColor().w <= 0.01f) {

                std::array<SpriteIndex, 8> mainMenuElements = {
                    backMenuText, BackMenuUIBar, backTitleText,
                    BackTitleUIBar, explanationText, ExplanationUIBar,
                    AButton, decision};

                for (auto index : mainMenuElements) {
                    if (sprites_[index] && sprites_[index]->sprite) {
                        animations_[index].alpha.Reset(0.0f, 1.0f, kAnimationDuration, EasingType::OutCubic);
                    }
                }
                if (sprites_[MenuBackGround] && sprites_[MenuBackGround]->sprite) {
                    animations_[MenuBackGround].alpha.Reset(0.0f, 1.0f, kAnimationDuration, EasingType::OutCubic);
                }
                transitionState_ = TransitionState::None;
            }
        }
    }

    // アニメーション更新
    float deltaTime = 1.0f / 60.0f;

    for (int i = 0; i < kMaxSprite; ++i) {
        if (sprites_[i] && sprites_[i]->sprite) {
            // 位置
            if (!animations_[i].position.IsFinished()) {
                Vector2 newPos = animations_[i].position.Update(deltaTime);
                sprites_[i]->sprite->SetPosition(newPos);
            }

            // 透明度
            if (!animations_[i].alpha.IsFinished()) {
                float newAlpha = animations_[i].alpha.Update(deltaTime);
                sprites_[i]->sprite->SetAlpha(newAlpha);
            }

            // スケール
            if (!animations_[i].scale.IsFinished()) {
                Vector2 newScale = animations_[i].scale.Update(deltaTime);
                sprites_[i]->sprite->SetSize(newScale);
            }
        }
    }
}

void GameUI::UpdateMenuSelection() {
    if (inputCooldown_ > 0.0f || transitionState_ != TransitionState::None) {
        return;
    }

    if (isBackTitle_) {
        return;
    }

    if (menuState_ == MainMenu) {
        if (IsUpPressed()) {
            currentMenuItem_--;
            if (currentMenuItem_ < 0) {
                currentMenuItem_ = maxMenuItem_ - 1;
            }
            inputCooldown_ = kInputCooldownTime;
        } else if (IsDownPressed()) {
            currentMenuItem_++;
            if (currentMenuItem_ >= maxMenuItem_) {
                currentMenuItem_ = 0;
            }
            inputCooldown_ = kInputCooldownTime;
        } else if (IsDecidePressed()) {
            if (currentMenuItem_ == 0) {
                isPause_ = false;
                inputCooldown_ = kInputCooldownTime;
            } else if (currentMenuItem_ == 1) {
                isBackTitle_ = true;
                inputCooldown_ = kInputCooldownTime;
            } else if (currentMenuItem_ == 2) {
                StartExplanationAnimation();
                inputCooldown_ = kInputCooldownTime;
            }
        }

    } else if (menuState_ == Explanation) {
        if (IsDecidePressed() || IsBackPressed()) {
            EndExplanationAnimation();
            inputCooldown_ = kInputCooldownTime;
        }
    }
}

void GameUI::UpdateMenuAnimation() {
    if (menuState_ == MainMenu) {
        std::array<std::pair<SpriteIndex, SpriteIndex>, 3> menuItems = {{{backMenuText, BackMenuUIBar},
                                                                         {backTitleText, BackTitleUIBar},
                                                                         {explanationText, ExplanationUIBar}}};

        for (int i = 0; i < 3; ++i) {
            auto [textIndex, barIndex] = menuItems[i];
            bool isSelected = (i == currentMenuItem_);

            // テキストのスケール
            if (sprites_[textIndex] && sprites_[textIndex]->sprite) {
                Vector2 targetScale = defaultSizes_[textIndex];
                if (isSelected) {
                    targetScale.x *= kSelectedScale;
                    targetScale.y *= kSelectedScale;
                }

                Vector2 currentSize = sprites_[textIndex]->sprite->GetSize();
                if (!animations_[textIndex].scale.isActive ||
                    (animations_[textIndex].scale.end.x != targetScale.x)) {
                    animations_[textIndex].scale.Reset(currentSize, targetScale,
                                                       kScaleAnimationDuration, EasingType::OutCubic);
                }
            }

            // UIBarのスケールと色
            if (sprites_[barIndex] && sprites_[barIndex]->sprite) {
                Vector2 targetScale = defaultSizes_[barIndex];
                Vector3 targetColor = kNormalColor;

                if (isSelected) {
                    targetScale.x *= kSelectedScale;
                    targetScale.y *= kSelectedScale;
                    targetColor = kSelectedColor;
                }

                Vector2 currentSize = sprites_[barIndex]->sprite->GetSize();
                if (!animations_[barIndex].scale.isActive ||
                    (animations_[barIndex].scale.end.x != targetScale.x)) {
                    animations_[barIndex].scale.Reset(currentSize, targetScale,
                                                      kScaleAnimationDuration, EasingType::OutCubic);
                }

                sprites_[barIndex]->sprite->SetColor(targetColor);
            }
        }
    }
}

void GameUI::StartExplanationAnimation() {
    menuState_ = Explanation;
    transitionState_ = TransitionState::ToExplanation;

    // メインメニュー要素をフェードアウト
    std::array<SpriteIndex, 8> mainMenuElements = {
        backMenuText, BackMenuUIBar, backTitleText,
        BackTitleUIBar, explanationText, ExplanationUIBar,
        AButton, decision};

    for (auto index : mainMenuElements) {
        if (sprites_[index] && sprites_[index]->sprite) {
            float currentAlpha = sprites_[index]->sprite->GetColor().w;
            animations_[index].alpha.Reset(currentAlpha, 0.0f, kFadeOutDuration, EasingType::OutCubic);
        }
    }

    if (sprites_[MenuBackGround] && sprites_[MenuBackGround]->sprite) {
        float currentAlpha = sprites_[MenuBackGround]->sprite->GetColor().w;
        animations_[MenuBackGround].alpha.Reset(currentAlpha, 0.0f, kFadeOutDuration, EasingType::OutCubic);
    }
}

void GameUI::EndExplanationAnimation() {
    menuState_ = MainMenu;
    transitionState_ = TransitionState::ToMain;

    // 操作説明スプライトをフェードアウト
    std::array<SpriteIndex, 5> explanationSprites = {
        Controller, AirController, SkillButton, BButton, back};

    for (auto index : explanationSprites) {
        if (sprites_[index] && sprites_[index]->sprite) {
            Vector2 startPos = sprites_[index]->sprite->GetPosition();
            Vector2 endPos = targetPositions_[index];
            endPos.y += kEndOffsetY;

            animations_[index].position.Reset(startPos, endPos, kAnimationDuration, EasingType::InCubic);
            animations_[index].alpha.Reset(sprites_[index]->sprite->GetColor().w, 0.0f,
                                           kFadeOutDuration, EasingType::InCubic);
        }
    }
}

bool GameUI::IsUpPressed() {
    if (!gamePad_->IsConnected()) {
        return Input::GetInstance()->TriggerKey(DIK_DOWN) || Input::GetInstance()->TriggerKey(DIK_S);
    } else {
        bool stickUp = gamePad_->GetLeftStickY() < -0.5f;
        bool dpadUp = gamePad_->IsTrigger(XINPUT_GAMEPAD_DPAD_DOWN);

        static float prevStickY = 0.0f;
        bool stickTrigger = stickUp && prevStickY >= -0.5f;
        prevStickY = gamePad_->GetLeftStickY();

        return stickTrigger || dpadUp;
    }
}

bool GameUI::IsDownPressed() {
    if (!gamePad_->IsConnected()) {
        return Input::GetInstance()->TriggerKey(DIK_UP) || Input::GetInstance()->TriggerKey(DIK_W);
    } else {
        bool stickDown = gamePad_->GetLeftStickY() > 0.5f;
        bool dpadDown = gamePad_->IsTrigger(XINPUT_GAMEPAD_DPAD_UP);

        static float prevStickY = 0.0f;
        bool stickTrigger = stickDown && prevStickY <= 0.5f;
        prevStickY = gamePad_->GetLeftStickY();

        return stickTrigger || dpadDown;
    }
}

bool GameUI::IsDecidePressed() {
    if (!gamePad_->IsConnected()) {
        return Input::GetInstance()->TriggerKey(DIK_SPACE);
    } else {
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_A);
    }
}

bool GameUI::IsBackPressed() {
    if (!gamePad_->IsConnected()) {
        return Input::GetInstance()->TriggerKey(DIK_SPACE);
    } else {
        return gamePad_->IsTrigger(XINPUT_GAMEPAD_B);
    }
}

void GameUI::Draw() {
}
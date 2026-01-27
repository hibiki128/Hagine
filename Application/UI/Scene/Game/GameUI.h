#pragma once
#include "Easing.h"
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
        SkillButton,
        Controller,
        AirController,
        BlackMask,
        backMenuText,
        BackMenuUIBar,
        backTitleText,
        BackTitleUIBar,
        explanationText,
        ExplanationUIBar,
        MenuBackGround,
        back,
        BButton,
        decision,
        AButton,
        kMaxSprite,
    };

    enum MenuState {
        MainMenu,    // メインメニュー
        Explanation, // 操作説明画面
    };

    // 画面遷移の待機状態
    enum class TransitionState {
        None,
        ToExplanation, // メインメニューが消えるのを待って説明を表示
        ToMain,        // 説明が消えるのを待ってメインメニューを表示
    };

    bool GetIsPause() const { return isPause_; }

    // タイトルに戻るフラグのGetter
    bool GetIsBackTitle() const { return isBackTitle_; }

  private:
    // メニュー選択の処理
    void UpdateMenuSelection();
    void UpdateMenuAnimation();
    void StartExplanationAnimation();
    void EndExplanationAnimation();

    // 入力検出
    bool IsUpPressed();
    bool IsDownPressed();
    bool IsDecidePressed();
    bool IsBackPressed();

    bool isPause_ = false;
    bool prevIsPause_ = false;

    // タイトルへ戻る選択状態
    bool isBackTitle_ = false;

    // メニュー状態
    MenuState menuState_ = MainMenu;
    TransitionState transitionState_ = TransitionState::None;

    int currentMenuItem_ = 0;
    int maxMenuItem_ = 3;

    // 入力の連続防止用
    float inputCooldown_ = 0.0f;
    const float kInputCooldownTime = 0.15f;

    std::unique_ptr<GamePad> gamePad_ = nullptr;
    std::array<SpriteData *, kMaxSprite> sprites_;

    // アニメーション用のイージングデータ
    struct UIElementAnimation {
        EasingData<Vector2> position;
        EasingData<float> alpha;
        EasingData<Vector2> scale;
    };

    std::array<UIElementAnimation, kMaxSprite> animations_;

    // 目標位置とスケールを保存
    std::array<Vector2, kMaxSprite> targetPositions_;
    std::array<Vector2, kMaxSprite> defaultSizes_;

    // アニメーション設定
    const float kAnimationDuration = 0.5f; // 0.5秒
    const float kFadeOutDuration = 0.2f;   // 消えるときは少し早く
    const float kScaleAnimationDuration = 0.2f;

    // 定数
    const float kStartOffsetY = -50.0f; // 出現時のオフセット
    const float kEndOffsetY = 100.0f;   // 消去時のオフセット
    const float kSelectedScale = 1.05f;  // 選択時の拡大率
    const Vector3 kNormalColor = {1.0f, 1.0f, 1.0f};

    const Vector3 kSelectedColor = {1.0f, 1.0f, 0.5f};
};
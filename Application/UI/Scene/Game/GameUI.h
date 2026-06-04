#pragma once
#include "Easing.h"
#include <GamePad.h>
#include <SpriteManager.h>
#include <array>

/// <summary>
/// ゲーム中のUI(ポーズメニューなど)を管理するクラス
/// </summary>
class GameUI {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// スプライトのインデックス
    /// </summary>
    enum SpriteIndex {
        MenuButton,       // メニューボタン
        SkillButton,      // スキルボタン
        Controller,       // コントローラー(地上)
        AirController,    // コントローラー(空中)
        BlackMask,        // 暗転用マスク
        backMenuText,     // メニューに戻るテキスト
        BackMenuUIBar,    // メニューに戻るバー
        backTitleText,    // タイトルに戻るテキスト
        BackTitleUIBar,   // タイトルに戻るバー
        explanationText,  // 操作説明テキスト
        ExplanationUIBar, // 操作説明バー
        MenuBackGround,   // メニュー背景
        back,             // 戻るテキスト
        BButton,          // Bボタン
        decision,         // 決定テキスト
        AButton,          // Aボタン
        kMaxSprite,       // 最大数
    };

    /// <summary>
    /// メニューの状態
    /// </summary>
    enum MenuState {
        MainMenu,    // メインメニュー
        Explanation, // 操作説明画面
    };

    /// <summary>
    /// 画面遷移の待機状態
    /// </summary>
    enum class TransitionState {
        None,          // なし
        ToExplanation, // 操作説明へ
        ToMain,        // メインメニューへ
    };

    /// <summary>
    /// ポーズ中かどうかを取得
    /// </summary>
    /// <returns>ポーズ中ならtrue</returns>
    bool GetIsPause() const { return isPause_; }

    /// <summary>
    /// タイトルに戻るかどうかを取得
    /// </summary>
    /// <returns>タイトルに戻るならtrue</returns>
    bool GetIsBackTitle() const { return isBackTitle_; }

    /// <summary>
    /// チュートリアル中かどうかを設定
    /// </summary>
    /// <param name="isTutorial">チュートリアル中ならtrue</param>
    void SetIsTutorial(bool isTutorial) { isTutorial_ = isTutorial; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// メニュー選択の更新
    /// </summary>
    void UpdateMenuSelection();

    /// <summary>
    /// メニューアニメーションの更新
    /// </summary>
    void UpdateMenuAnimation();

    /// <summary>
    /// 操作説明アニメーションの開始
    /// </summary>
    void StartExplanationAnimation();

    /// <summary>
    /// 操作説明アニメーションの終了
    /// </summary>
    void EndExplanationAnimation();

    /// <summary>
    /// 上入力が押されたか
    /// </summary>
    /// <returns>押されたらtrue</returns>
    bool IsUpPressed();

    /// <summary>
    /// 下入力が押されたか
    /// </summary>
    /// <returns>押されたらtrue</returns>
    bool IsDownPressed();

    /// <summary>
    /// 決定ボタンが押されたか
    /// </summary>
    /// <returns>押されたらtrue</returns>
    bool IsDecidePressed();

    /// <summary>
    /// 戻るボタンが押されたか
    /// </summary>
    /// <returns>押されたらtrue</returns>
    bool IsBackPressed();

    /// ===================================================
    /// private variables
    /// ===================================================

    bool isPause_ = false;     // ポーズフラグ
    bool prevIsPause_ = false; // 前フレームのポーズフラグ

    bool isBackTitle_ = false; // タイトルに戻るフラグ
    bool isTutorial_ = false;  // チュートリアルフラグ

    MenuState menuState_ = MainMenu;                    // メニュー状態
    TransitionState transitionState_ = TransitionState::None; // 遷移状態

    int currentMenuItem_ = 0; // 現在の選択項目
    int maxMenuItem_ = 3;     // 最大項目数

    float inputCooldown_ = 0.0f;           // 入力クールダウン
    const float kInputCooldownTime = 0.15f; // 入力クールダウン時間

    std::unique_ptr<Hagine::GamePad> gamePad_ = nullptr;            // ゲームパッド
    std::array<Hagine::SpriteData *, kMaxSprite> sprites_; // スプライト配列

    /// <summary>
    /// UI要素のアニメーション構造体
    /// </summary>
    struct UIElementAnimation {
        Hagine::EasingData<Hagine::Vector2> position; // 位置
        Hagine::EasingData<float> alpha;      // 透明度
        Hagine::EasingData<Hagine::Vector2> scale;     // スケール
    };

    std::array<UIElementAnimation, kMaxSprite> animations_; // アニメーション配列

    std::array<Hagine::Vector2, kMaxSprite> targetPositions_; // 目標位置
    std::array<Hagine::Vector2, kMaxSprite> defaultSizes_;    // デフォルトサイズ

    const float kAnimationDuration = 0.5f;      // アニメーション時間
    const float kFadeOutDuration = 0.2f;        // フェードアウト時間
    const float kScaleAnimationDuration = 0.2f; // スケールアニメーション時間

    const float kStartOffsetY = -50.0f; // 出現時のオフセット
    const float kEndOffsetY = 100.0f;   // 消去時のオフセット
    const float kSelectedScale = 1.05f;  // 選択時の拡大率
    const Hagine::Vector3 kNormalColor = {1.0f, 1.0f, 1.0f}; // 通常時の色

    const Hagine::Vector3 kSelectedColor = {1.0f, 1.0f, 0.5f}; // 選択時の色
};
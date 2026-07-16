#pragma once
#include "2d/Sprite.h"
#include <memory>
#include <string>

class TutorialSystem;
namespace Hagine {
class DataHandler;
}
enum class TutorialStep : int;

/// <summary>
/// チュートリアルの操作ガイド・進行度バーなどのUI表示を管理するクラス
/// </summary>
class TutorialUI
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="system">チュートリアルシステムへのポインタ</param>
    /// <param name="okFontKey">OK!表示に使用するフォントキー</param>
    void Initialize(TutorialSystem *system, const std::string &okFontKey = "");

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <param name="dt">デルタタイム</param>
    void Update(float dt);

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// ImGuiデバッグ表示
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// シーン開始時のフェードインを開始する
    /// （パーティクル遷移の完了後に呼ぶことで、説明書きが遷移より先に出るのを防ぐ）
    /// </summary>
    void BeginIntroFadeIn();

    /// <summary>
    /// 完了したかを取得
    /// </summary>
    /// <returns>完了していたらtrue</returns>
    bool IsFinished() const { return isFinished_; }

  private:
    /// ===================================================
    /// private enum
    /// ===================================================

    /// <summary>
    /// トランジション状態
    /// </summary>
    enum class UITransitionState
    {
        Idle,            // 通常表示
        FadingOut,       // 旧ステップUIをフェードアウト中
        FadingIn,        // 新ステップUIをフェードイン中
        CompleteFadeOut, // 完了後の一括フェードアウト中
        CompleteDone,    // フェードアウト完了、待機中
    };

    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// トランジションの更新
    /// </summary>
    /// <param name="dt">デルタタイム</param>
    void UpdateTransition(float dt);

    /// <summary>
    /// 進捗バーの更新
    /// </summary>
    /// <param name="dt">デルタタイム</param>
    void UpdateProgressBar(float dt);

    /// <summary>
    /// 補足メッセージの更新
    /// </summary>
    void UpdateSubMessage();

    /// <summary>
    /// メータースプライトの更新
    /// </summary>
    void UpdateMeterSprites();

    /// <summary>
    /// ステップに応じたフォルダ名を取得
    /// </summary>
    /// <param name="step">チュートリアルステップ</param>
    /// <returns>フォルダ名</returns>
    const char *GetFolderNameForStep(TutorialStep step) const;

    /// <summary>
    /// 指定ステップのスプライトをロード
    /// </summary>
    /// <param name="step">ロードするステップ</param>
    void LoadStepSprites(TutorialStep step);

    /// <summary>
    /// 管理下にある全スプライトにアルファ値を適用
    /// </summary>
    /// <param name="alpha">適用するアルファ値</param>
    void ApplyAlphaToAllManagedSprites(float alpha);

    /// <summary>
    /// OK!スプライトの初期化
    /// </summary>
    /// <param name="fontKey">使用するフォントキー</param>
    void InitializeOKSprite(const std::string &fontKey);

    /// <summary>
    /// メーター設定のロード
    /// </summary>
    void LoadMeterSettings();

    /// <summary>
    /// メーター設定の保存
    /// </summary>
    void SaveMeterSettings();

    /// ===================================================
    /// private variables
    /// ===================================================

    TutorialSystem *pSystem_ = nullptr; // チュートリアルシステム

    UITransitionState transitionState_ = UITransitionState::Idle; // 遷移状態
    float fadeTimer_ = 0.0f;                                      // フェード用タイマー
    float fadeOutDuration_ = 0.35f;                               // フェードアウト時間
    float fadeInDuration_ = 0.45f;                                // フェードイン時間

    float completeFadeOutDuration_ = 1.0f; // 完了時フェードアウト時間
    float postCompleteTimer_ = 0.0f;       // 完了後タイマー
    float postCompleteDelay_ = 1.0f;       // 完了後の待機時間
    bool isFinished_ = false;              // 完了フラグ
    bool isIntroFade_ = false;             // シーン開始時のフェードイン中か（OK!表示を抑制する）

    float displayedProgress_ = 0.0f; // 表示用進捗率
    float progressLerpSpeed_ = 6.0f; // 進捗率の補間速度

    bool subMessageVisible_ = false; // 補足メッセージ表示フラグ
    float subMessageTimer_ = 0.0f;   // 補足メッセージタイマー

    Hagine::Sprite barSprite_;        // 進捗バー
    Hagine::Sprite frameSprite_;      // バーの枠
    Hagine::Sprite SkipButtonSprite_; // スキップボタン

    float barAlpha_ = 1.0f;   // バーのアルファ値
    float frameAlpha_ = 1.0f; // 枠のアルファ値

    Hagine::Sprite okSprite_;    // OK!スプライト
    bool okSpriteReady_ = false; // OK!準備完了フラグ
    float okAlpha_ = 0.0f;       // OK!のアルファ値

    Hagine::Vector2 okPosition_ = {640.0f, 400.0f}; // OK!の位置
    float okRotation_ = 0.0f;                       // OK!の回転
    Hagine::Vector2 okSize_ = {200.0f, 80.0f};      // OK!のサイズ

    Hagine::Vector2 barPosition_ = {100.0f, 880.0f}; // バーの位置
    float barHeight_ = 24.0f;                        // バーの高さ
    float barMaxWidth_ = 500.0f;                     // バーの最大幅
    float borderThickness_ = 3.0f;                   // 枠の太さ

    Hagine::Vector2 skipButtonPosition_ = {1600.0f, 946.0f}; // スキップボタンの位置
    Hagine::Vector2 skipButtonSize_ = {320.0f, 64.0f};       // スキップボタンのサイズ

    std::unique_ptr<Hagine::DataHandler> dataHandler_; // データハンドラ
};

#pragma once
#include "Engine/2d/Sprite.h"
#include <memory>
#include <string>

class TutorialSystem;
class DataHandler;
enum class TutorialStep : int;

// ============================================================
//  TutorialUI
//  チュートリアルの操作ガイド・進行度バーなどの
//  UI 表示を専門に管理するクラス。
//
//  ◆ ステップ切替時の演出フロー
//    1. FadingOut      : SpriteManager 管理スプライト全体を徐々に透明化
//                        → 進捗バーを 1.0 方向へ補間（完了を視覚的に表示）
//                        → OK! スプライトをフェードイン
//    2. (瞬間)         : 新ステップのスプライトをロード（alpha=0 でスタート）
//                        → 進捗バーを 0.0 にリセット
//    3. FadingIn       : 新ステップのスプライトを徐々に不透明化
//                        → OK! スプライトをフェードアウト
//                        （Complete ステップでは OK! を永続表示）
//    4. CompleteFadeOut: Complete ステップの FadingIn 完了後、
//                        バー・枠・OK! 含む全 UI を一括フェードアウト
//    5. CompleteDone   : フェードアウト完了後に postCompleteDelay_ 秒待機し、
//                        isFinished_ フラグを立てる
//
//  ◆ 直接保持スプライト（SpriteManager 管理外）
//    barSprite_  : 進捗バー本体（黄色）
//    frameSprite_: 進捗バーの黒枠
//    okSprite_   : 完了時の "OK!" テキスト（黄色・アウトライン付き）
// ============================================================
class TutorialUI {
  public:
    // ──────────────────────────────────────────────────────────
    //  公開インタフェース
    // ──────────────────────────────────────────────────────────

    /// 初期化（TutorialScene::Initialize 内で呼ぶ）
    void Initialize(TutorialSystem *system, const std::string &okFontKey = "");

    /// 毎フレーム更新（TutorialScene::Update 内で呼ぶ）
    void Update(float dt);

    /// 描画（TutorialScene::Draw 内で呼ぶ）
    /// 枠 → バー → OK! の順で描画する
    void Draw();

    /// 終了処理
    void Finalize();

    /// ImGui デバッグ/設定ウィンドウを表示する
    void DrawImGui();

    // ──────────────────────────────────────────────────────────
    //  Getter
    // ──────────────────────────────────────────────────────────

    /// チュートリアル完了後、postCompleteDelay_ 秒が経過したら true を返す。
    /// このフラグが立ったらシーン遷移などを行う。
    bool IsFinished() const { return isFinished_; }

  private:
    // ──────────────────────────────────────────────────────────
    //  トランジション状態
    // ──────────────────────────────────────────────────────────

    enum class UITransitionState {
        Idle,            ///< 通常表示（ステップ切替待機）
        FadingOut,       ///< 旧ステップ UI をフェードアウト中
        FadingIn,        ///< 新ステップ UI をフェードイン中
        CompleteFadeOut, ///< 完了後、バー・枠・OK! を一括フェードアウト中
        CompleteDone,    ///< フェードアウト完了、完了後タイマー計測中
    };

    // ──────────────────────────────────────────────────────────
    //  内部更新ロジック
    // ──────────────────────────────────────────────────────────

    /// フェードイン/アウトの状態機械を毎フレーム進める
    void UpdateTransition(float dt);

    /// 進捗バーの表示値を補間する（トランジション状態に応じてターゲットを切替）
    void UpdateProgressBar(float dt);

    /// 補足メッセージ（着地補正など）の表示を制御する
    void UpdateSubMessage();

    /// barSprite_ / frameSprite_ のサイズ・位置を現在の進捗に合わせて更新する
    void UpdateMeterSprites();

    // ──────────────────────────────────────────────────────────
    //  スプライト管理
    // ──────────────────────────────────────────────────────────

    /// ステップ番号 → 保存フォルダ名 変換（nullptr = 無効ステップ）
    const char *GetFolderNameForStep(TutorialStep step) const;

    /// 指定ステップのスプライトを SpriteManager 経由でロードする
    void LoadStepSprites(TutorialStep step);

    /// SpriteManager に登録されている全スプライトのアルファ値を一括設定する
    void ApplyAlphaToAllManagedSprites(float alpha);

    // ──────────────────────────────────────────────────────────
    //  OK! スプライト管理
    // ──────────────────────────────────────────────────────────

    /// TextRenderer で "OK!" テクスチャを生成し、okSprite_ を初期化する。
    void InitializeOKSprite(const std::string &fontKey);

    // ──────────────────────────────────────────────────────────
    //  DataHandler 操作
    // ──────────────────────────────────────────────────────────

    void LoadMeterSettings();
    void SaveMeterSettings();

    // ──────────────────────────────────────────────────────────
    //  参照
    // ──────────────────────────────────────────────────────────

    TutorialSystem *system_ = nullptr;

    // ──────────────────────────────────────────────────────────
    //  トランジション変数
    // ──────────────────────────────────────────────────────────

    UITransitionState transitionState_ = UITransitionState::Idle;
    float fadeTimer_ = 0.0f;        ///< フェードアウト/イン共用タイマー
    float fadeOutDuration_ = 0.35f; ///< フェードアウトにかける秒数
    float fadeInDuration_ = 0.45f;  ///< フェードインにかける秒数

    // ──────────────────────────────────────────────────────────
    //  完了フェードアウト・完了フラグ
    // ──────────────────────────────────────────────────────────

    float completeFadeOutDuration_ = 1.0f; ///< 完了後の全UI フェードアウト秒数
    float postCompleteTimer_ = 0.0f;       ///< フェードアウト完了後の経過秒数
    float postCompleteDelay_ = 1.0f;       ///< フラグが立つまでの待機秒数
    bool isFinished_ = false;              ///< 完了フラグ（postCompleteDelay_ 秒後に true）

    // ──────────────────────────────────────────────────────────
    //  進捗バー
    // ──────────────────────────────────────────────────────────

    float displayedProgress_ = 0.0f; ///< 表示値（実進捗を Lerp で追従）
    float progressLerpSpeed_ = 6.0f; ///< 補間速度

    // ──────────────────────────────────────────────────────────
    //  補足メッセージ
    // ──────────────────────────────────────────────────────────

    bool subMessageVisible_ = false;
    float subMessageTimer_ = 0.0f;

    // ──────────────────────────────────────────────────────────
    //  直接保持スプライト（SpriteManager 管理外）
    // ──────────────────────────────────────────────────────────

    Sprite barSprite_;        ///< 進捗を表す黄色バー本体
    Sprite frameSprite_;      ///< バーを囲む黒枠
    Sprite SkipButtonSprite_; ///< スキップ操作を示すボタンアイコン

    float barAlpha_ = 1.0f;   ///< バースプライトの現在アルファ（CompleteFadeOut で減衰）
    float frameAlpha_ = 1.0f; ///< 枠スプライトの現在アルファ（CompleteFadeOut で減衰）

    // ── OK! スプライト ──
    Sprite okSprite_;            ///< "OK!" テキストスプライト
    bool okSpriteReady_ = false; ///< InitializeOKSprite() が成功したか
    float okAlpha_ = 0.0f;       ///< 現在の表示アルファ（0=透明 / 1=不透明）

    // OK! スプライト設定（ImGui で調整・DataHandler でセーブ/ロード）
    Vector2 okPosition_ = {640.0f, 400.0f};
    float okRotation_ = 0.0f;
    Vector2 okSize_ = {200.0f, 80.0f};

    // ──────────────────────────────────────────────────────────
    //  メーター設定（DataHandler でセーブ/ロード）
    // ──────────────────────────────────────────────────────────

    Vector2 barPosition_ = {100.0f, 880.0f};
    float barHeight_ = 24.0f;
    float barMaxWidth_ = 500.0f;
    float borderThickness_ = 3.0f;

    Vector2 skipButtonPosition_ = {1600.0f, 946.0f};
    Vector2 skipButtonSize_ = {320.0f, 64.0f};

    std::unique_ptr<DataHandler> dataHandler_;
};
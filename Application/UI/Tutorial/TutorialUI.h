#pragma once

class TutorialSystem;
enum class TutorialStep : int;

// ============================================================
//  TutorialUI
//  チュートリアルの操作ガイド・進行度バーなどの
//  UI 表示を専門に管理するクラス。
//  スプライトの読み込み・切り替えは SpriteManager を通じて行う。
// ============================================================
class TutorialUI {
  public:
    /// 初期化（TutorialScene::Initialize 内で呼ぶ）
    void Initialize(TutorialSystem *system);

    /// 毎フレーム更新（TutorialScene::Update 内で呼ぶ）
    void Update(float dt);

    /// 描画（TutorialScene::Draw 内で SpriteManager::DrawAll の前に呼ぶ）
    void Draw();

    /// 終了処理
    void Finalize();

  private:
    // ─────────────────────────────────────────
    //  内部更新ロジック
    // ─────────────────────────────────────────

    /// テキスト・アイコンスプライトを現在のステップ内容に更新する
    void UpdateInstructionDisplay();

    /// 進行度バーの表示値を滑らかに補間して更新する
    void UpdateProgressBar(float dt);

    /// 補足メッセージ（着地補正など）の表示を制御する
    void UpdateSubMessage();

    // ─────────────────────────────────────────
    //  スプライト管理
    // ─────────────────────────────────────────

    /// ステップに対応するフォルダ名を返す
    /// Complete ステップは "TutorialFinish"、それ以外は "TutorialStep1" 〜 "TutorialStep14"
    /// StepCount など無効なステップの場合は nullptr を返す
    const char *GetFolderNameForStep(TutorialStep step) const;

    /// 指定ステップのスプライトを SpriteManager 経由でロードする
    void LoadStepSprites(TutorialStep step);

    // ─────────────────────────────────────────
    //  参照
    // ─────────────────────────────────────────

    TutorialSystem *system_ = nullptr;

    // ─────────────────────────────────────────
    //  内部変数
    // ─────────────────────────────────────────

    float displayedProgress_ = 0.0f; ///< バー表示用スムーズ補間値（実進捗に追従）
    float progressLerpSpeed_ = 5.0f; ///< 補間速度

    bool subMessageVisible_ = false; ///< 補足メッセージ表示中か
    float subMessageTimer_ = 0.0f;   ///< 補足メッセージの表示継続タイマー
};
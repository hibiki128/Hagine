#pragma once

class TutorialSystem;

// ============================================================
//  TutorialUI
//  チュートリアルの操作ガイド・進行度バーなどの
//  UI 表示を専門に管理するクラス。
//
//  【スプライト系APIについて】
//  Sprite / SpriteManager の API が確定次第、
//  下記の「TODO: Sprite 差し替え」コメント部分を実装してください。
//  現状は構造とロジックのみを提供します。
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

    // ─────────────────────────────────────────
    //  TODO: Sprite 差し替え（Sprite API 確定後に実装）
    // ─────────────────────────────────────────
    //
    //  以下のメンバを Sprite* または std::unique_ptr<Sprite> で宣言し、
    //  Initialize() で生成・位置設定、Draw() で描画してください。
    //
    //  ■ 操作指示ウィンドウ
    //    Sprite* instructionBgSprite_    ── 指示テキストの背景パネル
    //    Sprite* buttonIconSprite_       ── キー/ボタンのアイコン画像
    //                                       ステップ切替時に差し替える
    //
    //  ■ 進行度バー
    //    Sprite* progressBarBgSprite_    ── バー背景（固定）
    //    Sprite* progressBarFillSprite_  ── バー塗り部分（幅を displayedProgress_ で制御）
    //                                       例: fillSprite->SetSize({maxWidth * progress_, barHeight});
    //
    //  ■ 補足メッセージ
    //    Sprite* subMessageSprite_       ── 着地補正時の「空中に戻ろう」テキスト
    //
    //  ■ ステップ完了エフェクト（任意）
    //    Sprite* stepClearEffectSprite_  ── ステップ達成時のフラッシュ演出
    //    float   stepClearEffectTimer_   ── エフェクト継続タイマー
};
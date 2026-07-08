#pragma once
#include "Application/GameObject/Player/Player.h"
#include "Sprite.h"
#include <GamePad.h>
#include <memory>
#include <string>
#include <type/Vector2.h>
#include <unordered_map>
#include <vector>

/// <summary>
/// トレーニング用の入力表示UI。
/// 押した操作のボタン/キーのグリフと操作名を画面左下に履歴表示する。
/// 新しい入力は左端に下からフェードインし、次の入力で古いものが右へスライドしていく。
/// 各エントリの間には矢印「→」を挟む。近接攻撃は段ごとの名前（Jab 等）を表示する。
///
/// 描画は「一度だけ生成した Sprite プールを毎フレーム再バインドする」方式。
/// Sprite は単一のマップ済みバッファを使うため、同一インスタンスを1フレームに複数位置で
/// 描くと破綻する。そのため表示要素ごとに別インスタンスを割り当てる（プール）。
/// 文字画像は TextRenderer で一度だけ生成し、テクスチャパスと表示サイズをキャッシュする。
/// </summary>
class InputDisplayUI {
  public:
    /// <summary>
    /// 初期化。グリフ・ラベルの文字画像とスプライトプールを生成する。
    /// </summary>
    /// <param name="player">近接攻撃名の取得・入力デバイス判定に使用</param>
    /// <param name="gamePad">入力検出に使うゲームパッド（シーン側で毎フレーム更新済みのもの）</param>
    void Initialize(Player *player, Hagine::GamePad *gamePad);

    /// <summary>更新（入力検出＋履歴アニメーション）</summary>
    void Update();

    /// <summary>描画</summary>
    void Draw();

    /// <summary>終了処理</summary>
    void Finalize();

  private:
    /// ===================================================
    /// private struct
    /// ===================================================

    // グリフ/ラベルのテクスチャ情報（テクスチャパスと表示サイズ）
    struct GlyphInfo {
        std::string path;         // テクスチャパス
        Hagine::Vector2 size{};   // 表示サイズ
    };

    // 履歴1エントリ
    struct HistoryEntry {
        std::string glyphPath;       // ボタン/キーのテクスチャパス
        std::string labelPath;       // 操作名のテクスチャパス
        Hagine::Vector2 glyphSize{}; // グリフの表示サイズ
        Hagine::Vector2 labelSize{}; // ラベルの表示サイズ
        Hagine::Vector2 pos{};       // 現在位置（グリフ左上）
        float alpha = 0.0f;          // 現在アルファ
        int slot = 0;                // スロット番号（0 = 最新・左端）
        float appearT = 0.0f;        // 出現アニメ進捗(0-1)
    };

    /// ===================================================
    /// private method
    /// ===================================================

    void PollInputs();
    void PushEntry(const std::string &glyphKey, const std::string &labelKey);

    /// <summary>Playerのアクションイベント（実発動）を履歴に積む</summary>
    void PushActionEvent(Player::ActionKind kind);

    /// <summary>テキストから文字画像を生成し、テクスチャ情報をキャッシュする</summary>
    void RegisterText(const std::string &key, const std::string &text, float displayHeight);

    /// <summary>ボタンPNGのテクスチャ情報をキャッシュする</summary>
    void RegisterButton(const std::string &key, const std::string &texturePath, float size);

    /// <summary>プールのスプライトにテクスチャ・サイズ・位置・アルファを設定して描画する</summary>
    void BindAndDraw(Hagine::Sprite *sprite, const std::string &path,
                     Hagine::Vector2 size, Hagine::Vector2 pos, float alpha);

    bool HasGlyph(const std::string &key) const { return glyphs_.count(key) != 0; }
    bool IsGamePad() const;
    static float SlotAlpha(int slot);

    /// ===================================================
    /// private variables
    /// ===================================================

    Player *player_ = nullptr;           // プレイヤー（非所有）
    Hagine::GamePad *gamePad_ = nullptr; // ゲームパッド（非所有）
    std::string fontKey_;                // 文字生成に使うフォントキー

    std::unordered_map<std::string, GlyphInfo> glyphs_; // グリフ/ラベルのテクスチャ情報

    // 描画用スプライトプール（一度だけ生成して毎フレーム再バインド）
    std::vector<std::unique_ptr<Hagine::Sprite>> glyphPool_;
    std::vector<std::unique_ptr<Hagine::Sprite>> labelPool_;
    std::vector<std::unique_ptr<Hagine::Sprite>> arrowPool_;

    // 履歴の背景パネル（可視エントリに合わせて幅・アルファを補間する）
    std::unique_ptr<Hagine::Sprite> bgPanel_;
    float panelWidth_ = 0.0f;
    float panelAlpha_ = 0.0f;

    std::vector<HistoryEntry> history_; // 表示中の入力履歴

    bool wasMoving_ = false; // 移動入力の立ち上がり検出用（前フレーム）

    // ---- レイアウト定数 ----
    static constexpr int kMaxHistory = 8;            // 履歴の上限（プールサイズと一致）
    static constexpr float kAnchorX = 60.0f;         // 最新エントリの左端X
    static constexpr float kBaseY = 990.0f - 130.0f; // グリフ上端の定位置Y（左下）
    static constexpr float kSlotPitch = 250.0f;      // スロット間の間隔
    static constexpr float kGlyphSize = 46.0f;       // ボタンアイコンの一辺
    static constexpr float kLabelHeight = 30.0f;     // 操作名の高さ
    static constexpr float kKeycapHeight = 34.0f;    // キーキャップ文字の高さ
    static constexpr float kGlyphLabelGap = 8.0f;    // グリフと操作名の間隔
    static constexpr float kArrowHeight = 26.0f;     // 矢印の高さ
    static constexpr float kArrowInSlotX = 198.0f;   // エントリ基準の矢印X位置
    static constexpr float kRiseOffset = 40.0f;      // 出現時の下方向オフセット
    static constexpr float kAppearTime = 0.3f;       // 出現アニメ時間(秒)
    static constexpr float kAlphaLerp = 12.0f;       // アルファ補間速度(毎秒)
    static constexpr float kSlideLerp = 12.0f;       // 横スライド補間速度(毎秒)
    static constexpr int kSlotVisibleCount = 4;      // 表示スロット数（これ以上古いと消える）

    // 背景パネル
    static constexpr float kPanelAlpha = 0.5f;      // パネルの最大不透明度
    static constexpr float kPanelPadX = 16.0f;      // 左右パディング
    static constexpr float kPanelPadTop = 10.0f;    // 上パディング
    static constexpr float kPanelPadBottom = 12.0f; // 下パディング
    static constexpr float kPanelLerp = 12.0f;      // パネルの幅・アルファ補間速度(毎秒)
};

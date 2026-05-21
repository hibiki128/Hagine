#pragma once
#include "Easing.h"
#include <SpriteManager.h>
#include <array>

// 前方宣言
class Input;
class GamePad;

/// <summary>
/// リザルト画面のUI管理クラス
/// クリアタイム、残り体力、ランクなどの表示を制御
/// </summary>
class ResultUI {
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
    /// 数値スプライトの更新
    /// </summary>
    void UpdateNumberSprites();

    /// <summary>
    /// 全てのアニメーションが終了したかを取得
    /// </summary>
    /// <returns>終了していたらtrue</returns>
    bool IsAllAnimationFinished() const { return isAllAnimationFinished_; }

    /// <summary>
    /// イージング開始フラグの設定
    /// </summary>
    /// <param name="isStart">開始するならtrue</param>
    void SetIsStartEasing(bool isStart) { isStartEasing_ = isStart; }

    /// <summary>
    /// HPの設定
    /// </summary>
    /// <param name="hp">設定するHP</param>
    void SetHP(float hp) { HP_ = hp; }

    /// <summary>
    /// クリアタイムの設定
    /// </summary>
    /// <param name="time">設定するクリアタイム</param>
    void SetClearTime(float time) { ClearTime_ = time; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// スキップ入力のチェック
    /// </summary>
    /// <returns>入力があったらtrue</returns>
    bool CheckSkipInput();

    /// <summary>
    /// タイムアニメーションをスキップ
    /// </summary>
    void SkipTimeAnimation();

    /// <summary>
    /// HPアニメーションをスキップ
    /// </summary>
    void SkipHPAnimation();

    /// <summary>
    /// ランク表示をスキップ
    /// </summary>
    void SkipRankDisplay();

    /// ===================================================
    /// private enum
    /// ===================================================

    /// <summary>
    /// スプライトのインデックス
    /// </summary>
    enum SpriteIndex {
        kBackground, // 背景
        kResult,     // 結果
        kClearTime,  // クリアタイム
        kMinTens,    // 分の十の位
        kMinOnes,    // 分の一の位
        kCoron,      // コロン
        kSecTens,    // 秒の十の位
        kSecOnes,    // 秒の一の位
        kHP,         // 残り体力
        kHPHund,     // 体力の百の位
        kHPTens,     // 体力の十の位
        kHPOnes,     // 体力の一の位
        kPercent,    // パーセント
        kRank,       // ランク
        kMaxSprite
    };

    /// <summary>
    /// スキップ段階
    /// </summary>
    enum SkipPhase {
        kNoSkip,    // スキップなし
        kSkipTime,  // クリアタイムをスキップ
        kSkipHP,    // HPをスキップ
        kSkipRank,  // ランクをスキップ
        kAllSkipped // 全てスキップ済み
    };

    /// <summary>
    /// 数字カウントアップの状態
    /// </summary>
    enum NumberAnimState {
        kWaiting,       // 待機中
        kAnimatingTime, // タイムアニメーション中
        kWaitingForHP,  // HP表示待ち
        kAnimatingHP,   // HPアニメーション中
        kFinished       // 完了
    };

    /// ===================================================
    /// private variables
    /// ===================================================

    // 定数定義
    static constexpr float kDelayTime = 0.25f;         // スプライト間の遅延時間(秒)
    static constexpr float kAnimDuration = 0.5f;       // カウントアップアニメーション時間(秒)
    static constexpr float kEasingDuration = 1.5f;     // イージング時間(秒)
    static constexpr float kDefaultHP = 50.0f;         // デフォルトHP
    static constexpr float kDefaultClearTime = 180.0f; // デフォルトクリアタイム(秒)
    static constexpr int kSecondsPerMinute = 60;       // 1分の秒数
    static constexpr int kDigitDivisor = 10;           // 桁の除数
    static constexpr int kHundredDivisor = 100;        // 百の位の除数
    static constexpr float kUVStep = 0.1f;             // UV座標のステップ幅
    static constexpr float kAlphaVisible = 1.0f;       // 完全表示のアルファ値
    static constexpr float kAlphaInvisible = 0.0f;     // 非表示のアルファ値
    static constexpr float kNormalizeValue = 1.0f;     // 正規化値
    static constexpr int kZeroValue = 0;               // ゼロ値

    std::array<SpriteData *, kMaxSprite> sprites_; // スプライト配列

    std::array<EasingData<Vector2>, kMaxSprite> positionEasings_; // 位置イージング

    std::array<Vector2, kMaxSprite> startPositions_; // 開始位置
    std::array<Vector2, kMaxSprite> endPositions_;   // 終了位置

    bool isStartEasing_ = false;          // イージング開始フラグ
    bool isAllAnimationFinished_ = false; // 全アニメーション完了フラグ

    int currentEasingIndex_ = 0; // 現在イージング中のインデックス
    float delayTimer_ = 0.0f;    // 遅延タイマー

    NumberAnimState numberAnimState_ = kWaiting; // カウントアップ状態
    float animTimer_ = 0.0f;                    // アニメーションタイマー

    float displayedTime_ = 0.0f; // 表示中のタイム
    float displayedHP_ = 0.0f;   // 表示中のHP

    float HP_ = kDefaultHP;               // 残り体力
    float ClearTime_ = kDefaultClearTime; // クリアタイム
    std::string Rank_ = "A";              // ランク

    Input *input_ = nullptr;                    // 入力
    std::unique_ptr<GamePad> gamePad_ = nullptr; // ゲームパッド

    SkipPhase skipPhase_ = kNoSkip; // スキップ段階
};
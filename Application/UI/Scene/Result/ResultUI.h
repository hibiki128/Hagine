#pragma once
#include "Easing.h"
#include <SpriteManager.h>
#include <array>
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
    /// 全てのアニメーションが終了したか
    /// </summary>
    bool IsAllAnimationFinished() const { return isAllAnimationFinished_; }

    /// <summary>
    /// Setter
    /// </summary>
    void SetIsStartEasing(bool isStart) { isStartEasing_ = isStart; }
    void SetHP(float hp) { HP_ = hp; }
    void SetClearTime(float time) { ClearTime_ = time; }

  private:
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

  private:
    // スプライト配列
    std::array<SpriteData *, kMaxSprite> sprites_;

    // イージング用
    std::array<EasingData<Vector2>, kMaxSprite> positionEasings_;

    // イージング用位置設定
    std::array<Vector2, kMaxSprite> startPositions_;
    std::array<Vector2, kMaxSprite> endPositions_;

    bool isStartEasing_ = false;
    bool isAllAnimationFinished_ = false; // 全アニメーション完了フラグ

    int currentEasingIndex_ = 0;   // 現在イージング中のスプライトインデックス
    float delayTimer_ = 0.0f;      // 次のスプライトまでの遅延タイマー
    const float kDelayTime = 0.25f; // スプライト間の遅延時間

    // 数字のカウントアップ用
    enum NumberAnimState {
        kWaiting,       // 待機中
        kAnimatingTime, // タイムアニメーション中
        kWaitingForHP,  // HP表示待ち
        kAnimatingHP,   // HPアニメーション中
        kFinished       // 完了
    };

    NumberAnimState numberAnimState_ = kWaiting;
    float animTimer_ = 0.0f;
    const float kAnimDuration = 0.5f; // アニメーション時間

    float displayedTime_ = 0.0f; // 表示中のタイム
    float displayedHP_ = 0.0f;   // 表示中のHP

    float HP_ = 50.0f;         // 残り体力(記録値)
    float ClearTime_ = 180.0f; // クリアタイム(記録値)
    std::string Rank_ = "A";
};
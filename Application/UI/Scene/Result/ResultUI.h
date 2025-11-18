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
    /// Setter
    /// </summary>

    void SetIsStartEasing(bool isStart) { isStartEasing_ = isStart; }

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
        kHP,         // 残り体力
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

    int currentEasingIndex_ = 0;   // 現在イージング中のスプライトインデックス
    float delayTimer_ = 0.0f;      // 次のスプライトまでの遅延タイマー
    const float kDelayTime = 0.4f; // スプライト間の遅延時間
};

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
        kScoreText,  // スコアテキスト
        kTimeText,   // タイムテキスト
        kRankText,   // ランクテキスト
        kMaxSprite
    };

  private:
    // スプライト配列
    std::array<SpriteData *, kMaxSprite> sprites_;

    // イージング用
    std::array<EasingData<Vector2>, kMaxSprite> positionEasings_;

    bool isStartEasing_ = false;
};

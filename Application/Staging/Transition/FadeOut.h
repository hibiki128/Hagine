#pragma once
#include "Particle/CSParticle/ParticleCSEmitter.h"

/// <summary>
/// フェードアウト効果を管理するクラス
/// パーティクルを使用してフェードアウト演出を実現
/// </summary>
class FadeOut {
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
    /// <param name="vp">ビュープロジェクション</param>
    void Draw(const Hagine::ViewProjection &vp);

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// ImGui表示
    /// </summary>
    void ImGui();

    /// <summary>
    /// Getter
    /// </summary>
    bool IsFinish() const { return isFinish_; }

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 定数定義
    static constexpr float kSpriteDrawTime = 0.5f;    // スプライト描画時間(秒)
    static constexpr float kGravityEnableTime = 0.5f; // 重力有効化時間(秒)
    static constexpr float kParticleStopTime = 0.6f;  // パーティクル停止時間(秒)
    static constexpr float kFinishTime = 2.0f;        // フェードアウト完了時間(秒)
    static constexpr float kDeltaTime = 1.0f / 60.0f; // デルタタイム(秒)
    static constexpr float kPositionX = 12.5f;         // パーティクル位置X
    static constexpr float kPositionY = 5.0f;          // パーティクル位置Y
    static constexpr float kPositionZ = -65.1f;         // パーティクル位置Z
    static constexpr float kRotationX = 0.0f;       // パーティクル回転X(度)

    std::unique_ptr<Hagine::ParticleCSEmitter> fadeOut_ = nullptr; // フェードアウトパーティクル
    float timer_ = 0.0f;    // 経過時間
    bool isFinish_ = false; // 終了フラグ
};
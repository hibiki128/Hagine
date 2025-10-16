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
    void Draw(const ViewProjection &vp);

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// ImGui表示
    /// </summary>
    void ImGui();

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    std::unique_ptr<ParticleCSEmitter> fadeOut_ = nullptr;
    float timer_ = 0.0f; // 経過時間
};
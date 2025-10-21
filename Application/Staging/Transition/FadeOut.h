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
    
    /// <summary>
    /// フェードアウトの終了判定
    /// </summary>
    /// <returns></returns>
    bool IsFinish() const { return isFinish_; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    std::unique_ptr<ParticleCSEmitter> fadeOut_ = nullptr;
    float timer_ = 0.0f; // 経過時間
    bool isFinish_ = false;
};
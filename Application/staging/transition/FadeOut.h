#pragma once
#include "particle/gpu/ParticleCSEmitter.h"

/// <summary>
/// フェードアウト効果を管理するクラス
/// パーティクルを使用してフェードアウト演出を実現
/// </summary>
class FadeOut
{
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

    /// <summary>
    /// パーティクルの発生が止まり、待機時間が経過したか
    /// （スタートカメラ演出を開始してよいタイミングの判定用）
    /// </summary>
    /// <returns>bool: カメラ演出を開始してよければtrue</returns>
    bool IsCameraStartReady() const { return timer_ >= kParticleStopTime + kCameraStartDelay; }

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 定数定義
    static constexpr float kSpriteDrawTime = 0.5f;    // スプライト描画時間(秒)
    static constexpr float kGravityEnableTime = 0.5f; // 重力有効化時間(秒)
    static constexpr float kParticleStopTime = 0.6f;  // パーティクル停止時間(秒)
    static constexpr float kCameraStartDelay = 0.3f;  // 発生停止からカメラ演出開始までの待機時間(秒)
    static constexpr float kFinishTime = 2.0f;        // フェードアウト完了時間(秒)
    static constexpr float kDeltaTime = 1.0f / 60.0f; // デルタタイム(秒)

    // 長方形パーティクル（壁）はカメラの真正面に追従配置して画面全体を覆う。
    // 壁の半サイズはカメラ距離に対して FOV を覆える十分な余裕を持たせる
    static constexpr float kCameraDistance = 3.0f; // カメラから壁までの距離
    static constexpr float kWallHalfWidth = 3.6f;  // 壁の半分の幅
    static constexpr float kWallHalfHeight = 2.2f; // 壁の半分の高さ

    std::unique_ptr<Hagine::ParticleCSEmitter> fadeOut_ = nullptr; // フェードアウトパーティクル
    float timer_ = 0.0f;                                           // 経過時間
    bool isFinish_ = false;                                        // 終了フラグ
};
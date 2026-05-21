#pragma once
#include "Particle/CSParticle/ParticleCSEmitter.h"
#include <memory>
#include <type/Vector3.h>

/// <summary>
/// 死亡演出用クラス
/// </summary>
class DeathStaging {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    DeathStaging();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(Vector3 position, Vector4 color,
                    Vector3 pos_R_Arm, Vector4 c_R_Arm,
                    Vector3 pos_L_Arm, Vector4 c_L_Arm);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const ViewProjection &vp);

    /// <summary>
    /// Getter
    /// </summary>
    bool GetIsStart() const { return isStart_; }

    /// <summary>
    /// ImGui描画
    /// </summary>
    void imgui();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 定数定義
    static constexpr float kParticleActiveTime = 0.6f; // パーティクルの有効時間(秒)
    static constexpr float kGravityStartTime = 0.3f;   // 重力開始時間(秒)
    static constexpr float kAlphaZero = 0.0f;          // アルファ値ゼロ
    static constexpr float kMaxVelocityX = 0.5f;       // X方向最大速度
    static constexpr float kMaxVelocityY = 0.0f;       // Y方向最大速度
    static constexpr float kMaxVelocityZ = 0.5f;       // Z方向最大速度
    static constexpr float kMinVelocityX = -0.5f;      // X方向最小速度
    static constexpr float kMinVelocityY = -1.0f;      // Y方向最小速度
    static constexpr float kMinVelocityZ = -0.5f;      // Z方向最小速度

    Vector3 position_{};       // 座標
    Vector3 position_R_Arm{};  // 右腕座標
    Vector3 position_L_Arm{};  // 左腕座標
    Vector4 color_{};          // 色
    Vector4 color_R_Arm{};     // 右腕の色
    Vector4 color_L_Arm{};     // 左腕の色

    float time_{};             // 経過時間

    bool isStart_ = false;      // 開始フラグ

    std::unique_ptr<ParticleCSEmitter> deathParticle_ = nullptr;       // 死亡パーティクル
    std::unique_ptr<ParticleCSEmitter> deathParticle_R_Arm = nullptr;  // 右腕死亡パーティクル
    std::unique_ptr<ParticleCSEmitter> deathParticle_L_Arm = nullptr;  // 左腕死亡パーティクル
};
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
    void Initialize(Hagine::Math::Vector3 position, Hagine::Math::Vector4 color,
                    Hagine::Math::Vector3 pos_R_Arm, Hagine::Math::Vector4 c_R_Arm,
                    Hagine::Math::Vector3 pos_L_Arm, Hagine::Math::Vector4 c_L_Arm);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const Hagine::Camera::ViewProjection &vp);

    /// <summary>
    /// Getter
    /// </summary>
    bool GetIsStart() const { return isStart_; }

    void imgui();

  private:
    /// ===================================================
    /// private varians
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

    Hagine::Math::Vector3 position_{};
    Hagine::Math::Vector3 position_R_Arm{};
    Hagine::Math::Vector3 position_L_Arm{};
    Hagine::Math::Vector4 color_{};
    Hagine::Math::Vector4 color_R_Arm{};
    Hagine::Math::Vector4 color_L_Arm{};

    float time_{};

    bool isStart_ = false;

    std::unique_ptr<Hagine::Graphics::ParticleCSEmitter> deathParticle_ = nullptr;
    std::unique_ptr<Hagine::Graphics::ParticleCSEmitter> deathParticle_R_Arm = nullptr;
    std::unique_ptr<Hagine::Graphics::ParticleCSEmitter> deathParticle_L_Arm = nullptr;
};
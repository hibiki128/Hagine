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

    void imgui();

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    Vector3 position_{};
    Vector3 position_R_Arm{};
    Vector3 position_L_Arm{};
    Vector4 color_{};
    Vector4 color_R_Arm{};
    Vector4 color_L_Arm{};

    float time_{};

    bool isStart_ = false;

    std::unique_ptr<ParticleCSEmitter> deathParticle_ = nullptr;
    std::unique_ptr<ParticleCSEmitter> deathParticle_R_Arm = nullptr;
    std::unique_ptr<ParticleCSEmitter> deathParticle_L_Arm = nullptr;
};

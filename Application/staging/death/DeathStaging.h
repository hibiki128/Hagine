#pragma once
#include "particle/gpu/ParticleCSEmitter.h"
#include <memory>
#include <type/Quaternion.h>
#include <type/Vector3.h>

/// <summary>
/// 死亡演出用クラス
/// 死亡ポーズメッシュ(die.obj)の表面からパーティクルを発生させ、
/// 本体が粒子になって消えていくように見せる
/// </summary>
class DeathStaging
{
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
    /// 発生源メッシュをプレイヤーの死亡ポーズへ重ねるための姿勢と色を設定する
    /// </summary>
    /// <param name="position">発生位置（描画オフセット適用後のモデル位置）</param>
    /// <param name="rotation">プレイヤーの向き</param>
    /// <param name="scale">プレイヤーのスケール</param>
    /// <param name="color">パーティクルの色（プレイヤーの体色）</param>
    void Initialize(const Hagine::Vector3 &position, const Hagine::Quaternion &rotation,
                    const Hagine::Vector3 &scale, const Hagine::Vector4 &color);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const Hagine::ViewProjection &vp);

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

    Hagine::Vector3 position_{};                                       // 発生位置
    Hagine::Quaternion rotation_ = Hagine::Quaternion::IdentityQuaternion(); // 向き
    Hagine::Vector3 scale_ = {1.0f, 1.0f, 1.0f};                       // スケール
    Hagine::Vector4 color_{};                                          // 色

    float time_{}; // 経過時間

    bool isStart_ = false; // 開始フラグ

    std::unique_ptr<Hagine::ParticleCSEmitter> deathParticle_ = nullptr; // 死亡パーティクル
};

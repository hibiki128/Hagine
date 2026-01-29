#pragma once
#include "Particle/CSParticle/ParticleCSEmitter.h"
#include <Object/Base/BaseObject.h>
class ResultStaging {

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
    void Draw(const Hagine::Camera::ViewProjection &viewProjection);

    /// <summary>
    /// ImGui描画
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// セッター
    /// </summary>
    void SetfireWorkStarted(bool started) { fireWorkStarted_ = started; }
    void SetStartEasing(bool started) { startEasing_ = started; }

  private:
    // 定数定義
    static constexpr int kFireWorksCount = 10;             // 花火の数
    static constexpr float kFireWorkRisingTime = 2.5f;     // 花火上昇時間(秒)
    static constexpr float kFireWorkExplodingTime = 1.2f;  // 花火爆発表示時間(秒)
    static constexpr float kFireWorkHeightOffset = 90.5f;  // 花火の高さオフセット
    static constexpr float kMinFireWorkInterval = 0.3f;    // 花火発射の最小間隔(秒)
    static constexpr float kMaxFireWorkInterval = 1.2f;    // 花火発射の最大間隔(秒)
    static constexpr float kRandomPositionRange = 0.5f;    // ランダム位置生成範囲
    static constexpr float kAreaHalfSizeMultiplier = 0.5f; // エリア半分サイズ乗数
    static constexpr float kLineColorR = 1.0f;             // ライン色(R)
    static constexpr float kLineColorG = 0.5f;             // ライン色(G)
    static constexpr float kLineColorB = 0.0f;             // ライン色(B)
    static constexpr float kLineColorA = 1.0f;             // ライン色(A)

    Hagine::Graphics::BaseObject *RightHand_ = nullptr;
    Hagine::Graphics::BaseObject *LeftHand_ = nullptr;

    std::vector<std::unique_ptr<Hagine::Graphics::ParticleCSEmitter>> fireWorks_explosions_;
    std::vector<std::unique_ptr<Hagine::Graphics::ParticleCSEmitter>> fireWorks_trails_;

    bool secondMove_ = false;
    bool motionStarted_ = false;
    bool fireWorkStarted_ = false;
    bool startEasing_ = false;

    int fireWorks_count_ = kFireWorksCount;

    struct FireWorkState {
        enum class Phase {
            Ready,     // 待機中
            Rising,    // 上昇中
            Exploding, // 爆発中
        };
        Phase phase = Phase::Ready;
        float timer = 0.0f;
        Hagine::Math::Vector3 startPosition;
        Hagine::Math::Vector3 explodePosition;
    };

    std::vector<FireWorkState> fireWorkStates_;

    Hagine::Math::Vector3 fireWorkAreaCenter_ = {-135.0f, -25.0f, -200.0f};
    Hagine::Math::Vector3 fireWorkAreaSize_ = {300.0f, 20.0f, 100.0f};
    Hagine::Math::Quaternion fireWorkAreaRotation_ = Hagine::Math::Quaternion::IdentityQuaternion();

    float nextFireWorkTimer_ = 0.0f;
    float minFireWorkInterval_ = kMinFireWorkInterval;
    float maxFireWorkInterval_ = kMaxFireWorkInterval;

    // ヘルパー関数
    Hagine::Math::Vector3 GetRandomPositionInArea();
    int FindAvailableFireWork();
    void DrawFireWorkArea();
};
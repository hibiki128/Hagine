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
    void Draw(const ViewProjection &viewProjection);

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
    BaseObject *RightHand_ = nullptr;
    BaseObject *LeftHand_ = nullptr;

    std::vector<std::unique_ptr<ParticleCSEmitter>> fireWorks_explosions_;
    std::vector<std::unique_ptr<ParticleCSEmitter>> fireWorks_trails_;

    bool secondMove_ = false;
    bool motionStarted_ = false;
    bool fireWorkStarted_ = false;
    bool startEasing_ = false;

    int fireWorks_count_ = 10;

    struct FireWorkState {
        enum class Phase {
            Ready,     // 待機中
            Rising,    // 上昇中
            Exploding, // 爆発中
        };
        Phase phase = Phase::Ready;
        float timer = 0.0f;
        Vector3 startPosition;
        Vector3 explodePosition;
    };

    std::vector<FireWorkState> fireWorkStates_;

    Vector3 fireWorkAreaCenter_ = {-135.0f, -25.0f, -200.0f};
    Vector3 fireWorkAreaSize_ = {300.0f, 20.0f, 100.0f};
    Quaternion fireWorkAreaRotation_ = Quaternion::IdentityQuaternion();

    float nextFireWorkTimer_ = 0.0f;
    float minFireWorkInterval_ = 0.3f;
    float maxFireWorkInterval_ = 1.2f;

    // ヘルパー関数
    Vector3 GetRandomPositionInArea();
    int FindAvailableFireWork();
    void DrawFireWorkArea();
};

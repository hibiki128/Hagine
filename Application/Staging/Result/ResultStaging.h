#pragma once
#include "Particle/CSParticle/ParticleCSEmitter.h"
#include <Object/Base/BaseObject.h>

/// <summary>
/// リザルト演出用クラス
/// </summary>
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
    void Draw(const Hagine::ViewProjection &viewProjection);

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
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// エリア内のランダムな座標を取得
    /// </summary>
    /// <returns>Vector3: ランダム座標</returns>
    Hagine::Vector3 GetRandomPositionInArea();

    /// <summary>
    /// 使用可能な花火のインデックスを検索
    /// </summary>
    /// <returns>int: インデックス（なければ-1）</returns>
    int FindAvailableFireWork();

    /// <summary>
    /// 花火の発生エリアをデバッグ描画
    /// </summary>
    void DrawFireWorkArea();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

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

    Hagine::BaseObject *RightHand_ = nullptr; // 右手
    Hagine::BaseObject *LeftHand_ = nullptr;  // 左手

    std::vector<std::unique_ptr<Hagine::ParticleCSEmitter>> fireWorks_explosions_; // 花火の爆発パーティクル
    std::vector<std::unique_ptr<Hagine::ParticleCSEmitter>> fireWorks_trails_;     // 花火の軌跡パーティクル

    bool secondMove_ = false;      // 2つ目の動きフラグ
    bool motionStarted_ = false;   // モーション開始フラグ
    bool fireWorkStarted_ = false; // 花火開始フラグ
    bool startEasing_ = false;     // イージング開始フラグ

    int fireWorks_count_ = kFireWorksCount; // 花火の総数

    struct FireWorkState {
        enum class Phase {
            Ready,     // 待機中
            Rising,    // 上昇中
            Exploding, // 爆発中
        };
        Phase phase = Phase::Ready;     // 現在のフェーズ
        float timer = 0.0f;             // タイマー
        Hagine::Vector3 startPosition;          // 開始位置
        Hagine::Vector3 explodePosition;        // 爆発位置
    };

    std::vector<FireWorkState> fireWorkStates_; // 花火の状態リスト

    Hagine::Vector3 fireWorkAreaCenter_ = {-135.0f, -25.0f, -200.0f};            // 花火発生エリアの中心
    Hagine::Vector3 fireWorkAreaSize_ = {300.0f, 20.0f, 100.0f};      // 花火発生エリアのサイズ
    Hagine::Quaternion fireWorkAreaRotation_ = Hagine::Quaternion::IdentityQuaternion(); // 花火発生エリアの回転

    float nextFireWorkTimer_ = 0.0f;             // 次の花火までのタイマー
    float minFireWorkInterval_ = kMinFireWorkInterval; // 最小発射間隔
    float maxFireWorkInterval_ = kMaxFireWorkInterval; // 最大発射間隔
};
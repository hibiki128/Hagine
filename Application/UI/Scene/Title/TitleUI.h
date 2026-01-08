#pragma once
#include "Particle/CSParticle/ParticleCSEmitter.h"
#include <Particle/ParticleEmitter.h>
#include <SpriteManager.h>
#include <memory>

/// <summary>
/// タイトル画面のUI管理クラス
/// ロゴ、Press Startテキスト、パーティクルなどを制御
/// </summary>
class TitleUI {
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
    /// <param name="vp_">ビュープロジェクション</param>
    void Draw(ViewProjection &vp_);

    /// <summary>
    /// Getter
    /// </summary>
    bool GetIsFinish() const { return isFinish_; }

  private:
    /// ===================================================
    /// private enum
    /// ===================================================

    /// <summary>
    /// スプライトのインデックス
    /// </summary>
    enum SpriteIndex {
        kTitleLogo,  // タイトルロゴ
        kPressStart, // Press Startテキスト
        kMaxSprite
    };

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    // 定数定義
    static constexpr float kInitialChargeScale = 0.0f;         // 初期チャージスケール
    static constexpr float kChargeScaleSpeed = 1.5f;           // スケール拡大速度
    static constexpr float kMaxChargeScale = 3.5f;             // 最大スケール
    static constexpr float kSpriteEaseDuration = 1.5f;         // イージング時間(秒)
    static constexpr float kMaxTime = 2.0f;                    // パーティクル開始時間(秒)
    static constexpr float kChargeEffectStartTime = 1.5f;      // チャージエフェクト開始時間(秒)
    static constexpr float kMinStartInputTime = 3.0f;          // スタート入力可能時間(秒)
    static constexpr float kBulletEaseDuration = 7.0f;         // 弾のイージング時間(秒)
    static constexpr float kFinishDelayTime = 0.3f;            // 完了判定の遅延時間(秒)
    static constexpr float kDeltaTime = 1.0f / 60.0f;          // デルタタイム(秒)
    static constexpr float kTitleLogoStartPosX = -1100.0f;     // タイトルロゴ開始位置X
    static constexpr float kTitleLogoStartPosY = 0.0f;         // タイトルロゴ開始位置Y
    static constexpr float kPressStartStartPosX = 2000.0f;     // Press Start開始位置X
    static constexpr float kPressStartStartPosY = 0.0f;        // Press Start開始位置Y
    static constexpr float kTargetPosX = -4.5f;                // ターゲット位置X
    static constexpr float kTargetPosY = -3.1f;                // ターゲット位置Y
    static constexpr float kTargetPosZ = 5.7f;                 // ターゲット位置Z
    static constexpr float kPlayerPositionOffsetY = 6.5f;      // プレイヤー位置のYオフセット
    static constexpr float kParticleScaleSpeedDivisor = 1.25f; // パーティクルスケール速度の除数
    static constexpr float kParticleScaleBase = 0.8f;          // パーティクルスケール基準値
    static constexpr float kParticleScaleMultiplier = 1.4f;    // パーティクルスケール倍率
    static constexpr float kParticleScaleOffset = 0.3f;        // パーティクルスケールオフセット
    static constexpr float kBulletScaleOffset = 1.0f;          // 弾スケールオフセット

    float chargeScale_ = kInitialChargeScale;    // チャージスケール
    float chargeScaleSpeed_ = kChargeScaleSpeed; // スケール拡大速度
    float maxChargeScale_ = kMaxChargeScale;     // 最大スケール

    float spriteExitTimer_ = 0.0f;                   // スプライト終了タイマー
    float spriteEaseTimer_ = 0.0f;                   // スプライトイージングタイマー
    float bulletEaseTimer_ = 0.0f;                   // 弾イージングタイマー
    float spriteEaseDuration_ = kSpriteEaseDuration; // イージング時間

    float time_ = 0.0f; // 経過時間

    bool isSpriteExiting_ = false;  // スプライト終了中フラグ
    bool isMaxChargeScale_ = false; // 最大スケール到達フラグ
    bool isSpriteVisible_ = false;  // スプライト表示フラグ
    bool secondMove_ = false;       // 2番目の移動フラグ
    bool isFinish_ = false;         // 完了フラグ
    bool cameraMove_ = false;       // カメラ移動フラグ

    Vector2 titleLogoStartPos_ = {kTitleLogoStartPosX, kTitleLogoStartPosY};    // タイトルロゴ開始位置
    Vector2 pressStartStartPos_ = {kPressStartStartPosX, kPressStartStartPosY}; // Press Start開始位置
    Vector2 titleLogoEndPos_ = {};                                              // タイトルロゴ終了位置
    Vector2 pressStartEndPos_ = {};                                             // Press Start終了位置

    Vector3 targetPos_{};
    const float kMaxTime_ = kMaxTime;
    float timer_ = 0.0f;

    std::unique_ptr<ParticleEmitter> chargeBullet_ = nullptr;   // チャージ弾パーティクル
    std::unique_ptr<ParticleCSEmitter> chargeEffect_ = nullptr; // チャージエフェクトパーティクル
    std::unique_ptr<ParticleCSEmitter> playerAura_ = nullptr;   // プレイヤーオーラパーティクル

    std::array<SpriteData *, kMaxSprite> sprites_;
};
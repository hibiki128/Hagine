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
    /// 完了状態を取得
    /// </summary>
    /// <returns>bool: 完了フラグ</returns>
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

    float chargeScale_ = 1.0f;      // チャージスケール
    float chargeScaleSpeed_ = 1.5f; // スケール拡大速度
    float maxChargeScale_ = 3.5f;   // 最大スケール

    float spriteExitTimer_ = 0.0f;    // スプライト終了タイマー
    float spriteEaseTimer_ = 0.0f;    // スプライトイージングタイマー
    float bulletEaseTimer_ = 0.0f;    // 弾イージングタイマー
    float spriteEaseDuration_ = 1.5f; // イージング時間

    float time_ = 0.0f; // 経過時間

    bool isSpriteExiting_ = false;  // スプライト終了中フラグ
    bool isMaxChargeScale_ = false; // 最大スケール到達フラグ
    bool isSpriteVisible_ = false;  // スプライト表示フラグ
    bool secondMove_ = false;       // 2番目の移動フラグ
    bool isFinish_ = false;         // 完了フラグ
    bool cameraMove_ = false;       // カメラ移動フラグ

    Vector2 titleLogoStartPos_ = {-1100.0f, 0.0f}; // タイトルロゴ開始位置
    Vector2 pressStartStartPos_ = {2000.0f, 0.0f}; // Press Start開始位置
    Vector2 titleLogoEndPos_ = {};                 // タイトルロゴ終了位置
    Vector2 pressStartEndPos_ = {};                // Press Start終了位置

    Vector3 targetPos_{};
    const float kMaxTime_ = 2.0f;
    float timer_ = 0.0f;

    std::unique_ptr<ParticleEmitter> chargeBullet_ = nullptr; // チャージ弾パーティクル
    std::unique_ptr<ParticleEmitter> chargeEffect_ = nullptr; // チャージエフェクトパーティクル
    std::unique_ptr<ParticleCSEmitter> playerAura_ = nullptr; // プレイヤーオーラパーティクル

    std::array<SpriteData *, kMaxSprite> sprites_;
};
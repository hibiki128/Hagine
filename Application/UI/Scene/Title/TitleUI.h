#pragma once
#include "Particle/CSParticle/ParticleCSEmitter.h"
#include <Particle/ParticleEmitter.h>
#include <SpriteManager.h>
#include <memory>
class TitleUI {
  public:
    void Initialize();
    void Update();
    void Draw(ViewProjection &vp_);

    bool GetIsFinish() const { return isFinish_; }

  private:
    enum SpriteIndex {
        kTitleLogo,
        kPressStart,
        kMaxSprite
    };

    float chargeScale_ = 1.0f;
    float chargeScaleSpeed_ = 1.5f; // スケール拡大速度
    float maxChargeScale_ = 3.5f;   // 最大スケール
    float spriteExitTimer_ = 0.0f;
    float spriteEaseTimer_ = 0.0f;
    float bulletEaseTimer_ = 0.0f;
    float spriteEaseDuration_ = 1.5f; // イージングにかける時間
    float time_ = 0.0f;

    bool isSpriteExiting_ = false;
    bool isMaxChargeScale_ = false;
    bool isSpriteVisible_ = false;
    bool secondMove_ = false;
    bool isFinish_ = false;
    bool cameraMove_ = false;

    Vector2 titleLogoStartPos_ = {-1100.0f, 0.0f}; // 画面外の開始位置（左から）
    Vector2 pressStartStartPos_ = {2000.0f, 0.0f}; // 画面外の開始位置（右から）
    Vector2 titleLogoEndPos_ = {};
    Vector2 pressStartEndPos_ = {};

    Vector3 targetPos_{};

    const float kMaxTime_ = 2.0f;
    float timer_ = 0.0f;

    std::unique_ptr<ParticleEmitter> chargeBullet_ = nullptr;
    std::unique_ptr<ParticleEmitter> chargeEffect_ = nullptr;
    std::unique_ptr<ParticleCSEmitter> playerAura_ = nullptr;

    std::array<SpriteData *, kMaxSprite> sprites_;
};

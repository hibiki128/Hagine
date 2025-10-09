#pragma once
#include <Particle/ParticleEmitter.h>
#include <SpriteManager.h>
#include <memory>
#include"Particle/CSParticle/ParticleCSEmitter.h"
class TitleUI {
  public:
    void Initialize();
    void Update();
    void Draw(const ViewProjection &vp_);

  private:
    enum SpriteIndex {
        kTitleLogo,
        kPressStart,
        kMaxSprite
    };

    float chargeScale_ = 1.0f;
    float chargeScaleSpeed_ = 1.5f; // スケール拡大速度
    float maxChargeScale_ = 3.5f;   // 最大スケール
    bool isMaxChargeScale_ = false;

     bool isSpriteVisible_ = false;
    float spriteEaseTimer_ = 0.0f;
    float spriteEaseDuration_ = 1.5f;              // イージングにかける時間
    Vector2 titleLogoStartPos_ = {-1100.0f, 0.0f};  // 画面外の開始位置（左から）
    Vector2 pressStartStartPos_ = {2000.0f, 0.0f}; // 画面外の開始位置（右から）
    Vector2 titleLogoEndPos_ = {};
    Vector2 pressStartEndPos_ = {};

    float time_ = 0.0f;
    const float kMaxTime_ = 2.0f;

    std::unique_ptr<ParticleEmitter> chargeBullet_ = nullptr;
    std::unique_ptr<ParticleEmitter> chargeEffect_ = nullptr;
    std::unique_ptr<ParticleCSEmitter> playerAura_ = nullptr;

    std::array<SpriteData *, kMaxSprite> sprites_;
};

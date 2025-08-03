#pragma once
#include "BaseScene.h"

class ClearScene : public BaseScene {
  public: 
    /// ====================================
    /// public methods
    /// ====================================

    void Initialize() override;

    void Finalize() override;

    void Update() override;

    void Draw() override;

    void DrawForOffScreen() override;

    void AddSceneSetting() override;

    void AddObjectSetting() override;

    void AddParticleSetting() override;

    ViewProjection *GetViewProjection() override { return &vp_; }

  private:
    /// ====================================
    /// private methods
    /// ====================================
   
    void CameraUpdate();

    void ChangeScene();

  private:
    /// ====================================
    /// private variaus
    /// ====================================

    Audio *audio_;
    Input *input_;
   SpriteCommon *spCommon_;
    ParticleCommon *ptCommon_;

    ViewProjection vp_;
    std::unique_ptr<DebugCamera> debugCamera_;

    
    std::unique_ptr<Sprite>  clearLogo_;
    std::unique_ptr<Sprite> titleButton_;

    Vector2 titleButtonPosition_ = {880.0f, 600.0f};
    float titleButtonSize_ = 2.5f;
    Vector2 clearLogoPosition_ = {880.0f, 400.0f};
    float clearLogoSize_ = 2.0f;
};

#pragma once

#include "BaseScene.h"
#include "Easing.h"
#include "Object/Base/BaseObject.h"

#include "SkyBox/SkyBox.h"

class TitleScene : public BaseScene {
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

    SkyBox *skyBox_ = nullptr;

    std::unique_ptr<Sprite> titleLogo_;
    std::unique_ptr<Sprite> startButton_;

    Vector2 startButtonPosition_ = {880.0f, 600.0f};
    float startButtonSize_ = 2.5f;
    Vector2 titleLogoPosition_ = {880.0f, 400.0f};
    float titleLogoSize_ = 2.0f;
};

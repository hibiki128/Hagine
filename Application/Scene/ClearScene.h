#pragma once
#include "BaseScene.h"
#include"Application/Staging/Transition/FadeOut.h"

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
    std::unique_ptr<FadeOut> fadeOut_;

    float timer_ = 0.0f;

};

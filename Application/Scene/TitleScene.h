#pragma once
#include "BaseScene.h"
#include "Easing.h"
#include "Object/Base/BaseObject.h"

#include "SkyBox/SkyBox.h"
#include"Application/UI/Scene/Title/TitleUI.h"

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

    float time_ = 0.0f;
    const float kMaxTime_ = 2.0f;

    SkyBox *skyBox_ = nullptr;

    std::unique_ptr<TitleUI> titleUI_ = nullptr;
};

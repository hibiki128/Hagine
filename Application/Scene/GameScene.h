#pragma once
#include "BaseScene.h"

#include "Application/Camera/FollowCamera.h"
#include "Application/GameObject/Field/Ground/Ground.h"
#include "Application/GameObject/Field/SkyDome/SkyDome.h"
#include "Application/GameObject/Player/Player.h"
#include"Application/GameObject/Enemy/Enemy.h"

class GameScene : public BaseScene {
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

    // �v���C���[
    std::unique_ptr<Player> player_;

    // �G
    std::unique_ptr<Enemy> enemy_;

    // �Ǐ]�J����
    std::unique_ptr<FollowCamera> followCamera_;

    // �V��
    std::unique_ptr<SkyDome> skyDome_;

    // �n��
    std::unique_ptr<Ground> ground_;

    Enemy *enemy_ptr = nullptr;
    Player *player_ptr = nullptr;
};

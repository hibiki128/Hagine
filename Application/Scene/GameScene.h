#pragma once
#include "Application/Camera/FollowCamera.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Field/Ground/Ground.h"
#include "Application/GameObject/Field/SkyDome/SkyDome.h"
#include "Application/GameObject/Player/Player.h"
#include"Application/GameObject/Enemy/Enemy.h"
#include"Application/UI/Player/PlayerUI.h"
#include"Application/UI/Enemy/EnemyUI.h"
#include"SkyBox/SkyBox.h"

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

    // プレイヤー
    std::unique_ptr<Player> player_;

    // 敵
    std::unique_ptr<Enemy> enemy_;

    // 追従カメラ
    std::unique_ptr<FollowCamera> followCamera_;

    // 天球
    std::unique_ptr<SkyDome> skyDome_;

    // 地面
    std::unique_ptr<Ground> ground_;

    std::unique_ptr<PlayerUI> playerUI_;
    std::unique_ptr<EnemyUI> enemyUI_;

    SkyBox *skyBox_ = nullptr;

    Enemy *enemy_ptr = nullptr;
    Player *player_ptr = nullptr;
};
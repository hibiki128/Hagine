#pragma once
#include "Application/Camera/FollowCamera.h"
#include "Application/Camera/StartCamera.h"
#include "Application/GameObject/BehaviorTree/Editor/BehaviorTreeEditor.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Field/AroundField/AroundField.h"
#include "Application/GameObject/Field/Ground/Ground.h"
#include "Application/GameObject/Player/Player.h"
#include "Application/UI/Enemy/EnemyUI.h"
#include "Application/UI/Player/PlayerUI.h"
#include "BaseScene.h"
#include "SkyBox/SkyBox.h"
#include <Application/Camera/DeathCamera.h>
#include <Application/Staging/Transition/FadeOut.h>
#include <Application/UI/Scene/Game/GameUI.h>

class GameScene : public BaseScene {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;
    void DrawForOffScreen() override;
    void AddSceneSetting() override;
    void AddObjectSetting() override;
    void AddParticleSetting() override;

  private:
    void CameraUpdate();
    void ChangeScene();

  private:
    std::unique_ptr<Player> player_;
    std::unique_ptr<Enemy> enemy_;
    std::unique_ptr<AroundField> aroundField_;
    std::unique_ptr<FollowCamera> followCamera_;
    std::unique_ptr<StartCamera> startCamera_;
    std::unique_ptr<DeathCamera> deathCamera_;
    std::unique_ptr<Ground> ground_;
    std::unique_ptr<PlayerUI> playerUI_;
    std::unique_ptr<EnemyUI> enemyUI_;
    std::unique_ptr<FadeOut> fadeOut_;
    std::unique_ptr<GameUI> gameUI_;

    SkyBox *skyBox_ = nullptr;

    Enemy *enemy_ptr = nullptr;
    Player *player_ptr = nullptr;

    bool isGameOver_ = false;
    bool deathCameraStarted_ = false;
    float GameOverTimer_ = 0.0f;
    float ClearTimer_ = 0.0f;

    // ---------- BehaviorTree ----------
    // Release: BehaviorTreeLoader で起動時にJSONからロード
    // Debug  : BehaviorTreeEditor でリアルタイム編集 + ロード
#ifdef _DEBUG
    std::unique_ptr<BehaviorTreeEditor> behaviorTreeEditor_;
#endif
    // Release用: 起動時に一度だけセット済みのツリーを保持
    std::shared_ptr<BTNode> m_BehaviorTreeRoot;
};
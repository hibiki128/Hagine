#pragma once
#include "Application/GameObject/Player/Player.h"

#include "Application/UI/Player/PlayerUI.h"

#include "Application/Camera/FollowCamera.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Field/AroundField/AroundField.h"
#include "Application/GameObject/Field/Ground/Ground.h"
#include "Application/System/Tutorial/TutorialSystem.h"
#include "Application/UI/Enemy/EnemyUI.h"
#include "BaseScene.h"
#include "SkyBox/SkyBox.h"
#include <Application/Staging/Transition/FadeOut.h>
#include <Application/UI/Scene/Game/GameUI.h>
#include <Application/UI/Tutorial/TutorialUI.h>

class TutorialScene : public BaseScene {
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

    /// チュートリアルシステムからのエネミー出現/消滅リクエストを処理する
    void HandleEnemySpawnRequest();

  private:
    std::unique_ptr<Player> player_;
    std::unique_ptr<Enemy> enemy_;
    std::unique_ptr<AroundField> aroundField_;
    std::unique_ptr<FollowCamera> followCamera_;
    std::unique_ptr<Ground> ground_;
    std::unique_ptr<PlayerUI> playerUI_;
    std::unique_ptr<EnemyUI> enemyUI_;
    std::unique_ptr<FadeOut> fadeOut_;
    std::unique_ptr<GameUI> gameUI_;
    std::unique_ptr<GamePad> gamePad_;

    // ─── チュートリアル管理 ───
    std::unique_ptr<TutorialSystem> tutorialSystem_;
    std::unique_ptr<TutorialUI> tutorialUI_;

    SkyBox *skyBox_ = nullptr;
    Enemy *enemy_ptr = nullptr;
    Player *player_ptr = nullptr;

    float startDelayTimer_ = 0.0f;              ///< シーン開始からの経過時間
    bool sceneStarted_ = false;                 ///< 遅延が終了し、更新・描画を開始するフラグ
    static constexpr float kStartDelay_ = 2.0f; ///< 開始までの待機秒数
};
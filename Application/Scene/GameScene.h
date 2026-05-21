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

/// <summary>
/// ゲーム本編のシーンクラス
/// プレイヤー、敵、カメラ、UIなどのゲームメインループを管理する
/// </summary>
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
    /// ===================================================
    /// private method
    /// ===================================================

    void CameraUpdate();
    void ChangeScene();

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    std::unique_ptr<Player> player_;        // プレイヤー
    std::unique_ptr<Enemy> enemy_;          // 敵
    std::unique_ptr<AroundField> aroundField_; // 周囲のフィールド
    std::unique_ptr<FollowCamera> followCamera_; // フォローカメラ
    std::unique_ptr<StartCamera> startCamera_; // 開始時カメラ
    std::unique_ptr<DeathCamera> deathCamera_; // 死亡時カメラ
    std::unique_ptr<Ground> ground_;        // 地面
    std::unique_ptr<PlayerUI> playerUI_;    // プレイヤーUI
    std::unique_ptr<EnemyUI> enemyUI_;      // 敵UI
    std::unique_ptr<GameUI> gameUI_;        // ゲームUI

    SkyBox *skyBox_ = nullptr;              // スカイボックス

    Enemy *enemy_ptr = nullptr;             // 敵のポインタ（共有用）
    Player *player_ptr = nullptr;           // プレイヤーのポインタ（共有用）

    bool isGameOver_ = false;               // ゲームオーバーフラグ
    bool deathCameraStarted_ = false;       // 死亡時カメラ開始フラグ
    float GameOverTimer_ = 0.0f;            // ゲームオーバータイマー
    float ClearTimer_ = 0.0f;               // クリアタイマー

    // ---------- BehaviorTree ----------
#ifdef _DEBUG
    std::unique_ptr<BehaviorTreeEditor> behaviorTreeEditor_; // ビヘイビアツリーエディタ
#endif
    std::shared_ptr<BTNode> m_BehaviorTreeRoot; // ビヘイビアツリールート
};
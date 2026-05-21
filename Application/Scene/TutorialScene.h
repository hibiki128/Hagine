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

/// <summary>
/// チュートリアルシーンのクラス
/// 基本操作のレクチャーを行うシーンを管理する
/// </summary>
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
    /// ===================================================
    /// private method
    /// ===================================================

    void CameraUpdate();
    void ChangeScene();

    /// <summary>
    /// チュートリアルシステムからのエネミー出現/消滅リクエストを処理する
    /// </summary>
    void HandleEnemySpawnRequest();

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    std::unique_ptr<Player> player_;        // プレイヤー
    std::unique_ptr<Enemy> enemy_;          // 敵
    std::unique_ptr<AroundField> aroundField_; // 周囲のフィールド
    std::unique_ptr<FollowCamera> followCamera_; // フォローカメラ
    std::unique_ptr<Ground> ground_;        // 地面
    std::unique_ptr<PlayerUI> playerUI_;    // プレイヤーUI
    std::unique_ptr<EnemyUI> enemyUI_;      // 敵UI
    std::unique_ptr<FadeOut> fadeOut_;      // フェードアウト
    std::unique_ptr<GameUI> gameUI_;        // ゲームUI
    std::unique_ptr<GamePad> gamePad_;      // ゲームパッド

    // ─── チュートリアル管理 ───
    std::unique_ptr<TutorialSystem> tutorialSystem_; // チュートリアルシステム
    std::unique_ptr<TutorialUI> tutorialUI_;         // チュートリアルUI

    SkyBox *skyBox_ = nullptr;              // スカイボックス
    Enemy *enemy_ptr = nullptr;             // 敵のポインタ
    Player *player_ptr = nullptr;           // プレイヤーのポインタ

    float startDelayTimer_ = 0.0f;          // シーン開始からの経過時間
    bool sceneStarted_ = false;             // 遅延終了フラグ
    static constexpr float kStartDelay_ = 2.0f; // 開始までの待機秒数
};
#pragma once
#include "Application/Camera/FollowCamera.h"
#include"Application/Camera/StartCamera.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Field/Ground/Ground.h"
#include "Application/GameObject/Player/Player.h"
#include "Application/UI/Enemy/EnemyUI.h"
#include "Application/UI/Player/PlayerUI.h"
#include <Application/GameObject/Enemy/BehaviorTree/Editor/BehaviorTreeEditor.h>
#include <Application/Staging/Transition/FadeOut.h>
#include <Application/Camera/DeathCamera.h>
#include "SkyBox/SkyBox.h"
#include "BaseScene.h"

class GameScene : public BaseScene {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize() override;

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw() override;

    /// <summary>
    /// オフスクリーン描画処理
    /// </summary>
    void DrawForOffScreen() override;

    /// <summary>
    /// シーン設定を追加
    /// </summary>
    void AddSceneSetting() override;

    /// <summary>
    /// オブジェクト設定を追加
    /// </summary>
    void AddObjectSetting() override;

    /// <summary>
    /// パーティクル設定を追加
    /// </summary>
    void AddParticleSetting() override;

    /// <summary>
    /// ビュープロジェクションを取得
    /// </summary>
    /// <returns>ViewProjection*: ビュープロジェクションのポインタ</returns>
    ViewProjection *GetViewProjection() override { return &vp_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// カメラを更新
    /// </summary>
    void CameraUpdate();

    /// <summary>
    /// シーン遷移を実行
    /// </summary>
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

    // 開始時カメラ
    std::unique_ptr<StartCamera> startCamera_;

    // 死亡時カメラ
    std::unique_ptr<DeathCamera> deathCamera_;

    // 地面
    std::unique_ptr<Ground> ground_;

    std::unique_ptr<PlayerUI> playerUI_;
    std::unique_ptr<EnemyUI> enemyUI_;

    std::unique_ptr<BehaviorTreeEditor> behaviorTreeEditor_;

    std::unique_ptr<FadeOut> fadeOut_;

    SkyBox *skyBox_ = nullptr;

    Enemy *enemy_ptr = nullptr;
    Player *player_ptr = nullptr;

    bool isGameOver_ = false;
    bool deathCameraStarted_ = false;
    float GameOverTimer_ = 0.0f;
};

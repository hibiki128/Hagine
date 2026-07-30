#pragma once
#include <Application/camera/follow/FollowCamera.h>
#include <Application/camera/start/StartCamera.h>
#include <Application/entity/behavior/editor/BehaviorTreeEditor.h>
#include <Application/entity/enemy/Enemy.h>
#include <Application/entity/field/around/AroundField.h>
#include <Application/entity/field/ground/Ground.h>
#include <Application/entity/player/Player.h>
#include <Application/ui/enemy/EnemyUI.h>
#include <Application/ui/player/PlayerUI.h>
#include "BaseScene.h"
#include <skybox/SkyBox.h>
#include <Application/camera/death/DeathCamera.h>
#include <Application/staging/transition/FadeOut.h>
#include <Application/ui/scene/game/GameUI.h>

/// <summary>
/// ゲーム本編のシーンクラス
/// プレイヤー、敵、カメラ、UIなどのゲームメインループを管理する
/// </summary>
class GameScene : public Hagine::BaseScene
{
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
    /// ===================================================
    /// private variables
    /// ===================================================

    std::unique_ptr<Player> player_;             // プレイヤー
    std::unique_ptr<Enemy> enemy_;               // 敵
    std::unique_ptr<AroundField> aroundField_;   // 周囲のフィールド
    std::unique_ptr<FollowCamera> followCamera_; // フォローカメラ
    std::unique_ptr<StartCamera> startCamera_;   // 開始時カメラ
    std::unique_ptr<DeathCamera> deathCamera_;   // 死亡時カメラ
    std::unique_ptr<Ground> ground_;             // 地面
    std::unique_ptr<PlayerUI> playerUI_;         // プレイヤーUI
    std::unique_ptr<EnemyUI> enemyUI_;           // 敵UI
    std::unique_ptr<GameUI> gameUI_;             // ゲームUI
    std::unique_ptr<FadeOut> fadeOut_;           // シーン開始時のパーティクル遷移

    Hagine::SkyBox *pSkyBox_ = nullptr; // スカイボックス

    Enemy *pEnemy_ = nullptr;   // 敵のポインタ（共有用）
    Player *pPlayer_ = nullptr; // プレイヤーのポインタ（共有用）

    // 死亡カメラ完了からシーン切替までの待機時間(秒)
    // （die アニメーション約2.6秒 → 粒子化演出を見届けるまでの猶予）
    static constexpr float kGameOverWaitTime = 4.0f;

    // 敵撃破からシーン切替までの待機時間(秒)
    // （敵の die アニメーション約2.6秒 → 粒子化演出を見届けるまでの猶予）
    static constexpr float kEnemyDeathWaitTime = 4.5f;

    bool isGameOver_ = false;             // ゲームオーバーフラグ
    bool deathCameraStarted_ = false;     // 死亡時カメラ開始フラグ
    bool testDeathCameraStarted_ = false; // テスト用死亡カメラ開始フラグ
    float gameOverTimer_ = 0.0f;          // ゲームオーバータイマー
    float clearTimer_ = 0.0f;             // クリアタイマー
    float enemyDownTimer_ = 0.0f;         // 敵撃破後の演出待ちタイマー

    // ---------- BehaviorTree ----------
#ifdef _DEBUG
    std::unique_ptr<BehaviorTreeEditor> behaviorTreeEditor_; // ビヘイビアツリーエディタ
#endif
    std::shared_ptr<BTNode> behaviorTreeRoot_; // ビヘイビアツリールート
};

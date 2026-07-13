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
    /// private variants
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

    Hagine::SkyBox *pSkyBox_ = nullptr; // スカイボックス

    Enemy *enemy_ptr = nullptr;   // 敵のポインタ（共有用）
    Player *player_ptr = nullptr; // プレイヤーのポインタ（共有用）

    bool isGameOver_ = false;             // ゲームオーバーフラグ
    bool deathCameraStarted_ = false;     // 死亡時カメラ開始フラグ
    bool testDeathCameraStarted_ = false; // テスト用死亡カメラ開始フラグ
    float GameOverTimer_ = 0.0f;          // ゲームオーバータイマー
    float ClearTimer_ = 0.0f;             // クリアタイマー

    // ---------- BehaviorTree ----------
#ifdef _DEBUG
    std::unique_ptr<BehaviorTreeEditor> behaviorTreeEditor_; // ビヘイビアツリーエディタ
#endif
    std::shared_ptr<BTNode> behaviorTreeRoot_; // ビヘイビアツリールート
};

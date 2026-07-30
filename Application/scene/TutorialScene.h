#pragma once
#include <Application/entity/player/Player.h>

#include <Application/ui/player/PlayerUI.h>

#include <Application/camera/follow/FollowCamera.h>
#include <Application/entity/enemy/Enemy.h>
#include <Application/entity/field/around/AroundField.h>
#include <Application/entity/field/ground/Ground.h>
#include <Application/system/tutorial/TutorialSystem.h>
#include <Application/ui/enemy/EnemyUI.h>
#include "BaseScene.h"
#include <skybox/SkyBox.h>
#include <Application/staging/transition/FadeOut.h>
#include <Application/ui/scene/game/GameUI.h>
#include <Application/ui/tutorial/TutorialUI.h>

/// <summary>
/// チュートリアルシーンのクラス
/// 基本操作のレクチャーを行うシーンを管理する
/// </summary>
class TutorialScene : public Hagine::BaseScene
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
    /// シーン設定の追加（デバッグ用）
    /// </summary>
    void AddSceneSetting() override;

    /// <summary>
    /// オブジェクト設定の追加（デバッグ用）
    /// </summary>
    void AddObjectSetting() override;

    /// <summary>
    /// パーティクル設定の追加（デバッグ用）
    /// </summary>
    void AddParticleSetting() override;

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// カメラの更新処理
    /// </summary>
    void CameraUpdate();

    /// <summary>
    /// シーン切り替え処理
    /// </summary>
    void ChangeScene();

    /// <summary>
    /// チュートリアルシステムからのエネミー出現/消滅リクエストを処理する
    /// </summary>
    void HandleEnemySpawnRequest();

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    std::unique_ptr<Player> player_;             // プレイヤー
    std::unique_ptr<Enemy> enemy_;               // 敵
    std::unique_ptr<AroundField> aroundField_;   // 周囲のフィールド
    std::unique_ptr<FollowCamera> followCamera_; // フォローカメラ
    std::unique_ptr<Ground> ground_;             // 地面
    std::unique_ptr<PlayerUI> playerUI_;         // プレイヤーUI
    std::unique_ptr<EnemyUI> enemyUI_;           // 敵UI
    std::unique_ptr<FadeOut> fadeOut_;           // フェードアウト
    std::unique_ptr<GameUI> gameUI_;             // ゲームUI
    std::unique_ptr<Hagine::GamePad> gamePad_;   // ゲームパッド

    // ─── チュートリアル管理 ───
    std::unique_ptr<TutorialSystem> tutorialSystem_; // チュートリアルシステム
    std::unique_ptr<TutorialUI> tutorialUI_;         // チュートリアルUI

    Hagine::SkyBox *pSkyBox_ = nullptr; // スカイボックス
    Enemy *pEnemy_ = nullptr;        // 敵のポインタ
    Player *pPlayer_ = nullptr;      // プレイヤーのポインタ

    bool sceneStarted_ = false; // 遅延終了フラグ（パーティクル遷移完了で立つ）
};

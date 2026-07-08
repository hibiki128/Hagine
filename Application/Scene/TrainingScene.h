#pragma once
#include "Application/GameObject/Player/Player.h"

#include "Application/UI/Player/PlayerUI.h"

#include "Application/Camera/FollowCamera.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Field/AroundField/AroundField.h"
#include "Application/GameObject/Field/Ground/Ground.h"
#include "Application/UI/Enemy/EnemyUI.h"
#include "Application/UI/Scene/Training/InputDisplayUI.h"
#include "BaseScene.h"
#include "SkyBox/SkyBox.h"
#include <Application/UI/Scene/Game/GameUI.h>

/// <summary>
/// トレーニングシーンのクラス
/// 動かないダミー敵を相手に、操作方法を自由に確かめられる練習場を管理する。
/// カメラ演出は行わず、通常のシーン遷移フェードで出入りする。
/// </summary>
class TrainingScene : public Hagine::BaseScene {
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

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    std::unique_ptr<Player> player_;             // プレイヤー
    std::unique_ptr<Enemy> enemy_;               // ダミー敵
    std::unique_ptr<AroundField> aroundField_;   // 周囲のフィールド
    std::unique_ptr<FollowCamera> followCamera_; // フォローカメラ
    std::unique_ptr<Ground> ground_;             // 地面
    std::unique_ptr<PlayerUI> playerUI_;         // プレイヤーUI
    std::unique_ptr<EnemyUI> enemyUI_;           // 敵UI
    std::unique_ptr<GameUI> gameUI_;             // ゲームUI
    std::unique_ptr<InputDisplayUI> inputDisplay_; // 入力表示UI
    std::unique_ptr<Hagine::GamePad> gamePad_;   // ゲームパッド

    Hagine::SkyBox *skyBox_ = nullptr; // スカイボックス
    Enemy *enemy_ptr = nullptr;        // ダミー敵のポインタ（非所有）
    Player *player_ptr = nullptr;      // プレイヤーのポインタ（非所有）
};

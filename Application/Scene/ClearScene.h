#pragma once
#include "Application/GameObject/Field/Ground/Ground.h"
#include "Application/Staging/Result/ResultStaging.h"
#include "Application/Staging/Transition/FadeOut.h"
#include "Application/UI/Scene/Result/ResultUI.h"
#include "BaseScene.h"
#include "SkyBox/SkyBox.h"
#include <GamePad.h>

/// <summary>
/// クリア画面のシーンクラス
/// ゲームクリア時の処理と表示を管理する
/// </summary>
class ClearScene : public Hagine::BaseScene
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

    std::unique_ptr<Ground> ground_;                     // 地面
    std::unique_ptr<ResultStaging> resultStaging_;       // リザルト演出
    std::unique_ptr<ResultUI> resultUI_;                 // リザルトUI
    std::unique_ptr<Hagine::GamePad> gamePad_ = nullptr; // ゲームパッド
    Hagine::SkyBox *skyBox_ = nullptr;                   // スカイボックス

    const float cameraStartTimer_ = 3.0f;  // カメラ開始タイマー
    float currentCameraStartTimer_ = 0.0f; // 現在のカメラ開始タイマー
    bool cameraStart_ = false;             // カメラ開始フラグ
};

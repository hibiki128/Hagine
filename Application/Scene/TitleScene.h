#pragma once
#include "BaseScene.h"
#include "Easing.h"
#include "Object/Base/BaseObject.h"

#include "SkyBox/SkyBox.h"
#include"Application/UI/Scene/Title/TitleUI.h"
#include <GamePad.h>

/// <summary>
/// タイトルシーンのクラス
/// ゲーム起動後のタイトル画面を管理する
/// </summary>
class TitleScene : public BaseScene {
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

    float time_ = 0.0f;                     // 経過時間
    const float kMaxTime_ = 2.0f;           // カメラ移動開始までの時間
    bool firstMove_ = false;                // 最初のカメラ移動フラグ
    bool secondMove_ = false;               // 二番目のカメラ移動フラグ

    SkyBox *skyBox_ = nullptr;              // スカイボックス

    std::unique_ptr<TitleUI> titleUI_ = nullptr; // タイトルUI
    std::unique_ptr<GamePad> gamePad_ = nullptr; // ゲームパッド
};

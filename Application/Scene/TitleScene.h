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
class TitleScene : public Hagine::BaseScene {
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

    /// <summary>
    /// チュートリアル/トレーニング選択メニューのスプライトを生成する
    /// </summary>
    void CreateMenuSprites();

    /// <summary>
    /// 選択メニューを描画する
    /// </summary>
    void DrawMenu();

    // ---- 入力ヘルパ（キーボード/パッド両対応）----
    bool PressStartInput(); // Press Start / 決定（A / SPACE）
    bool ConfirmInput();    // メニュー決定（A / SPACE / Enter）
    bool UpInput();         // カーソル上（チュートリアル）
    bool DownInput();       // カーソル下（トレーニング）

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    /// タイトル進行フェーズ
    enum class TitlePhase {
        WaitStart,         // Press Start 待ち
        Menu,              // チュートリアル/トレーニング 選択中
        MenuClosing,       // 決定後のメニュー退場アニメ中
        TutorialCinematic, // チュートリアル開始演出中
        Training,          // トレーニングへ遷移予約済み
    };

    float time_ = 0.0f;                     // 経過時間
    const float kMaxTime_ = 2.0f;           // カメラ移動開始までの時間
    bool firstMove_ = false;                // 最初のカメラ移動フラグ
    bool secondMove_ = false;               // 二番目のカメラ移動フラグ

    TitlePhase titlePhase_ = TitlePhase::WaitStart; // 進行フェーズ
    int menuIndex_ = 0;                             // 0=チュートリアル / 1=トレーニング
    float menuAnimTimer_ = 0.0f;                    // メニュー出現アニメの経過時間
    float menuSelectLerp_ = 0.0f;                   // カーソル/ハイライトの補間値(0=上, 1=下)
    float menuCloseTimer_ = 0.0f;                   // 決定後の退場アニメ経過時間

    Hagine::SkyBox *skyBox_ = nullptr;              // スカイボックス

    std::unique_ptr<TitleUI> titleUI_ = nullptr; // タイトルUI
    std::unique_ptr<Hagine::GamePad> gamePad_ = nullptr; // ゲームパッド

    // 選択メニュー用スプライト
    std::unique_ptr<Hagine::Sprite> menuTutorial_ = nullptr;
    std::unique_ptr<Hagine::Sprite> menuTraining_ = nullptr;
    std::unique_ptr<Hagine::Sprite> menuCursor_ = nullptr;
    Hagine::Vector2 menuTutorialPos_{};
    Hagine::Vector2 menuTrainingPos_{};

    // メニュー配置定数
    static constexpr float kMenuTextHeight = 64.0f;  // メニュー文字の高さ（大きめ）
    static constexpr float kMenuX = 720.0f;          // メニュー左端の最終X
    static constexpr float kMenuTutorialY = 520.0f;  // チュートリアルY
    static constexpr float kMenuTrainingY = 650.0f;  // トレーニングY
    static constexpr float kMenuCursorGap = 78.0f;   // カーソルの左オフセット

    // メニュー出現アニメ（画面右外からイージングでスライドイン）
    static constexpr float kMenuStartX = 2000.0f;       // スライドイン開始X（画面右外）
    static constexpr float kMenuSlideDuration = 0.45f;  // スライド時間(秒)
    static constexpr float kMenuStagger = 0.09f;        // 2項目目の遅延(秒)

    // カーソル/ハイライトの補間・決定後の退場アニメ
    static constexpr float kMenuSelectLerp = 16.0f;     // 選択切替の補間速度(毎秒)
    static constexpr float kMenuCloseDuration = 0.3f;   // 決定後の退場アニメ時間(秒)
    static constexpr float kMenuCloseSlide = 260.0f;    // 退場時に右へずらす量(px)
};

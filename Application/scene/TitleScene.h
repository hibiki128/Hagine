#pragma once
#include "BaseScene.h"
#include "Easing.h"
#include "object/base/BaseObject.h"

#include "skybox/SkyBox.h"
#include "Application/ui/scene/title/TitleUI.h"
#include <GamePad.h>
#include <string>

/// <summary>
/// タイトルシーンのクラス
/// ゲーム起動後のタイトル画面を管理する
/// </summary>
class TitleScene : public Hagine::BaseScene
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

    /// <summary>
    /// チュートリアル/ゲーム/トレーニング選択メニューのスプライトを生成する
    /// </summary>
    void CreateMenuSprites();

    /// <summary>
    /// 選択メニューを描画する
    /// </summary>
    void DrawMenu();

    // ---- 入力ヘルパ（キーボード/パッド両対応）----
    bool PressStartInput(); // Press Start / 決定（A / SPACE）
    bool ConfirmInput();    // メニュー決定（A / SPACE / Enter）
    bool UpInput();         // カーソルを1つ上へ
    bool DownInput();       // カーソルを1つ下へ

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    /// タイトル進行フェーズ
    enum class TitlePhase
    {
        WaitStart,   // Press Start 待ち
        Menu,        // チュートリアル/ゲーム/トレーニング 選択中
        MenuClosing, // 決定後のメニュー退場アニメ中
        Cinematic,   // 開始演出中（チュートリアル/ゲーム共通）
        Training,    // トレーニングへ遷移予約済み
    };

    float time_ = 0.0f;           // 経過時間
    const float kMaxTime_ = 2.0f; // カメラ移動開始までの時間
    bool firstMove_ = false;      // 最初のカメラ移動フラグ
    bool secondMove_ = false;     // 二番目のカメラ移動フラグ

    TitlePhase titlePhase_ = TitlePhase::WaitStart; // 進行フェーズ
    int menuIndex_ = 0;                             // 0=チュートリアル / 1=ゲーム / 2=トレーニング
    float menuAnimTimer_ = 0.0f;                    // メニュー出現アニメの経過時間
    float menuSelectLerp_ = 0.0f;                   // カーソル/ハイライトの補間値(0=上端)
    float menuCloseTimer_ = 0.0f;                   // 決定後の退場アニメ経過時間
    std::string cinematicNextScene_ = "TUTORIAL";   // 開始演出後に遷移するシーン名
    bool prevStickUp_ = false;                      // 前フレームのスティック上入力（エッジ検出用）
    bool prevStickDown_ = false;                    // 前フレームのスティック下入力（エッジ検出用）

    Hagine::SkyBox *pSkyBox_ = nullptr; // スカイボックス

    std::unique_ptr<TitleUI> titleUI_ = nullptr;         // タイトルUI
    std::unique_ptr<Hagine::GamePad> gamePad_ = nullptr; // ゲームパッド

    // 選択メニュー用スプライト
    std::unique_ptr<Hagine::Sprite> menuTutorial_ = nullptr;
    std::unique_ptr<Hagine::Sprite> menuGame_ = nullptr;
    std::unique_ptr<Hagine::Sprite> menuTraining_ = nullptr;
    std::unique_ptr<Hagine::Sprite> menuCursor_ = nullptr;
    Hagine::Vector2 menuTutorialPos_{};
    Hagine::Vector2 menuGamePos_{};
    Hagine::Vector2 menuTrainingPos_{};

    // メニュー項目インデックス
    static constexpr int kMenuItemCount = 3;    // メニュー項目数
    static constexpr int kMenuIndexTutorial = 0; // チュートリアル
    static constexpr int kMenuIndexGame = 1;     // ゲーム本編
    static constexpr int kMenuIndexTraining = 2; // トレーニング

    // メニュー配置定数
    static constexpr float kMenuTextHeight = 64.0f; // メニュー文字の高さ（大きめ）
    static constexpr float kMenuX = 720.0f;         // メニュー左端の最終X
    static constexpr float kMenuTutorialY = 520.0f; // チュートリアルY
    static constexpr float kMenuGameY = 650.0f;     // ゲームY
    static constexpr float kMenuTrainingY = 780.0f; // トレーニングY
    static constexpr float kMenuCursorGap = 78.0f;  // カーソルの左オフセット

    // メニュー出現アニメ（画面右外からイージングでスライドイン）
    static constexpr float kMenuStartX = 2000.0f;      // スライドイン開始X（画面右外）
    static constexpr float kMenuSlideDuration = 0.45f; // スライド時間(秒)
    static constexpr float kMenuStagger = 0.09f;       // 2項目目の遅延(秒)

    // カーソル/ハイライトの補間・決定後の退場アニメ
    static constexpr float kMenuSelectLerp = 16.0f;   // 選択切替の補間速度(毎秒)
    static constexpr float kMenuCloseDuration = 0.3f; // 決定後の退場アニメ時間(秒)
    static constexpr float kMenuCloseSlide = 260.0f;  // 退場時に右へずらす量(px)
};

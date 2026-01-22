#pragma once
#include "AbstractSceneFactory.h"
#include "SceneTransition.h"
#include "memory"

namespace Hagine::Scene {

class SceneManager {
  private:
    static SceneManager *instance;

    SceneManager() = default;
    ~SceneManager() = default;
    SceneManager(SceneManager &) = delete;
    SceneManager &operator=(SceneManager &) = delete;

  public: // メンバ関数
    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    /// <returns></returns>
    static SceneManager *GetInstance();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// 描画
    /// </summary>
    void DrawForOffScreen();

    void SceneSelection(const std::string &sceneName);

    /// <summary>
    /// 遷移描画
    /// </summary>
    void DrawTransition();

    bool GetTransitionEnd() { return transitionEnd; }

  public: // setter
    /// <summary>
    /// シーンファクトリーのセット
    /// </summary>
    /// <param name="sceneFactory"></param>
    void SetSceneFactory(AbstractSceneFactory *sceneFactory) { sceneFactory_ = sceneFactory; }

    /// <summary>
    /// 次シーン予約
    /// </summary>
    /// <param name="nextScene"></param>
    void NextSceneReservation(const std::string &sceneName);

    /// <summary>
    /// シーン切り替え
    /// </summary>
    void SceneChange();

    float GetClearTime() const { return ClaerTime_; }
    float GetHP() const { return HP_; }

    BaseScene *GetBaseScene() { return scene_; }
    std::string GetCurrentSceneName() const { return currentSceneName_; }
    void SetClearTime(float time) { ClaerTime_ = time; }
    void SetHP(float hp) { HP_ = hp; }

  private:
    // 今のシーン(実行中のシーン)
    BaseScene *scene_ = nullptr;
    // 次のシーン
    BaseScene *nextScene_ = nullptr;
    // シーンファクトリー
    AbstractSceneFactory *sceneFactory_ = nullptr;
    SceneTransition *transition_ = nullptr;

    std::string currentSceneName_;

    bool transitionEnd = false;
    bool firstChange = false;

    float ClaerTime_ = 0.0f;
    float HP_ = 0.0f;
};

} // namespace Hagine::Scene
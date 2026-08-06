#pragma once
#include "Audio.h"
#include "camera/CameraManager.h"
#include "camera/debug/DebugCamera.h"
#include "camera/projection/ViewProjection.h"
#include "Input.h"
#include "light/LightGroup.h"
#include "object/base/BaseObject.h"
#include "object/base/BaseObjectManager.h"
#include "object/Object3dCommon.h"
#include "particle/gpu/ParticleCSEditor.h"
#include "particle/ParticleCommon.h"
#include "particle/ParticleEditor.h"
#include "particle/ParticleEmitter.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "SpriteManager.h"
#include "transform/WorldTransform.h"
#include "line/LineRenderer.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG
#include <OffScreen.h>
#include "SpriteManager.h"
#include "object/base/BaseObjectManager.h"
#include "render/DrawSystem.h"

namespace Hagine {
class SceneManager;

class BaseScene
{
  public:
    virtual ~BaseScene() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    virtual void Initialize();

    /// <summary>
    /// 終了
    /// </summary>
    virtual void Finalize();

    /// <summary>
    /// 更新
    /// </summary>
    virtual void Update();

    /// <summary>
    /// 描画
    /// </summary>
    virtual void Draw();

    /// <summary>
    /// ヒエラルキーに追加
    /// </summary>
    virtual void AddSceneSetting();

    /// <summary>
    /// インスペクターに追加
    /// </summary>
    virtual void AddObjectSetting();

    /// <summary>
    /// プロジェクトに追加
    /// </summary>
    virtual void AddParticleSetting();

    /// <summary>
    /// 描画
    /// </summary>
    virtual void DrawForOffScreen();

    virtual void SetSceneManager(SceneManager *sceneManager) { pSceneManager_ = sceneManager; }

    void SetOffScreen(OffScreen *pOffScreen) { pOffScreen_ = pOffScreen; }
    void SetDrawSystem(DrawSystem *drawSystem) { pDrawSystem_ = drawSystem; }
    void SetWinApp(WinApp *winApp) { pWinApp_ = winApp; }

    void DrawParticleEditorUI();

    void DrawAllObjects();

    /// <summary>
    /// 描画へ渡すビュープロジェクションを取得する（今アクティブなカメラのもの）
    /// </summary>
    ViewProjection *GetViewProjection() { return &vp(); }

    /// <summary>
    /// カメラを追加する（名前で管理される。最初の1台は自動でアクティブになる）。
    /// アクティブカメラの結果はそのまま描画に使われるので、行列を手でコピーする必要はない。
    /// </summary>
    /// <param name="name">カメラ名</param>
    /// <returns>Camera*: 追加したカメラ（所有は CameraManager）</returns>
    Camera *AddCamera(const std::string &name) { return pCameraManager_->Create(name); }

    /// <summary>名前でカメラを取得する</summary>
    /// <param name="name">カメラ名</param>
    /// <returns>Camera*: 見つからなければ nullptr</returns>
    Camera *GetCamera(const std::string &name) { return pCameraManager_->Find(name); }

  protected:
    // シーンマネージャ
    Audio *pAudio_ = nullptr;
    Input *pInput_ = nullptr;
    LightGroup *pLightGroup_ = nullptr;
    ParticleEditor *pPtEditor_ = nullptr;
    ParticleCSEditor *pPtCSEditor_ = nullptr;
    OffScreen *pOffScreen_ = nullptr;
    WinApp *pWinApp_ = nullptr;

    /// <summary>
    /// このシーンのメインカメラ（BaseScene::Initialize で生成。所有は CameraManager）。
    /// 位置や向きはこのカメラに対して設定する。
    /// </summary>
    Camera *camera_ = nullptr;

    /// <summary>
    /// 描画へ渡すビュープロジェクション（今アクティブなカメラのもの）。
    /// ViewProjection を持つのは Camera だけなので、描画に渡すときはこれを経由する。
    /// </summary>
    ViewProjection &vp();

    std::unique_ptr<DebugCamera> debugCamera_;

    SceneManager *pSceneManager_ = nullptr;
    CameraManager *pCameraManager_ = nullptr;
    SpriteManager *pSpriteManager_ = nullptr;
    BaseObjectManager *pObjectManager_ = nullptr;
    DrawSystem *pDrawSystem_ = nullptr;

    float clearTime_ = 0.0f;
    float hp_ = 0.0f;
};
} // namespace Hagine

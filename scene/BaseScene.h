#pragma once
#include "Audio.h"
#include "DebugCamera/DebugCamera.h"
#include "Input.h"
#include "LightGroup.h"
#include "Object/Object3d.h"
#include "Object/Object3dCommon.h"
#include "ParticleCommon.h"
#include "ParticleEditor.h"
#include "ParticleEmitter.h"
#include "SpriteCommon.h"
#include "ViewProjection/ViewProjection.h"
#include "WorldTransform.h"
#include "line/DrawLine3D.h"
#include"Object/BaseObjectManager.h"
#include"Sprite.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG
class SceneManager;
class BaseScene {
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

    virtual void SetSceneManager(SceneManager *sceneManager) { sceneManager_ = sceneManager; }

    virtual ViewProjection *GetViewProjection() = 0;

  protected:
    // シーンマネージャ
    SceneManager *sceneManager_ = nullptr;
};

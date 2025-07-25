#pragma once
#include "Audio.h"
#include "Camera/DebugCamera/DebugCamera.h"
#include "Input.h"
#include "Light/LightGroup.h"
#include "Object/Object3dCommon.h"
#include "Particle/ParticleCommon.h"
#include "Particle/ParticleEditor.h"
#include "Particle/ParticleEmitter.h"
#include "SpriteCommon.h"
#include "Camera/ViewProjection/ViewProjection.h"
#include "Transform/WorldTransform.h"
#include "line/DrawLine3D.h"
#include"Object/Base/BaseObjectManager.h"
#include"Sprite.h"
#include"Object/Base/BaseObject.h"
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

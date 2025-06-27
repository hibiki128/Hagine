#pragma once
#include "DirectXCommon.h"
#ifdef _DEBUG
#endif // _DEBUG
#include "Audio.h"
#include "Input.h"
#include"Object/Object3dCommon.h"
#include "SpriteCommon.h"
#include "Engine/offscreen/OffScreen.h"
#include <line/DrawLine3D.h>
#include"Object/BaseObjectManager.h"
#include"Particle/ParticleGroupManager.h"
#include <Graphics/Srv/SrvManager.h>
#include <Graphics/Texture/TextureManager.h>
#include <Graphics/Model/ModelManager.h>
#include <Debug/ImGui/ImGuizmoManager.h>
#include <Debug/ImGui/ImGuiManager.h>
#include <Particle/ParticleCommon.h>
#include <Particle/ParticleEditor.h>
#include <Collider/CollisionManager.h>
#include <Scene/AbstractSceneFactory.h>
#include <Scene/SceneManager.h>
#include"Debug/ResourceLeakChecker/D3DResourceLeakChecker.h"
#include"Graphics/PipeLine/PipeLineManager.h"
#include"Graphics/PipeLine/ComputePipeLineManager.h"

class Framework {
  public: // メンバ関数
    virtual ~Framework() = default;

    /// <summary>
    /// 実行
    /// </summary>
    void Run();

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
    /// リソース
    /// </summary>
    void LoadResource();

    /// <summary>
    /// 描画
    /// </summary>
    virtual void Draw() = 0;

    void PlaySounds();

    /// <summary>
    /// 終了チェック
    /// </summary>
    /// <returns></returns>
    virtual bool IsEndRequest() { return endRequest_; }
  private:

  protected:
    Input *input = nullptr;
    Audio *audio = nullptr;
    DirectXCommon *dxCommon = nullptr;
    WinApp *winApp = nullptr;
    DrawLine3D *line3d_ = nullptr;

    // シーンファクトリー
    AbstractSceneFactory *sceneFactory_ = nullptr;

    SceneManager *sceneManager_ = nullptr;
    SrvManager *srvManager = nullptr;
    TextureManager *textureManager_ = nullptr;
    ModelManager *modelManager_ = nullptr;
    ImGuiManager *imGuiManager_ = nullptr;
    ImGuizmoManager *imGuizmoManager_ = nullptr;
    BaseObjectManager *baseObjectManager_ = nullptr;
    ParticleGroupManager *particleGroupManager_ = nullptr;
    PipeLineManager *pipeLineManager_ = nullptr;
    ComputePipeLineManager *computePipeLineManager_ = nullptr;

    SpriteCommon *spriteCommon = nullptr;
    ParticleCommon *particleCommon = nullptr;
    ParticleEditor *particleEditor = nullptr;

    PrimitiveModel *primitiveModel = nullptr;

    std::unique_ptr<CollisionManager> collisionManager_;
    std::unique_ptr<OffScreen> offscreen_;

    bool endRequest_;
};

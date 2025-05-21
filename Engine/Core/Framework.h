#pragma once
#include "DirectXCommon.h"
#ifdef _DEBUG
#endif // _DEBUG
#include "AbstractSceneFactory.h"
#include "Audio.h"
#include "CollisionManager.h"
#include "Input.h"
#include"Model/ModelManager.h"
#include"Object/Object3dCommon.h"
#include "ParticleCommon.h"
#include "ParticleEditor.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include"Srv/SrvManager.h"
#include"Texture/TextureManager.h"
#include "Engine/offscreen/OffScreen.h"
#include <line/DrawLine3D.h>
#include"ImGui/ImGuiManager.h"
#include"ImGui/ImGuizmoManager.h"
#include"Object/BaseObjectManager.h"
#include"Particle/ParticleGroupManager.h"
#include"PipeLine/PipeLineManager.h"

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

    SpriteCommon *spriteCommon = nullptr;
    ParticleCommon *particleCommon = nullptr;
    ParticleEditor *particleEditor = nullptr;

    PrimitiveModel *primitiveModel = nullptr;

    std::unique_ptr<CollisionManager> collisionManager_;
    std::unique_ptr<OffScreen> offscreen_;

    bool endRequest_;
};

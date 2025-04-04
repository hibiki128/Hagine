#pragma once
#include "DirectXCommon.h"
#ifdef _DEBUG
#endif // _DEBUG
#include "AbstractSceneFactory.h"
#include "Audio.h"
#include "CollisionManager.h"
#include "Input.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "ParticleCommon.h"
#include "ParticleEditor.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "myEngine/offscreen/OffScreen.h"
#include <line/DrawLine3D.h>

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
    /// <summary>
    ///  FPS表示
    /// </summary>
    void DisplayFPS();

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

    SpriteCommon *spriteCommon = nullptr;
    Object3dCommon *object3dCommon = nullptr;
    ParticleCommon *particleCommon = nullptr;
    ParticleEditor *particleEditor = nullptr;

    std::unique_ptr<CollisionManager> collisionManager_;
    std::unique_ptr<OffScreen> offscreen_;
    std::unique_ptr<OffScreen> offscreen2_;

    bool endRequest_;
};

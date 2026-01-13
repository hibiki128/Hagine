#pragma once
#include "DirectXCommon.h"
#ifdef _DEBUG
#endif // _DEBUG
#include "Audio.h"
#include "Collider/CollisionManager.h"
#include "Debug/ImGui/ImGuiManager.h"
#include "Debug/ImGui/ImGuizmoManager.h"
#include "Debug/ResourceLeakChecker/D3DResourceLeakChecker.h"
#include "Edit/ShortcutManager/ShortcutManager.h"
#include "Engine/offscreen/OffScreen.h"
#include "Graphics/Model/ModelManager.h"
#include "Graphics/PipeLine/ComputePipeLineManager.h"
#include "Graphics/PipeLine/PipeLineManager.h"
#include "Graphics/Srv/SrvManager.h"
#include "Graphics/Texture/TextureManager.h"
#include "Input.h"
#include "Model/ModelCommon.h"
#include "Object/Base/BaseObjectManager.h"
#include "Particle/CSParticle/ParticleCSEditor.h"
#include "Particle/CSParticle/ParticleCSGroupManager.h"
#include "Particle/ParticleCommon.h"
#include "Particle/ParticleEditor.h"
#include "Particle/ParticleGroupManager.h"
#include "Scene/AbstractSceneFactory.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneTransition.h"
#include "SkyBox/SkyBox.h"
#include "SpriteCommon.h"
#include "SpriteManager.h"
#include "line/DrawLine3D.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
namespace Hagine::Core {
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
    /// ショートカットキーの登録
    /// </summary>
    void RegisterShortcutKey();

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
    Input *input_ = nullptr;
    Audio::Audio *audio_ = nullptr;
    Core::DirectXCommon *dxCommon_ = nullptr;
    Core::WinApp *winApp_ = nullptr;
    Graphics::Line::DrawLine3D *line3d_ = nullptr;
    Graphics::SkyBox *skyBox_ = nullptr;

    // シーンファクトリー
    AbstractSceneFactory *sceneFactory_ = nullptr;
    SceneTransition *sceneTransition_ = nullptr;

    SceneManager *sceneManager_ = nullptr;
    SrvManager *srvManager_ = nullptr;
    TextureManager *textureManager_ = nullptr;
    ModelManager *modelManager_ = nullptr;
    ImGuiManager *imGuiManager_ = nullptr;
    ImGuizmoManager *imGuizmoManager_ = nullptr;
    Graphics::BaseObjectManager *baseObjectManager_ = nullptr;
    Graphics::ParticleGroupManager *particleGroupManager_ = nullptr;
    Graphics::ParticleCSGroupManager *particleCSGroupManager_ = nullptr;
    PipeLineManager *pipeLineManager_ = nullptr;
    MotionEditor *motionEditor_ = nullptr;
    ComputePipeLineManager *computePipeLineManager_ = nullptr;
    ShortcutManager *shortcutManager_ = nullptr;
    Graphics::SpriteManager *spriteManager_ = nullptr;

    Graphics::SpriteCommon *spriteCommon_ = nullptr;
    Graphics::ParticleCommon *particleCommon_ = nullptr;
    Graphics::ModelCommon *modelCommon_ = nullptr;

    Graphics::Light::LightGroup *lightGroup_ = nullptr;

    Graphics::ParticleEditor *particleEditor_ = nullptr;
    Graphics::ParticleCSEditor *particleCSEditor_ = nullptr;

    Graphics::PrimitiveModel *primitiveModel_ = nullptr;

    D3DResourceLeakChecker LeakChecker_;

    CollisionManager *collisionManager_ = nullptr;

    std::unique_ptr<OffScreen> offscreen_;

    bool endRequest_;
};
} // namespace Hagine::Core
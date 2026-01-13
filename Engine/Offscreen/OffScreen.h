#pragma once
#include "PostEffect/PostEffectChain.h"
#include "PostEffect/PostEffectDataManager.h"
#include "PostEffect/PostEffectParameters.h"
#include "PostEffect/PostEffectRenderer.h"
#include <type/Matrix4x4.h>
namespace Hagine::Graphics {
class OffScreen {
  public:
    void Initialize();
    void Draw();
    void Setting();
    void SetProjection(Math::Matrix4x4 projectionMatrix);

    uint32_t GetFinalResultSrvIndex() const;
    void CopyFinalResultToBackBuffer();

  private:
    PostEffectChain effectChain_;
    PostEffectRenderer renderer_;
    PostEffectParameters parameters_;
    PostEffectDataManager dataManager_;

    Math::Matrix4x4 projectionMatrix_;
};
} // namespace Hagine::Graphics
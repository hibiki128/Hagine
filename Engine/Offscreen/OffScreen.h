#pragma once
#include "PostEffect/PostEffectChain.h"
#include "PostEffect/PostEffectDataManager.h"
#include "PostEffect/PostEffectParameters.h"
#include "PostEffect/PostEffectRenderer.h"
#include <type/Matrix4x4.h>

class OffScreen {
  public:
    void Initialize();
    void Draw();
    void Setting();
    void SetProjection(Matrix4x4 projectionMatrix);

    uint32_t GetFinalResultSrvIndex() const;
    void CopyFinalResultToBackBuffer();

  private:
    PostEffectChain effectChain_;
    PostEffectRenderer renderer_;
    PostEffectParameters parameters_;
    PostEffectDataManager dataManager_;

    Matrix4x4 projectionMatrix_;
};
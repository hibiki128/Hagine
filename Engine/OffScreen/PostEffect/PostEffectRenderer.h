#pragma once
#include "PostEffectChain.h"
#include "RendererBuffer.h"
#include"PostEffectParameters.h"

class PostEffectRenderer {
  public:
    void Initialize(DirectXCommon *dxCommon, SrvManager *srvManager, PipeLineManager *psoManager);
    void Draw(const PostEffectChain &effectChain, PostEffectParameters &parameters);

    uint32_t GetFinalResultSrvIndex() const { return renderBuffer_.GetFinalResultSrvIndex(); }
    void CopyFinalResultToBackBuffer();

  private:
    void DrawToFinalResult();
    void DrawSingleEffect(ShaderMode mode, bool isFirstInput, int inputPingPongIndex, int outputRtvIndex,
                          PostEffectParameters &parameters);

    DirectXCommon *dxCommon_;
    SrvManager *srvManager_;
    PipeLineManager *psoManager_;
    RenderBuffer renderBuffer_;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE finalResultRtvHandle_;
};
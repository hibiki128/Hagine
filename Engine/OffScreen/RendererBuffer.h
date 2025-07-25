#pragma once
#include "DirectXCommon.h"
#include <Graphics/Srv/SrvManager.h>

class RenderBuffer {
  public:
    void Initialize(DirectXCommon *dxCommon, SrvManager *srvManager);

    // ピンポンバッファ関連
    D3D12_CPU_DESCRIPTOR_HANDLE GetPingPongRtvHandle(int index) const { return pingPongRtvHandles_[index]; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetPingPongSrvHandleGPU(int index) const { return pingPongSrvHandlesGPU_[index]; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetPingPongResource(int index) const { return pingPongResources_[index]; }

    // 最終結果バッファ関連
    D3D12_CPU_DESCRIPTOR_HANDLE GetFinalResultRtvHandle() const { return finalResultRtvHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetFinalResultSrvHandleGPU() const { return finalResultSrvHandleGPU_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetFinalResultResource() const { return finalResultResource_; }
    uint32_t GetFinalResultSrvIndex() const { return finalResultSrvIndex_; }

    const int GetPingPongBufferCount() const {
        return kPingPongBufferCount;
    }

  private:
    void CreatePingPongBuffers();
    void CreateFinalResultTexture();

    int currentPingPongBuffer_ = 0;

    DirectXCommon *dxCommon_ = nullptr;
    SrvManager *srvManager_ = nullptr;

    static const int kPingPongBufferCount = 2;
    Microsoft::WRL::ComPtr<ID3D12Resource> pingPongResources_[kPingPongBufferCount];
    uint32_t pingPongSrvIndices_[kPingPongBufferCount];
    D3D12_CPU_DESCRIPTOR_HANDLE pingPongRtvHandles_[kPingPongBufferCount];
    D3D12_CPU_DESCRIPTOR_HANDLE pingPongSrvHandlesCPU_[kPingPongBufferCount];
    D3D12_GPU_DESCRIPTOR_HANDLE pingPongSrvHandlesGPU_[kPingPongBufferCount];

    Microsoft::WRL::ComPtr<ID3D12Resource> finalResultResource_;
    uint32_t finalResultSrvIndex_;
    D3D12_CPU_DESCRIPTOR_HANDLE finalResultRtvHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE finalResultSrvHandleCPU_;
    D3D12_GPU_DESCRIPTOR_HANDLE finalResultSrvHandleGPU_;
};
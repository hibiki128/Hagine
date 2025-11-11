#pragma once
#include "Particle/ParticleStruct.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <Graphics/Srv/SrvManager.h>
#include <Graphics/Texture/TextureManager.h>
#include <Model/Model.h>
#include <Model/ModelStructs.h>
#include <Particle/ParticleCommon.h>
#include <Primitive/PrimitiveModel.h>
#include <d3d12.h>
#include <utility>
#include <wrl.h>

class ParticleCSGroup {
  public:
    /// ===================================
    /// public methods
    /// ===================================

    ~ParticleCSGroup();
    ParticleCSGroupData CreateParticleGroup(const std::string &groupName, const std::string &filename, uint32_t maxParticleCount = 10000, const std::string &texturePath = {}, BlendMode blendMode = BlendMode::kAdd);
    ParticleCSGroupData CreatePrimitiveParticleGroup(const std::string &groupName, PrimitiveType type, uint32_t maxParticleCount = 10000, const std::string &texturePath = {}, BlendMode blendMode = BlendMode::kAdd);
    void Update(const ViewProjection &vp);
    void DrawImGui();
    int CalculateOptimalEmitCount() const;
    void UpdateParticleCSDisPatch();
    ParticleCSGroupData GetParticleGroupData() { return particleGroupData_; }

    /// ===================================
    /// Getter
    /// ===================================

    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetOutputParticleSrvHandle() const {
        return outputParticleSrvHandle_;
    }

    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetFreeListIndexSrvHandle() const {
        return freeListIndexSrvHandle_;
    }

    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetFreeListSrvHandle() const {
        return freeListSrvHandle_;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> GetPerFrameResource() const {
        return perFrameResource_;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> GetMaterialResource() const {
        return materialResource_;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> GetOutputParticleResource() const {
        return outputParticleResource_;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> GetPerViewResource() const {
        return perViewResource_;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> GetSettingsResource() const {
        return settingsResource_;
    }

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const {
        return indexBufferView_;
    }

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const {
        return vertexBufferView_;
    }

    uint32_t GetOutputParticleSrvIndex() const {
        return outputParticleSrvIndex_;
    }

    uint32_t GetOutputParticleSrvForVSIndex() const {
        return outputParticleSrvForVSIndex_;
    }

    ModelData GetModelData() const {
        return modelData;
    }

    PerFrame *GetPerFrameData() const {
        return perFrameData_;
    }

    uint32_t GetMaxParticleCount() const {
        return settingsData_->maxParticleCount;
    }

    ParticleCSSettings *GetSettingsData() const {
        return settingsData_;
    }

    void SetFrequency(float frequency) {
        frequency_ = frequency;
    }

    void SetSettingData(const ParticleCSSettings &settings) {
        *settingsData_ = settings;
    }

    void SetBlendMode(BlendMode blendMode) {
        particleGroupData_.blendMode = blendMode;
    }

    std::string GetGroupName() { return particleGroupData_.groupName; }

    PrimitiveType GetPrimitiveType() { return type_; }

    std::string GetModelPath() { return modelFilePath_; }

    // 生存パーティクル数を取得
    uint32_t GetAliveParticleCount();

    // カウント処理を実行
    void CountAliveParticles();

  private:
    /// ===================================
    /// private methods
    /// ===================================
    void Initialize(uint32_t maxParticleCount = 10000);
    void InitParticle();
    void CreateOutputParticleResource();
    void CreatePerViewResource();
    void CreateMaterialResource();
    void CreateIndexResource();
    void CreateVertexResource();
    void CreatePerFrameResource();
    void CreateFreeListIndexResource();
    void CreateFreeListResource();
    void CreateSettingsResource();
    void CreateAliveCountResource();

  private:
    /// ===================================
    /// private variaus
    /// ===================================
    Microsoft::WRL::ComPtr<ID3D12Resource> outputParticleResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputParticleSrvHandle_{};
    uint32_t outputParticleSrvIndex_ = 0;
    uint32_t outputParticleSrvForVSIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
    uint32_t *indexData_{};
    // バッファリソースの使い道を補足するバッファビュー
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_{};
    PerView *perViewData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
    ParticleMaterial *materialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    VertexData *vertexData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_ = nullptr;
    PerFrame *perFrameData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListIndexSrvHandle_{};
    uint32_t freeListIndexSrvIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListSrvHandle_{};
    uint32_t freeListSrvIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> settingsResource_{};
    ParticleCSSettings *settingsData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCountResource_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCountReadbackResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> aliveCountSrvHandle_{};
    uint32_t aliveCountSrvIndex_ = 0;
    uint32_t cachedAliveCount_ = 0;

    ID3D12GraphicsCommandList *commandList{};

    ParticleCommon *particleCommon_{};
    DirectXCommon *dxCommon_{};
    SrvManager *srvManager_{};
    TextureManager *texManager_{};
    Model *model_{};
    ModelData modelData{};
    std::string modelFilePath_{};

    PrimitiveType type_ = PrimitiveType::None;
    ParticleCSGroupData particleGroupData_{};

    std::string texPath_ = "debug/circle2.png";

    float frequency_ = 0.1f;
    bool isRandomColor_ = false;
    bool isInitialized_ = false;
};

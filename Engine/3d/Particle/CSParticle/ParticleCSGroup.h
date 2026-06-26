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

namespace Hagine {

/// <summary>
/// GPUパーティクル1グループ分のリソースとコンピュート処理を管理するクラス
/// パーティクルバッファ・生存リスト・各種定数バッファを保持し、
/// 更新コンピュートのディスパッチ（indirect対応）と描画用データの提供を行う
/// </summary>
class ParticleCSGroup {
  public:
    /// ===================================
    /// public methods
    /// ===================================

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~ParticleCSGroup();
    ParticleCSGroupData CreateParticleGroup(const std::string &groupName, const std::string &filename, uint32_t maxParticleCount = 10000, const std::string &texturePath = {}, BlendMode blendMode = BlendMode::kAdd);
    ParticleCSGroupData CreatePrimitiveParticleGroup(const std::string &groupName, PrimitiveType type, uint32_t maxParticleCount = 10000, const std::string &texturePath = {}, BlendMode blendMode = BlendMode::kAdd);
    void Update(const ViewProjection &vp);
    void DrawImGui();
    int CalculateOptimalEmitCount() const;
    void UpdateParticleCSDisPatch(
        std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> fieldsSrvHandle,
        Microsoft::WRL::ComPtr<ID3D12Resource> fieldCountResource,
        std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> overrideSrvHandle,
        ID3D12GraphicsCommandList *cmdList = nullptr);
    ParticleCSGroupData GetParticleGroupData() { return particleGroupData_; }

    /// ===================================
    /// Getter
    /// ===================================

    // SoA: Compute(Emit/Update) が u0-u5 にバインドする UAV の GPU ハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetLifeUavGpu() const { return soaLife_.uavHandle.second; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetDrawCoreUavGpu() const { return soaDrawCore_.uavHandle.second; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSimCoreUavGpu() const { return soaSimCore_.uavHandle.second; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetTrailUavGpu() const { return soaTrail_.uavHandle.second; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetRotationUavGpu() const { return soaRotation_.uavHandle.second; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetOverrideUavGpu() const { return soaOverride_.uavHandle.second; }
    // SoA: 描画VS が読む SRV インデックス（t0:DrawCore / t4:Rotation）
    uint32_t GetDrawCoreSrvForVSIndex() const { return soaDrawCore_.srvForVSIndex; }
    uint32_t GetRotationSrvForVSIndex() const { return soaRotation_.srvForVSIndex; }
    // 生存コンパクション用ハンドル/インデックス
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetAliveListUavHandle() const { return aliveListUavHandle_; }
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetAliveCounterUavHandle() const { return aliveCounterUavHandle_; }
    // 描画(VS)が読む生存リスト/カウンタの SRV インデックス。
    uint32_t GetAliveListSrvForVSIndex() const { return aliveListSrvForVSIndex_; }
    uint32_t GetAliveCounterSrvForVSIndex() const { return aliveCounterSrvForVSIndex_; }
    // 直近フレームに読み戻した生存数(描画 instanceCount のヒント)
    uint32_t GetAliveDrawCount() const { return aliveDrawCount_; }
    // 生存コンパクションカウンタを 0 にリセットする 1スレッドパス
    void ResetAliveCounterDispatch(ID3D12GraphicsCommandList *cmdList);
    // 生存数を readback バッファへコピーする（compute キュー上で記録すること）
    void RecordAliveCountReadback(ID3D12GraphicsCommandList *computeCmdList);
    // readback 済みの生存数を CPU へ取り込む（aliveDrawCount_ を更新）
    void FetchAliveDrawCount();
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetFreeListIndexSrvHandle() const { return freeListIndexSrvHandle_; }
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetFreeListTrailIndexSrvHandle() const { return freeListTrailIndexSrvHandle_; }
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetFreeListSrvHandle() const { return freeListSrvHandle_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetPerFrameResource() const { return perFrameResource_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetMaterialResource() const { return materialResource_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetPerViewResource() const { return perViewResource_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetSettingsResource() const { return settingsResource_; }
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return indexBufferView_; }
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return vertexBufferView_; }
    ModelData GetModelData() const { return modelData_; }
    PerFrame *GetPerFrameData() const { return perFrameData_; }
    uint32_t GetMaxParticleCount() const { return settingsData_->maxParticleCount; }
    ParticleCSSettings *GetSettingsData() const { return settingsData_; }
    std::string GetGroupName() { return particleGroupData_.groupName; }
    PrimitiveType GetPrimitiveType() { return type_; }
    std::string GetModelPath() { return modelFilePath_; }
    uint32_t GetAliveParticleCount();
    PerView *GetPerView() { return perViewData_; }

    /// ===================================
    /// Setter
    /// ===================================

    void SetFrequency(float frequency) { frequency_ = frequency; }
    void SetSettingData(const ParticleCSSettings &settings) { *settingsData_ = settings; }
    void SetBlendMode(BlendMode blendMode) { particleGroupData_.blendMode = blendMode; }
    void SetPerView(PerView *perViewData) { perViewData_ = perViewData; }
    void SetBillboard(bool flag) { perViewData_->enableBillboard = flag; }

    // カウント処理を実行
    void CountAliveParticles();

    // プール再利用時に GPU 上のパーティクル状態とフリーリストを初期化し直す。
    // （新規生成時の InitParticle と同等。バッファ/SRV は再確保しない）
    void ResetForReuse() { InitParticle(); }

  private:
    /// ===================================
    /// private methods
    /// ===================================
    void Initialize(uint32_t maxParticleCount = 10000);
    void InitParticle();
    void CreateParticleSoABuffers();
    void CreatePerViewResource();
    void CreateMaterialResource();
    void CreateIndexResource();
    void CreateVertexResource();
    void CreatePerFrameResource();
    void CreateFreeListIndexResource();
    void CreateFreeListTrailIndexResource();
    void CopyDebugDataToReadback();
    void CreateFreeListResource();
    void CreateSettingsResource();
    void CreateAliveCountResource();
    void CreateAliveListResources();

  private:
    /// ===================================
    /// private variaus
    /// ===================================
    // ===== GPUパーティクル SoA バッファ（旧 outputParticleResource_ を機能別に分割） =====
    // 各バッファは Compute(Emit/Update) 用 UAV を持つ。描画VSが読む DrawCore/Rotation のみ
    // 追加で SRV(srvForVSIndex) を持つ。
    struct SoABuffer {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> uavHandle{};
        uint32_t uavIndex = 0;
        uint32_t srvForVSIndex = 0;
    };
    SoABuffer soaLife_;     // float            (生存判定。早期returnで4Bのみ読む)
    SoABuffer soaDrawCore_; // CSParticleDrawCore (translate/scale/velocity/color)
    SoABuffer soaSimCore_;  // CSParticleSimCore  (currentTime/initialScale/isTrailParticle)
    SoABuffer soaTrail_;    // CSParticleTrail    (parentIndex/lastTrailPosition/trailSpawnDistance)
    SoABuffer soaRotation_; // CSParticleRotation (rotation/angularVelocity)
    SoABuffer soaOverride_; // CSParticleOverride (settingsOverrideFlags uint2)

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

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListTrailIndexResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListTrailIndexSrvHandle_{};
    uint32_t freeListTrailIndexSrvIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListSrvHandle_{};
    uint32_t freeListSrvIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> settingsResource_{};
    ParticleCSSettings *settingsData_ = nullptr;

    // ===== 生存コンパクション (Phase 1) =====
    // 生存パーティクルの slot index を詰めるバッファ (UAV: compute u4 / SRV: VS t2)
    Microsoft::WRL::ComPtr<ID3D12Resource> aliveListResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> aliveListUavHandle_{};
    uint32_t aliveListUavIndex_ = 0;
    uint32_t aliveListSrvForVSIndex_ = 0;
    // 生存数アトミックカウンタ (UAV: compute u5 / SRV: VS t3)
    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCounterResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> aliveCounterUavHandle_{};
    uint32_t aliveCounterUavIndex_ = 0;
    uint32_t aliveCounterSrvForVSIndex_ = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCounterReadbackResource_{};
    uint32_t aliveDrawCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCountResource_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCountReadbackResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> aliveCountSrvHandle_{};
    uint32_t aliveCountSrvIndex_ = 0;
    uint32_t cachedAliveCount_ = 0;

    ID3D12GraphicsCommandList *commandList_{};
    ID3D12GraphicsCommandList *computeCommandList_{};

    ParticleCommon *particleCommon_{};
    DirectXCommon *dxCommon_{};
    SrvManager *srvManager_{};
    TextureManager *texManager_{};
    Model *model_{};
    ModelData modelData_{};
    std::string modelFilePath_{};

    PrimitiveType type_ = PrimitiveType::None;
    ParticleCSGroupData particleGroupData_{};

    std::string texPath_ = "debug/circle2.png";

    float frequency_ = 0.1f;
    bool isRandomColor_ = false;
    bool isInitialized_ = false;

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexReadbackBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListTrailIndexReadbackBuffer_;

    int32_t debugFreeListCount_ = 0;
    int32_t debugFreeListTailCount_ = 0;
};
} // namespace Hagine

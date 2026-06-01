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
    void UpdateParticleCSDisPatch(
        std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> fieldsSrvHandle,
        Microsoft::WRL::ComPtr<ID3D12Resource> fieldCountResource,
        std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> overrideSrvHandle,
        ID3D12GraphicsCommandList *cmdList = nullptr);
    ParticleCSGroupData GetParticleGroupData() { return particleGroupData_; }

    /// ===================================
    /// Getter
    /// ===================================

    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetOutputParticleSrvHandle() const { return outputParticleSrvHandle_; }
    // 生存コンパクション用ハンドル/インデックス
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetAliveListUavHandle() const { return aliveListUavHandle_; }
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetAliveCounterUavHandle() const { return aliveCounterUavHandle_; }
    // 描画(VS)が読む生存リスト/カウンタの SRV インデックス。
    // indirect モード有効時は「このフレームの出力(Out)」側を返す（ping-pong）。
    // 既定(OFF)では従来どおり単一バッファを返すため挙動不変。
    uint32_t GetAliveListSrvForVSIndex() const {
        return (useIndirectSim_ && indirectResourcesCreated_)
                   ? (writeToB_ ? aliveListBSrvForVSIndex_ : aliveListSrvForVSIndex_)
                   : aliveListSrvForVSIndex_;
    }
    uint32_t GetAliveCounterSrvForVSIndex() const {
        return (useIndirectSim_ && indirectResourcesCreated_)
                   ? (writeToB_ ? aliveCounterBSrvForVSIndex_ : aliveCounterSrvForVSIndex_)
                   : aliveCounterSrvForVSIndex_;
    }
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
    Microsoft::WRL::ComPtr<ID3D12Resource> GetOutputParticleResource() const { return outputParticleResource_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetPerViewResource() const { return perViewResource_; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetSettingsResource() const { return settingsResource_; }
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return indexBufferView_; }
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return vertexBufferView_; }
    uint32_t GetOutputParticleSrvIndex() const { return outputParticleSrvIndex_; }
    uint32_t GetOutputParticleSrvForVSIndex() const { return outputParticleSrvForVSIndex_; }
    ModelData GetModelData() const { return modelData; }
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

    // ===================================
    // Phase 3: ping-pong + DispatchIndirect（既定 OFF）
    //   sim を「生存数ぶんだけ」DispatchIndirect で起動する。
    //   旧パス（全 N ディスパッチ）は非改変のまま温存し、フラグで切替える。
    // ===================================
    // maxParticleCount がこの値以上のグループは、生成時に自動で indirect 更新を有効化する。
    //   理由: 更新の全 N ディスパッチは毎フレーム maxParticleCount ぶんスレッドを起動し、
    //   各スレッドが Particle(約150B) をロードして死スロットを弾く。max が大きいと生存数に
    //   関係なく帯域を食い潰す（例: 1000万 → 毎フレーム約1.5GB読み）。indirect は生存数ぶん
    //   しか起動しないためこれを解消する。
    //   小容量グループは全 N が十分軽く、indirect のオーバーヘッド（間接参照＋追加ディスパッチ/
    //   バリア）だけが乗るため OFF のままにする。ImGui トグルでいつでも手動上書き可。
    static constexpr uint32_t kIndirectSimAutoThreshold = 262144; // 256K スロット
    void SetUseIndirectSim(bool enable);
    bool GetUseIndirectSim() const { return useIndirectSim_; }
    bool IsIndirectSimReady() const { return useIndirectSim_ && indirectResourcesCreated_; }

    // B 系 aliveList/counter と dispatchArgs、および専用 PSO を遅延生成する。
    // 既に生成済みなら no-op。SetUseIndirectSim(true) 内で呼ばれる。
    void EnsureIndirectSimResources();

    // フレーム先頭で In/Out を入れ替える（描画は Out を読むため、compute 開始時に呼ぶ）。
    void SwapIndirectAliveLists() { writeToB_ = !writeToB_; }

    // 出力(Out)カウンタを 0 にリセット（emit / update の append より前に呼ぶ）。
    void ResetIndirectOutCounterDispatch(ID3D12GraphicsCommandList *cmdList);
    // 入力(In)生存数から間接ディスパッチ引数を生成し UAV->INDIRECT_ARGUMENT へ遷移する。
    void BuildDispatchArgsDispatch(ID3D12GraphicsCommandList *cmdList);
    // UpdateParticleIndirect を ExecuteIndirect で実行（In を処理し Out へ append）。
    void UpdateParticleIndirectDispatch(
        std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> fieldsSrvHandle,
        Microsoft::WRL::ComPtr<ID3D12Resource> fieldCountResource,
        std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> overrideSrvHandle,
        ID3D12GraphicsCommandList *cmdList);

    // emit が append する出力(Out) aliveList/counter の UAV ハンドル（u4/u5 に束ねる）。
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetIndirectOutAliveListUavHandle() const {
        return writeToB_ ? aliveListBUavHandle_ : aliveListUavHandle_;
    }
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetIndirectOutAliveCounterUavHandle() const {
        return writeToB_ ? aliveCounterBUavHandle_ : aliveCounterUavHandle_;
    }

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
    void CreateFreeListTrailIndexResource();
    void CopyDebugDataToReadback();
    void CreateFreeListResource();
    void CreateSettingsResource();
    void CreateAliveCountResource();
    void CreateAliveListResources();
    // Phase 3: B 系 aliveList/counter ＋ dispatchArgs を生成
    void CreateIndirectSimResources();

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

    // ===== Phase 3: ping-pong + DispatchIndirect（既定OFF・遅延生成） =====
    bool useIndirectSim_ = false;          // indirect sim を使うか（OFF=従来パス）
    bool indirectResourcesCreated_ = false; // B/dispatchArgs/PSO を生成済みか
    bool writeToB_ = true;                  // このフレームの出力(Out)が B 側か（毎フレームトグル）
    bool indirectBootstrap_ = false;        // 初回のみ入力(In)カウンタも 0 にして空リスト扱いにする

    // aliveList B（A=既存 aliveListResource_ と ping-pong する相方）
    Microsoft::WRL::ComPtr<ID3D12Resource> aliveListBResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> aliveListBUavHandle_{};
    uint32_t aliveListBUavIndex_ = 0;
    uint32_t aliveListBSrvForVSIndex_ = 0;
    // aliveCounter B
    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCounterBResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> aliveCounterBUavHandle_{};
    uint32_t aliveCounterBUavIndex_ = 0;
    uint32_t aliveCounterBSrvForVSIndex_ = 0;
    // 間接ディスパッチ引数（D3D12_DISPATCH_ARGUMENTS = uint x3）
    Microsoft::WRL::ComPtr<ID3D12Resource> dispatchArgsResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> dispatchArgsUavHandle_{};
    uint32_t dispatchArgsUavIndex_ = 0;
    D3D12_RESOURCE_STATES dispatchArgsState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    // Phase 5: グループ単位トレイル予算カウンタ（u8・毎フレーム 0 リセット）
    Microsoft::WRL::ComPtr<ID3D12Resource> trailBudgetResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> trailBudgetUavHandle_{};
    uint32_t trailBudgetUavIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCountResource_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> aliveCountReadbackResource_{};
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> aliveCountSrvHandle_{};
    uint32_t aliveCountSrvIndex_ = 0;
    uint32_t cachedAliveCount_ = 0;

    ID3D12GraphicsCommandList *commandList{};
    ID3D12GraphicsCommandList *computeCommandList_{};

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

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexReadbackBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListTrailIndexReadbackBuffer_;

    int32_t debugFreeListCount_ = 0;
    int32_t debugFreeListTailCount_ = 0;
};

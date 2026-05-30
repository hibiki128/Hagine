#pragma once
#include "Graphics/Model/ModelManager.h"
#include "Particle/ParticleStruct.h"
#include "ParticleCSGroup.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <DirectXCommon.h>
#include <Graphics/Srv/SrvManager.h>
#include <Particle/ParticleCommon.h>
#include <set>
#include <vector>

class ParticleCSEmitter {

  public:
    /// ==============================================
    /// public methods
    /// ==============================================
    ParticleCSEmitter() = default;
    // 破棄時に保有する独立グループを再利用プールへ返却する（バッファ累積を防ぐ）
    ~ParticleCSEmitter();

    void Initialize(const std::string &name);
    void Initialize(const std::string &name, const std::string &modelPath);
    void Initialize(const std::string &name, PrimitiveType primitiveType);
    void Update();
    void Draw(const ViewProjection &vp);
    void DrawImGui();
    void AddParticleGroup(ParticleCSGroup *particleGroup);
    void RemoveParticleGroup(const std::string &groupName);
    void EmitOnce();

    EmitterMesh GetEmitterMesh() const {
        if (emitterMeshData_)
            return *emitterMeshData_;
        return EmitterMesh{};
    }

    void SetName(const std::string &name) { name_ = name; }
    void SetFrequency(float frequency) {
        if (emitterMeshData_)
            emitterMeshData_->frequency = frequency;
    }
    void SetActive(bool isActive) { isActive_ = isActive; }
    void SetAuto(bool isAuto) { isAuto_ = isAuto; }
    bool GetAuto() const { return isAuto_; }
    // エミッターのワイヤーフレーム描画の表示・非表示を切り替える
    void SetVisible(bool isVisible) { isVisible_ = isVisible; }

    bool IsGizmoSelectable() const { return isGizmoSelectable_; }
    void SetGizmoSelectable(bool selectable) { isGizmoSelectable_ = selectable; }

    std::string GetName() const { return name_; }
    void SetEnableGravity(bool enable) {
        for (auto &group : particleGroups_) {
            group->GetSettingsData()->enableGravity = enable;
        }
    }

    void SetEnableLifeTimeScale(bool enable) {
        for (auto &group : particleGroups_) {
            group->GetSettingsData()->enableLifetimeScale = enable;
        }
    }

    void SetMinVelocity(Vector3 minVelocity) {
        for (auto &group : particleGroups_) {
            group->GetSettingsData()->velocityMin = minVelocity;
        }
    }

    void SetMaxVelocity(Vector3 maxVelocity) {
        for (auto &group : particleGroups_) {
            group->GetSettingsData()->velocityMax = maxVelocity;
        }
    }

    std::unique_ptr<ParticleCSEmitter> Clone() const;

    void SetTranslate(Vector3 transform) {
        if (emitterMeshData_)
            emitterMeshData_->translate = transform;
    }

    void SetStartColor(Vector4 color) {
        for (auto &group : particleGroups_) {
            group->GetSettingsData()->startColor = color;
        }
    }

    void SetEndColor(Vector4 color) {
        for (auto &group : particleGroups_) {
            group->GetSettingsData()->endColor = color;
        }
    }

    void SetRotation(Quaternion rotation) {
        if (emitterMeshData_)
            emitterMeshData_->rotation = -rotation;
    }

    void SetScale(Vector3 scale) {
        if (emitterMeshData_)
            emitterMeshData_->scale = scale;
    }

    void SetAnchorPoint(Vector3 anchor) {
        if (emitterMeshData_)
            emitterMeshData_->anchorPoint = anchor;
    }

    void SetReceiveFields(bool receive) { receiveFields_ = receive; }
    bool GetReceiveFields() const { return receiveFields_; }

    // フィールド接触時のみEmitするモード
    // true  = enableEmitSpawnフィールドが存在する場合、シェーダー側で
    //         フィールド球内のランダム点→エミッター表面投影でEmit位置を決定する。
    //         emitCount は fieldContactEmitCount_ の値を使用する。
    // false = 通常の自動Emit（フィールドは UpdateCS での物理影響のみ）
    void SetEmitOnlyOnFieldContact(bool enable) { emitOnlyOnFieldContact_ = enable; }
    bool GetEmitOnlyOnFieldContact() const { return emitOnlyOnFieldContact_; }

    // フィールド接触Emitモード時の1フレームあたり発生数
    // 全スレッドがフィールド接触部分にEmitするので、
    // 少ない値（例: 500〜2000）でも十分密になる
    void SetFieldContactEmitCount(uint32_t count) { fieldContactEmitCount_ = count; }
    uint32_t GetFieldContactEmitCount() const { return fieldContactEmitCount_; }

    // フィールドグループID（このIDと一致するフィールドのみ影響を受ける）
    // -1 = 全フィールドから影響を受ける（デフォルト）
    void SetFieldGroupId(int32_t id) { fieldGroupId_ = id; }
    int32_t GetFieldGroupId() const { return fieldGroupId_; }
    bool GetAcitve() const { return isActive_; }

    Vector3 GetAnchorPoint() const {
        if (emitterMeshData_)
            return emitterMeshData_->anchorPoint;
        return Vector3(0.5f, 0.5f, 0.5f);
    }

    Vector3 GetTranslate() const {
        if (emitterMeshData_)
            return emitterMeshData_->translate;
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    Quaternion GetRotation() const {
        if (emitterMeshData_)
            return emitterMeshData_->rotation;
        return Quaternion::IdentityQuaternion();
    }

    Vector3 GetScale() const {
        if (emitterMeshData_)
            return emitterMeshData_->scale;
        return Vector3(1.0f, 1.0f, 1.0f);
    }

    Vector3 GetRadius() const {
        return Vector3(1.0f, 1.0f, 1.0f);
    }

    static void ClearNameCounter() {
        GetNameCounter().clear();
    }

    static void ClearNameCounter(const std::string &baseName) {
        GetNameCounter().erase(baseName);
    }

    size_t GetTotalAliveParticles();

    // グループごとの統計情報
    struct GroupStatistics {
        std::string groupName;
        uint32_t aliveCount;
    };

    // 全グループの統計を取得
    std::vector<GroupStatistics> GetGroupStatistics();

  private:
    /// ==============================================
    /// private methods
    /// ==============================================

    void CreateEmitterMeshResource();
    void EmitterUpdate();
    void EmitterDisPatch(ID3D12GraphicsCommandList *cmdList = nullptr);

  public:
    // ---- バッチ非同期コンピュート用 2フェーズ API ----
    /// Compute フェーズ: Emit/Update を Compute Queue に記録するだけ（Execute しない）
    void DrawCompute(const ViewProjection &vp);
    /// Graphics フェーズ: Count + DrawIndexed を Direct Queue で実行（Compute 済み前提）
    void DrawGraphics(const ViewProjection &vp);
    // プレビュー隔離描画: 外部 per-view CB（プレビューVP）で Graphics のみ描画する。
    // RT/DSV/Viewport/DescriptorHeap は呼び出し側で設定済みであること。ワイヤー(DrawEmitter)は描かない。
    void DrawGraphicsForPreview(D3D12_GPU_VIRTUAL_ADDRESS perViewGpuAddress);
    void DrawEmitter();

    void SaveSetting();
    void LoadSetting();
    void LoadCloneSetting();

    void LoadModel(const std::string &modelPath);
    void LoadPrimitiveModel(PrimitiveType type);
    void CreateModelTriangles();
    void CreateModelEdges();

  private:
    /// ==============================================
    /// private variables
    /// ==============================================
    ///

    static std::unordered_map<std::string, int> &GetNameCounter() {
        static std::unordered_map<std::string, int> nameCounter;
        return nameCounter;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> emitterMeshResource_ = nullptr;
    EmitterMesh *emitterMeshData_ = nullptr;

    DirectXCommon *dxCommon_ = nullptr;
    ID3D12GraphicsCommandList *commandList = nullptr;
    ParticleCommon *particleCommon_ = nullptr;
    SrvManager *srvManager_ = nullptr;

    std::vector<ParticleCSGroup *> particleGroups_;
    std::set<std::string> particleGroupNames_;

    Microsoft::WRL::ComPtr<ID3D12Resource> triangleInfoResource_ = nullptr;
    TriangleInfo *triangleInfoData_ = nullptr;
    std::vector<TriangleInfo> triangleInfoList_;

    Microsoft::WRL::ComPtr<ID3D12Resource> triangleCDFResource_ = nullptr;
    float *triangleCDFData_ = nullptr;
    std::vector<float> triangleCDF_;

    uint32_t triangleInfoSrvIndex_ = 0;
    uint32_t triangleCDFSrvIndex_ = 0;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> triangleInfoSrvHandle_;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> triangleCDFSrvHandle_;

    Microsoft::WRL::ComPtr<ID3D12Resource> edgeInfoResource_ = nullptr;
    EdgeInfo *edgeInfoData_ = nullptr;
    std::vector<EdgeInfo> edgeInfoList_;

    uint32_t edgeInfoSrvIndex_ = 0;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> edgeInfoSrvHandle_;

    // Model data for mesh emitters
    Model *model_ = nullptr;
    ModelData modelData_;
    std::string modelPath_;
    PrimitiveType primitiveType_ = PrimitiveType::None;

    std::string name_;
    int groupNum_ = 0;

    bool isAuto_ = false;
    bool isActive_ = false;
    bool isVisible_ = true;
    bool isGizmoSelectable_ = true;
    bool emitOnce_ = false;
    bool receiveFields_ = true;
    int32_t fieldGroupId_ = -1;             // -1=全フィールド対象, 0以上=同じIDのフィールドのみ対象
    bool emitOnlyOnFieldContact_ = false;   // true=フィールド接触時のみEmit（シェーダー側で位置を決定）
    uint32_t fieldContactEmitCount_ = 1000; // 接触Emitモード時の発生数/フレーム
};
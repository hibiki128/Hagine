#pragma once
#include "d3d12.h"
#include "wrl.h"
#include <cstdint>
#include <string>
#include <type/Matrix4x4.h>
#include <type/Vector3.h>

namespace Hagine {
class LightGroup;

class DirectXCommon;
class SrvManager;

/// @brief シャドウマップ管理クラス（シングルトン）
///
/// 平行光源からの深度テクスチャを生成し、影の描画に使用する。
/// ImGui でオン/オフ・パラメータ調整が可能。設定は JSON に保存/復元できる。
class ShadowMap {
public:
    static ShadowMap *GetInstance() {
        static ShadowMap instance;
        return &instance;
    }

    void Initialize();
    void Finalize();

    // シャドウパスの開始・終了
    void BeginShadowPass();
    void EndShadowPass();

    // ライト View×Projection 行列を再計算する（毎フレーム呼ぶ）
    void Update();

    // ---- アクセサ ----

    bool IsEnabled() const { return enabled_; }
    void SetEnabled(bool v) { enabled_ = v; }

    bool IsShadowPassActive() const { return shadowPassActive_; }
    void SetShadowPassActive(bool v) { shadowPassActive_ = v; }

    const Matrix4x4 &GetLightViewProjection() const { return lightViewProj_; }

    /// シャドウマップ SRV インデックス（SrvManager 管理）
    uint32_t GetShadowSrvIndex() const { return srvIndex_; }

    /// ShadowData 定数バッファの GPU アドレス
    D3D12_GPU_VIRTUAL_ADDRESS GetShadowDataGpuAddress() const;

    // ---- ImGui / 保存 ----
    void UpdateImGui();
    void SaveConfig(const std::string &fileName = "ShadowMap");
    void LoadConfig(const std::string &fileName = "ShadowMap");

    // ---- ライトパラメータ ----
    const Vector3 &GetLightDirection() const { return lightDir_; }
    void SetLightDirection(const Vector3 &dir) { lightDir_ = dir.Normalize(); }

    void SetLightTarget(const Vector3 &target) { lightTarget_ = target; }

private:
    ShadowMap() = default;
    ~ShadowMap() = default;
    ShadowMap(const ShadowMap &) = delete;
    ShadowMap &operator=(const ShadowMap &) = delete;

    // 内部ヘルパー
    Matrix4x4 MakeLightViewMatrix() const;
    void CreateShadowSRV();

    // ---- GPU リソース ----
    static constexpr UINT kShadowMapSize = 2048;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthResource_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};

    uint32_t srvIndex_ = UINT32_MAX;

    // ShadowData 定数バッファ
    struct ShadowDataGPU {
        int32_t enabled;
        float bias;
        float strength;
        float padding;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> shadowDataResource_;
    ShadowDataGPU *shadowDataPtr_ = nullptr;

    // ---- パラメータ ----
    Matrix4x4 lightViewProj_{};

    Vector3 lightDir_ = {0.0f, -1.0f, 0.5f};   // ライト方向（正規化）
    Vector3 lightTarget_ = {0.0f, 0.0f, 0.0f};  // シャドウ投影の中心
    float lightDistance_ = 80.0f;
    float orthoWidth_ = 40.0f;
    float orthoHeight_ = 40.0f;
    float nearZ_ = 0.1f;
    float farZ_ = 200.0f;
    float bias_ = 0.001f;
    float strength_ = 0.7f;
    bool enabled_ = true;
    bool shadowPassActive_ = false;
    bool syncWithDirectionalLight_ = true; // DirectionalLightの方向を自動同期するか

    D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    DirectXCommon *dxCommon_ = nullptr;
    SrvManager *srvManager_ = nullptr;
};
} // namespace Hagine

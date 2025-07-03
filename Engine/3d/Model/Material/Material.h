#pragma once
#include "wrl.h"
#include <Model/ModelStructs.h>
#include <Primitive/PrimitiveModel.h>
#include <d3d12.h>
class DirectXCommon;
class Material {
  public:
    /// ==========================================
    /// public methods
    /// ==========================================

    void Initialize();

    void LoadTexture();
    void PrimitiveInitialize(const PrimitiveType &type);

    void Draw(const Vector4 &color, bool lighting);

    MaterialData &GetMaterialData() { return materialData_; }
    const MaterialData &GetMaterialData() const { return materialData_; }

    // GPU側データの直接アクセス（描画時の一時的な変更用）
    MaterialDataGPU *GetMaterialDataGPU() { return materialDataGPU_; }

    void SetTexture(const std::string &texturePath);
    void SetEnvironmentCoefficients(float environmentCoefficients);

  private:
    /// ==========================================
    /// private variaus
    /// ==========================================
    MaterialData materialData_;                               // CPU側データ
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_; // GPUバッファ
    MaterialDataGPU *materialDataGPU_ = nullptr;              // GPUバッファへのポインタ
    DirectXCommon *dxCommon_ = nullptr;

  private:
    /// ==========================================
    /// private methods
    /// ==========================================

    MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename);

    void CreateMaterial();
    void UpdateGPUData();
};
#pragma once
#include "wrl.h"
#include <ModelStructs.h>
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

    void Draw(Vector4 &color, bool &lighting);

    MaterialData *GetMaterialDataGPU() { return materialDataGPU_; }
    MaterialData &GetMaterialData() { return materialData_; }
    void SetMaterialDataGPU(MaterialData *materialDataGPU) { materialDataGPU_ = materialDataGPU; }

  private:
    /// ==========================================
    /// private variaus
    /// ==========================================
    MaterialData materialData_;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    DirectXCommon *dxCommon_ = nullptr;
    MaterialData *materialDataGPU_;

  private:
    /// ==========================================
    /// private methods
    /// ==========================================

    MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename);

    void CreateMaterial();
};
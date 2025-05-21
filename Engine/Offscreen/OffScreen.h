#pragma once
#include "PipeLine/PipeLineManager.h"
#include "Srv/SrvManager.h"
#include "externals/nlohmann/json.hpp"
#include "myMath.h"
#include "wrl.h"
#include <Matrix4x4.h>
#include <Vector2.h>
#include <d3d12.h>
class DirectXCommon;
class OffScreen {
  public:
    void Initialize();

    void Draw();

    void Setting();

    void SetProjection(Matrix4x4 projectionMatrix) { projectionInverse_ = projectionMatrix; }

  private:
    void CreateSmooth();
    void CreateGauss();
    void CreateVignette();
    void CreateDepth();
    void CreateRadial();
    void CreateCinematic();
    void SaveToJson();
    void LoadFromJson(ShaderMode shaderMode);
    void LoadFromJson();

  private:
    DirectXCommon *dxCommon;
    SrvManager *srvManager_;
    PipeLineManager *psoManager_ = nullptr;
    ShaderMode shaderMode_ = ShaderMode::kNone;

    using json = nlohmann::json;

    struct KernelSettings {
        int kernelSize;
    };

    struct GaussianParams {
        int kernelSize;
        float sigma;
    };

    struct VignetteParameter {
        float vignetteStrength;
        float vignetteRadius;
        float vignetteExponent;
        float padding;
        Vector2 vignetteCenter;
    };

    struct Depth {
        Matrix4x4 projectionInverse;
        int kernelSize;
    };

    struct RadialBlur {
        Vector2 kCenter;
        float kBlurWidth;
    };

    struct Cinematic {
        Vector2 iResolution;
        float contrast;
        float saturation;
        float brightness;
    };

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource;
    // バッファリソース内のデータを指すポインタ
    VignetteParameter *vignetteData = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> smoothResource;
    // バッファリソース内のデータを指すポインタ
    KernelSettings *smoothData = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> gaussianResouce;
    // バッファリソース内のデータを指すポインタ
    GaussianParams *gaussianData = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> depthResouce;
    // バッファリソース内のデータを指すポインタ
    Depth *depthData = nullptr;

    Matrix4x4 projectionInverse_;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> radialResource;
    // バッファリソース内のデータを指すポインタ
    RadialBlur *radialData = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> cinematicResource;
    // バッファリソース内のデータを指すポインタ
    Cinematic *cinematicData = nullptr;
};

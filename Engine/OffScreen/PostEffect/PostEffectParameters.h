#pragma once
#include "Data/DataHandler.h"
#include <memory>
#include"Graphics/Srv/SrvManager.h"

class PostEffectParameters {
  public:
    void Initialize(DirectXCommon *dxCommon);
    void SetShaderParameters(ShaderMode mode, ID3D12GraphicsCommandList *commandList, SrvManager *srvManager, DirectXCommon *dxCommon);
    void UpdateTimeParameters(float deltaTime);

    void SaveParameters(DataHandler *dataHandler) const;
    void LoadParameters(DataHandler *dataHandler);

    // ImGui用のパラメータ設定UI
    void DrawParameterUI(ShaderMode mode);
    void SetProjection(Matrix4x4 projectionMatrix) { projectionInverse_ = projectionMatrix; }
  private:
    void CreateAllBuffers();

    void CreateSmooth();
    void CreateGauss();
    void CreateVignette();
    void CreateDepth();
    void CreateRadial();
    void CreateCinematic();
    void CreateDissolve();
    void CreateRandom();
    void CreateFocusLine();

  private:

      DirectXCommon *dxCommon_ = nullptr;

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

    struct Dissolve {
        float threshold;
        float edgeWidth;
        float _pad[2];
        Vector3 edgeColor;
        float _pad1;
        bool invert;
        float _pad2[3];
    };

    struct Random {
        float time;
    };

    struct FocusLine {
        float time;
        float lines;
        float width;
        float speed;
        float intensity;
        float centerRadius;
        float maxDistance;
        float padding1;
        Vector4 lineColor;
    };

    // バッファリソース（既存のまま）
    Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource;
    VignetteParameter *vignetteData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> smoothResource;
    KernelSettings *smoothData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> gaussianResouce;
    GaussianParams *gaussianData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthResouce;
    Depth *depthData = nullptr;

    Matrix4x4 projectionInverse_;

    Microsoft::WRL::ComPtr<ID3D12Resource> radialResource;
    RadialBlur *radialData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> cinematicResource;
    Cinematic *cinematicData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> dissolveResource;
    Dissolve *dissolveData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> randomResource;
    Random *randomData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> focusLineResource;
    FocusLine *focusLineData = nullptr;

    std::string texPath_ = "debug/noise0.png";
};
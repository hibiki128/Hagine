#pragma once
#include "Data/DataHandler.h"
#include <memory>
#include"Graphics/Srv/SrvManager.h"

namespace Hagine {
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
    void CreatePixelate();
    void CreateBloom();
    void CreateRetro();

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

    struct Pixelate {
        float blockSize;
        float centerX;
        float centerY;
    };
    
    struct Bloom {
        float bloomThreshold;
        float bloomIntensity;
        Vector2 texelSize;
    };

    struct Retro {
        float pixelSize;
        float colorLevels;
        float scanlineIntensity;
        float scanlineCount;
        float vignetteStrength;
        float chromaticOffset;
        float time;
        float resolutionX;
    };

    // バッファリソース（既存のまま）
    Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource_;
    VignetteParameter *vignetteData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> smoothResource_;
    KernelSettings *smoothData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> gaussianResouce_;
    GaussianParams *gaussianData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthResouce_;
    Depth *depthData_ = nullptr;

    Matrix4x4 projectionInverse_;

    Microsoft::WRL::ComPtr<ID3D12Resource> radialResource_;
    RadialBlur *radialData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> cinematicResource_;
    Cinematic *cinematicData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> dissolveResource_;
    Dissolve *dissolveData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> randomResource_;
    Random *randomData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> focusLineResource_;
    FocusLine *focusLineData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> pixelateResource_;
    Pixelate *pixelateData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> bloomResource_;
    Bloom *bloomData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> retroResource_;
    Retro *retroData_ = nullptr;

    std::string texPath_ = "debug/noise0.png";
};
} // namespace Hagine

#pragma once
#include "IPostEffectParams.h"
#include <type/Matrix4x4.h>
#include <type/Vector2.h>
#include <type/Vector3.h>
#include <type/Vector4.h>
#ifdef _DEBUG
#include "imgui.h"
#endif

// ============================================================
//  ヘルパー: 定数バッファ作成
// ============================================================
namespace PostEffectParamsHelper {
template <typename T>
static void CreateConstantBuffer(DirectXCommon *dxCommon,
                                 Microsoft::WRL::ComPtr<ID3D12Resource> &resource,
                                 T **mappedData) {
    const UINT64 size = (sizeof(T) + 255) & ~255;
    resource = dxCommon->CreateBufferResource(size);
    resource->Map(0, nullptr, reinterpret_cast<void **>(mappedData));
}
} // namespace PostEffectParamsHelper

// ============================================================
//  None
// ============================================================
class NoneParams : public IPostEffectParams {
  public:
    void Initialize(DirectXCommon *) override {}
    ShaderMode GetMode() const override { return ShaderMode::kNone; }
    void Apply(ID3D12GraphicsCommandList *, SrvManager *, DirectXCommon *) override {}
    void DrawUI() override {}
    void Save(DataHandler *, const std::string &) const override {}
    void Load(DataHandler *, const std::string &) override {}
};

// ============================================================
//  Vignette
// ============================================================
class VignetteParams : public IPostEffectParams {
  public:
    struct Data {
        float strength = 1.0f;
        float radius = 0.8f;
        float exponent = 2.0f;
        float padding = 0.0f;
        Vector2 center = {0.5f, 0.5f};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kVigneet; }

    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }

    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderFloat("強度", &data_->strength, 0.0f, 3.0f);
        ImGui::SliderFloat("半径", &data_->radius, 0.0f, 1.0f);
        ImGui::SliderFloat("指数", &data_->exponent, 0.1f, 5.0f);
        ImGui::SliderFloat2("中心", &data_->center.x, 0.0f, 1.0f);
#endif
    }

    void Save(DataHandler *h, const std::string &p) const override {
        h->Save<float>(p + "strength", data_->strength);
        h->Save<float>(p + "radius", data_->radius);
        h->Save<float>(p + "exponent", data_->exponent);
        h->Save<float>(p + "centerX", data_->center.x);
        h->Save<float>(p + "centerY", data_->center.y);
    }
    void Load(DataHandler *h, const std::string &p) override {
        data_->strength = h->Load<float>(p + "strength", 1.0f);
        data_->radius = h->Load<float>(p + "radius", 0.8f);
        data_->exponent = h->Load<float>(p + "exponent", 2.0f);
        data_->center.x = h->Load<float>(p + "centerX", 0.5f);
        data_->center.y = h->Load<float>(p + "centerY", 0.5f);
    }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Smooth (Box Filter)
// ============================================================
class SmoothParams : public IPostEffectParams {
  public:
    struct Data {
        int kernelSize = 3;
        int pad[3] = {};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kSmooth; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderInt("カーネルサイズ", &data_->kernelSize, 1, 15);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override { h->Save<int>(p + "kernelSize", data_->kernelSize); }
    void Load(DataHandler *h, const std::string &p) override { data_->kernelSize = h->Load<int>(p + "kernelSize", 3); }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Gaussian
// ============================================================
class GaussianParams : public IPostEffectParams {
  public:
    struct Data {
        int kernelSize = 5;
        float sigma = 1.0f;
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kGauss; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderInt("カーネルサイズ", &data_->kernelSize, 1, 15);
        ImGui::SliderFloat("シグマ", &data_->sigma, 0.1f, 10.0f);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override {
        h->Save<int>(p + "kernelSize", data_->kernelSize);
        h->Save<float>(p + "sigma", data_->sigma);
    }
    void Load(DataHandler *h, const std::string &p) override {
        data_->kernelSize = h->Load<int>(p + "kernelSize", 5);
        data_->sigma = h->Load<float>(p + "sigma", 1.0f);
    }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Outline (Edge Detection)
// ============================================================
class OutlineEdgeParams : public IPostEffectParams {
  public:
    struct Data {
        float edgeStrength = 1.0f;
        float pad[3] = {};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kOutLine; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderFloat("エッジ強度", &data_->edgeStrength, 0.0f, 5.0f);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override { h->Save<float>(p + "edgeStrength", data_->edgeStrength); }
    void Load(DataHandler *h, const std::string &p) override { data_->edgeStrength = h->Load<float>(p + "edgeStrength", 1.0f); }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Outline (Depth Based)
// ============================================================
class OutlineDepthParams : public IPostEffectParams {
  public:
    struct Data {
        Matrix4x4 projectionInverse;
        int kernelSize = 3;
        float pad[3] = {};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kDepth; }

    void SetProjectionInverse(const Matrix4x4 &mat) { data_->projectionInverse = mat; }

    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderInt("カーネルサイズ", &data_->kernelSize, 1, 9);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override { h->Save<int>(p + "kernelSize", data_->kernelSize); }
    void Load(DataHandler *h, const std::string &p) override { data_->kernelSize = h->Load<int>(p + "kernelSize", 3); }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Radial Blur
// ============================================================
class RadialBlurParams : public IPostEffectParams {
  public:
    struct Data {
        Vector2 center = {0.5f, 0.5f};
        float blurWidth = 0.01f;
        float pad = 0.0f;
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kBlur; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderFloat2("中心", &data_->center.x, 0.0f, 1.0f);
        ImGui::SliderFloat("ブラー幅", &data_->blurWidth, 0.0f, 0.1f);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override {
        h->Save<float>(p + "centerX", data_->center.x);
        h->Save<float>(p + "centerY", data_->center.y);
        h->Save<float>(p + "blurWidth", data_->blurWidth);
    }
    void Load(DataHandler *h, const std::string &p) override {
        data_->center.x = h->Load<float>(p + "centerX", 0.5f);
        data_->center.y = h->Load<float>(p + "centerY", 0.5f);
        data_->blurWidth = h->Load<float>(p + "blurWidth", 0.01f);
    }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Cinematic
// ============================================================
class CinematicParams : public IPostEffectParams {
  public:
    struct Data {
        Vector2 resolution = {1280.0f, 720.0f};
        float contrast = 1.0f;
        float saturation = 1.0f;
        float brightness = 1.0f;
        float pad[3] = {};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kCinematic; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderFloat("コントラスト", &data_->contrast, 0.0f, 3.0f);
        ImGui::SliderFloat("彩度", &data_->saturation, 0.0f, 3.0f);
        ImGui::SliderFloat("明度", &data_->brightness, 0.0f, 3.0f);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override {
        h->Save<float>(p + "contrast", data_->contrast);
        h->Save<float>(p + "saturation", data_->saturation);
        h->Save<float>(p + "brightness", data_->brightness);
    }
    void Load(DataHandler *h, const std::string &p) override {
        data_->contrast = h->Load<float>(p + "contrast", 1.0f);
        data_->saturation = h->Load<float>(p + "saturation", 1.0f);
        data_->brightness = h->Load<float>(p + "brightness", 1.0f);
    }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Dissolve
// ============================================================
class DissolveParams : public IPostEffectParams {
  public:
    struct Data {
        float threshold = 0.5f;
        float edgeWidth = 0.05f;
        float pad[2] = {};
        Vector3 edgeColor = {1.0f, 0.5f, 0.0f};
        float pad2 = 0.0f;
        int invert = 0;
        float pad3[3] = {};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kDissolve; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *srv, DirectXCommon *dxCommon) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderFloat("閾値", &data_->threshold, 0.0f, 1.0f);
        ImGui::SliderFloat("エッジ幅", &data_->edgeWidth, 0.0f, 0.5f);
        ImGui::ColorEdit3("エッジカラー", &data_->edgeColor.x);
        bool inv = data_->invert != 0;
        if (ImGui::Checkbox("反転", &inv)) {
            data_->invert = inv ? 1 : 0;
        }
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override {
        h->Save<float>(p + "threshold", data_->threshold);
        h->Save<float>(p + "edgeWidth", data_->edgeWidth);
        h->Save<float>(p + "edgeR", data_->edgeColor.x);
        h->Save<float>(p + "edgeG", data_->edgeColor.y);
        h->Save<float>(p + "edgeB", data_->edgeColor.z);
        h->Save<bool>(p + "invert", data_->invert != 0);
    }
    void Load(DataHandler *h, const std::string &p) override {
        data_->threshold = h->Load<float>(p + "threshold", 0.5f);
        data_->edgeWidth = h->Load<float>(p + "edgeWidth", 0.05f);
        data_->edgeColor.x = h->Load<float>(p + "edgeR", 1.0f);
        data_->edgeColor.y = h->Load<float>(p + "edgeG", 0.5f);
        data_->edgeColor.z = h->Load<float>(p + "edgeB", 0.0f);
        data_->invert = h->Load<bool>(p + "invert", false) ? 1 : 0;
    }

    void SetNoiseTextureSrvIndex(uint32_t idx) { noiseSrvIndex_ = idx; }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
    uint32_t noiseSrvIndex_ = 0;
};

// ============================================================
//  Random (Noise)
// ============================================================
class RandomParams : public IPostEffectParams {
  public:
    struct Data {
        float time = 0.0f;
        float pad[3] = {};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kRandom; }
    void UpdateTime(float dt) override { data_->time += dt; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {}
    void Save(DataHandler *, const std::string &) const override {}
    void Load(DataHandler *, const std::string &) override {}

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Focus Line (集中線)
// ============================================================
class FocusLineParams : public IPostEffectParams {
  public:
    struct Data {
        float time = 0.0f;
        float lines = 200.0f;
        float width = 0.01f;
        float speed = 1.0f;
        float intensity = 1.0f;
        float centerRadius = 0.1f;
        float maxDistance = 0.8f;
        float pad = 0.0f;
        Vector4 lineColor = {0.0f, 0.0f, 0.0f, 1.0f};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kFocusLine; }
    void UpdateTime(float dt) override { data_->time += dt; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderFloat("線の数", &data_->lines, 10.0f, 500.0f);
        ImGui::SliderFloat("線幅", &data_->width, 0.001f, 0.1f);
        ImGui::SliderFloat("速度", &data_->speed, 0.0f, 5.0f);
        ImGui::SliderFloat("強度", &data_->intensity, 0.0f, 2.0f);
        ImGui::SliderFloat("中心半径", &data_->centerRadius, 0.0f, 0.5f);
        ImGui::SliderFloat("最大距離", &data_->maxDistance, 0.0f, 1.0f);
        ImGui::ColorEdit4("線の色", &data_->lineColor.x);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override {
        h->Save<float>(p + "lines", data_->lines);
        h->Save<float>(p + "width", data_->width);
        h->Save<float>(p + "speed", data_->speed);
        h->Save<float>(p + "intensity", data_->intensity);
        h->Save<float>(p + "centerRadius", data_->centerRadius);
        h->Save<float>(p + "maxDistance", data_->maxDistance);
    }
    void Load(DataHandler *h, const std::string &p) override {
        data_->lines = h->Load<float>(p + "lines", 200.0f);
        data_->width = h->Load<float>(p + "width", 0.01f);
        data_->speed = h->Load<float>(p + "speed", 1.0f);
        data_->intensity = h->Load<float>(p + "intensity", 1.0f);
        data_->centerRadius = h->Load<float>(p + "centerRadius", 0.1f);
        data_->maxDistance = h->Load<float>(p + "maxDistance", 0.8f);
    }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Pixelate
// ============================================================
class PixelateParams : public IPostEffectParams {
  public:
    struct Data {
        float blockSize = 8.0f;
        float centerX = 0.5f;
        float centerY = 0.5f;
        float pad = 0.0f;
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kPixelate; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderFloat("ブロックサイズ", &data_->blockSize, 1.0f, 64.0f);
        ImGui::SliderFloat("中心X", &data_->centerX, 0.0f, 1.0f);
        ImGui::SliderFloat("中心Y", &data_->centerY, 0.0f, 1.0f);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override {
        h->Save<float>(p + "blockSize", data_->blockSize);
        h->Save<float>(p + "centerX", data_->centerX);
        h->Save<float>(p + "centerY", data_->centerY);
    }
    void Load(DataHandler *h, const std::string &p) override {
        data_->blockSize = h->Load<float>(p + "blockSize", 8.0f);
        data_->centerX = h->Load<float>(p + "centerX", 0.5f);
        data_->centerY = h->Load<float>(p + "centerY", 0.5f);
    }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};

// ============================================================
//  Bloom
// ============================================================
class BloomParams : public IPostEffectParams {
  public:
    struct Data {
        float threshold = 0.8f;
        float intensity = 1.0f;
        Vector2 texelSize = {1.0f / 1280.0f, 1.0f / 720.0f};
    };

    void Initialize(DirectXCommon *dxCommon) override {
        PostEffectParamsHelper::CreateConstantBuffer(dxCommon, resource_, &data_);
        *data_ = Data{};
    }
    ShaderMode GetMode() const override { return ShaderMode::kBloom; }
    void Apply(ID3D12GraphicsCommandList *cmd, SrvManager *, DirectXCommon *) override {
        cmd->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    }
    void DrawUI() override {
#ifdef _DEBUG
        ImGui::SliderFloat("閾値", &data_->threshold, 0.0f, 1.0f);
        ImGui::SliderFloat("強度", &data_->intensity, 0.0f, 5.0f);
#endif
    }
    void Save(DataHandler *h, const std::string &p) const override {
        h->Save<float>(p + "threshold", data_->threshold);
        h->Save<float>(p + "intensity", data_->intensity);
    }
    void Load(DataHandler *h, const std::string &p) override {
        data_->threshold = h->Load<float>(p + "threshold", 0.8f);
        data_->intensity = h->Load<float>(p + "intensity", 1.0f);
    }

    Data *GetData() { return data_; }
    const Data *GetData() const { return data_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Data *data_ = nullptr;
};
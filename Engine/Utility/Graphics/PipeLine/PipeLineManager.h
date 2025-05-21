#pragma once
#include "d3d12.h"
#include "wrl.h"
#include <DirectXCommon.h>
#include <string>
#include <unordered_map>

enum class BlendMode {
    // ブレンドなし
    kNone,
    // 通常ブレンド
    kNormal,
    // 加算
    kAdd,
    // 減算
    kSubtract,
    // 乗算
    kMultiply,
    // スクリーン
    kScreen,
};

enum class ShaderMode {
    kNone,
    kGray,
    kVigneet,
    kSmooth,
    kGauss,
    kOutLine,
    kDepth,
    kBlur,
    kCinematic,
};

enum class PipelineType {
    kStandard,
    kParticle,
    kSprite,
    kRender,
    kSkinning,
    kLine3d
};

class PipeLineManager {
  private:
    /// ====================================
    /// public method
    /// ====================================

    static PipeLineManager *instance;

    PipeLineManager() = default;
    ~PipeLineManager() = default;
    PipeLineManager(PipeLineManager &) = delete;
    PipeLineManager &operator=(PipeLineManager &) = delete;

  public:
    static PipeLineManager *GetInstance();

    void Finalize();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(DirectXCommon *dxCommon);

    /// <summary>
    /// パイプラインの取得
    /// </summary>
    Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipeline(PipelineType type, BlendMode blendMode = BlendMode::kNormal, ShaderMode shaderMode = ShaderMode::kNone);

    /// <summary>
    /// ルートシグネチャの取得
    /// </summary>
    Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature(PipelineType type, ShaderMode shaderMode = ShaderMode::kNone);

    /// <summary>
    /// 描画に必要な共通設定を行う
    /// </summary>
    void DrawCommonSetting(PipelineType type, BlendMode blendMode = BlendMode::kNormal, ShaderMode shaderMode = ShaderMode::kNone);

  private:
    // 内部パイプライン作成メソッド
    void CreateAllPipelines();

    // 標準パイプライン関連
    void CreateStandardPipelines();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode);

    // パーティクル関連
    void CreateParticlePipelines();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateParticleRootSignature();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateParticleGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode);

    // スプライト関連
    void CreateSpritePipelines();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateSpriteRootSignature();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateSpriteGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode);

    // レンダー関連
    void CreateRenderPipelines();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRenderRootSignature(ShaderMode shaderMode);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateRenderGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, ShaderMode shaderMode);

    // スキニング関連
    void CreateSkinningPipelines();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateSkinningRootSignature();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateSkinningGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

    // 3Dライン関連
    void CreateLine3dPipelines();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateLine3dRootSignature();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateLine3dGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

    // シェーダーモード別のルートシグネチャ作成
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateBaseRootSignature();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateGrayRootSignature();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateVignetteRootSignature();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateSmoothRootSignature();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateGaussRootSignature();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateOutLineRootSignature();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateDepthRootSignature();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateBlurRootSignature();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateCinematicRootSignature();

    // シェーダーモード別のパイプライン作成
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateNoneGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateGrayGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateVigneetGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateSmoothGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateGaussGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateOutLineGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateDepthGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateBlurGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateCinematicGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

  private:
    DirectXCommon *dxCommon_;

    // パイプラインとルートシグネチャの格納用マップ
    // キーは "PipelineType_BlendMode_ShaderMode" 形式の文字列
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelines_;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatures_;

    // キー文字列を生成するヘルパー関数
    std::string MakePipelineKey(PipelineType type, BlendMode blendMode, ShaderMode shaderMode);
    std::string MakeRootSignatureKey(PipelineType type, ShaderMode shaderMode);

    D3D12_STATIC_SAMPLER_DESC CreateCommonSamplerDesc();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateCommonRootSignature(bool hasCBV);

    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateFullScreenPostEffectPipeline(const std::wstring& psPath,Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature);

    D3D12_DEPTH_STENCIL_DESC SettingDepthStencilDesc(bool depth);
};
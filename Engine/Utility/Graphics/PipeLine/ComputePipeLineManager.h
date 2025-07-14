#pragma once
#include"d3d12.h"
#include"wrl.h"
#include"DirectXCommon.h"
#include"string"
#include"unordered_map"
#include "PipeLineManager.h"

enum class ComputePipelineType {
   kSkinning,
};


class ComputePipeLineManager {
  private:
    /// ====================================
    /// public method
    /// ====================================

    static ComputePipeLineManager *instance;

    ComputePipeLineManager() = default;
    ~ComputePipeLineManager() = default;
    ComputePipeLineManager(ComputePipeLineManager &) = delete;
    ComputePipeLineManager &operator=(ComputePipeLineManager &) = delete;

  public:
    static ComputePipeLineManager *GetInstance();

    void Finalize();
    
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(DirectXCommon *dxCommon);

    /// <summary>
    /// パイプラインの取得
    /// </summary>
    Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipeline(ComputePipelineType type, BlendMode blendMode = BlendMode::kNormal, ShaderMode shaderMode = ShaderMode::kNone);

    /// <summary>
    /// ルートシグネチャの取得
    /// </summary>
    Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature(ComputePipelineType type, ShaderMode shaderMode = ShaderMode::kNone);

    /// <summary>
    /// 描画に必要な共通設定を行う
    /// </summary>
    void DrawCommonSetting(ComputePipelineType type, BlendMode blendMode = BlendMode::kNormal, ShaderMode shaderMode = ShaderMode::kNone);

  private:
    // 内部パイプライン作成メソッド
    void CreateAllPipelines();

    // スキニング関連
    void CreateSkinningPipelines();
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateSkinningRootSignature();
    Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateSkinningGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

    private:
    DirectXCommon *dxCommon_;

    // パイプラインとルートシグネチャの格納用マップ
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelines_;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatures_;

    // キー文字列を生成するヘルパー関数
    std::string MakePipelineKey(ComputePipelineType type, BlendMode blendMode, ShaderMode shaderMode);
    std::string MakeRootSignatureKey(ComputePipelineType type, ShaderMode shaderMode);
};

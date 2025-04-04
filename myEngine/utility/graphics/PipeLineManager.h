#pragma once
#include "wrl.h"
#include "d3d12.h"
#include <DirectXCommon.h>

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

class PipeLineManager
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	/// <summary>
	/// グラフィックスパイプラインの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode_);

	/// <summary>
	/// パーティクル用のルートシグネチャの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateParticleRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	/// <summary>
	/// パーティクル用パイプラインの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateParticleGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode_);

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateSpriteRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	/// <summary>
	/// グラフィックスパイプラインの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateSpriteGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode_);

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRenderRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, ShaderMode shaderMode_);

	/// <summary>
	/// グラフィックスパイプラインの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateRenderGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, ShaderMode shaderMode_);

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateSkinningRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	/// <summary>
	/// グラフィックスパイプラインの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateSkinningGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateLine3dRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	/// <summary>
	/// グラフィックスパイプラインの作成
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateLine3dGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawCommonSetting(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatur);

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateBaseRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateVignetteRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateSmoothRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateGaussRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateDepthRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateBlurRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateCinematicRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateNoneGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateGrayGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateVigneetGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateSmoothGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateGaussGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateOutLineGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateDepthGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateBlurGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateCinematicGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);
private:
	DirectXCommon* dxCommon_;
};

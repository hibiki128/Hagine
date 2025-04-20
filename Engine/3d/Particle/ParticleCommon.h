#pragma once
#include"PipeLine/PipeLineManager.h"
#include"DirectXCommon.h"
class ParticleCommon
{
private:
	static ParticleCommon* instance;

	ParticleCommon() = default;
	~ParticleCommon() = default;
	ParticleCommon(ParticleCommon&) = delete;
	ParticleCommon& operator=(ParticleCommon&) = delete;

public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static ParticleCommon* GetInstance();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// 共通描画処理
	/// </summary>
	void DrawCommonSetting();

	void SetBlendMode(BlendMode blendMode);

	DirectXCommon* GetDxCommon()const { return dxCommon_; }
private:
	DirectXCommon* dxCommon_;
	std::unique_ptr<PipeLineManager>psoManager_;
	// ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState[5];

	BlendMode blendMode_ = BlendMode::kAdd;

};


#pragma once
#include"DirectXCommon.h"
#include"PipeLineManager.h"
class Object3dCommon
{
private:
	static Object3dCommon* instance;

	Object3dCommon() = default;
	~Object3dCommon() = default;
	Object3dCommon(Object3dCommon&) = delete;
	Object3dCommon& operator=(Object3dCommon&) = delete;

public: // メンバ関数

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static Object3dCommon* GetInstance();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	///  初期化
	/// </summary>
	void  Initialize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawCommonSetting();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void skinningDrawCommonSetting();

	/// <summary>
	///  getter
	/// </summary>
	/// <returns></returns>
	DirectXCommon* GetDxCommon()const { return dxCommon_; }

	/// <summary>
	/// ブレンドモードの切り替え
	/// </summary>
	void SetBlendMode(BlendMode blendMode);

private:
	DirectXCommon* dxCommon_;
	std::unique_ptr<PipeLineManager> psoManager_ = nullptr;

	// ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState[5];

	// ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> skinningRootSignature = nullptr;

	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skinningGraphicsPipelineState = nullptr;

	BlendMode blendMode_ = BlendMode::kNormal;

};


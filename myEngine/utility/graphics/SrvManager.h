#pragma once
#include <cstdint>
#include <queue>  // 空きインデックスの管理用
#include <wrl.h>
#include <d3d12.h>

class DirectXCommon;

class SrvManager
{
private:
	static SrvManager* instance;

	SrvManager() = default;
	~SrvManager() = default;
	SrvManager(SrvManager&) = delete;
	SrvManager& operator=(SrvManager&) = delete;


private:
	DirectXCommon* dxCommon = nullptr;

	// SRV用のでスクリプタサイズ
	uint32_t descriptorSize;
	// SRV用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	// 次に使用するSRVインデックス
	uint32_t useIndex = 0;
	// 空きインデックスを管理するキュー
	std::queue<uint32_t> freeIndices;  // 解放されたSRVインデックスを保存

public:
	// 最大SRV数(最大テクスチャ枚数)
	static const uint32_t kMaxSRVCount;

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static SrvManager* GetInstance();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 描画前処理
	/// </summary>
	void PreDraw();

	/// <summary>
	/// SRV生成(テクスチャ用)
	/// </summary>
	/// <param name="srvIndex"></param>
	/// <param name="pResource"></param>
	/// <param name="Format"></param>
	/// <param name="MipLevels"></param>
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);

	/// <summary>
	/// SRV生成(Structured Buffer用)
	/// </summary>
	/// <param name="srvIndex"></param>
	/// <param name="pResource"></param>
	/// <param name="numElements"></param>
	/// <param name="structureByteStride"></param>
	void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	/// <summary>
   /// SRV生成(RenderTexture用)
   /// </summary>
   /// <param name="srvIndex"></param>
   /// <param name="pResource"></param>
   /// <param name="numElements"></param>
   /// <param name="structureByteStride"></param>
	void CreateSRVforRenderTexture(uint32_t srvIndex, ID3D12Resource* pResource);

	/// <summary>
    /// SRV生成(Depth用)
    /// </summary>
    /// <param name="srvIndex"></param>
    /// <param name="pResource"></param>
    /// <param name="numElements"></param>
    /// <param name="structureByteStride"></param>
	void CreateSRVforDepth(uint32_t srvIndex, ID3D12Resource* pResource);

	/// <summary>
	/// インデックス割り当て
	/// </summary>
	/// <returns></returns>
	uint32_t Allocate();

	/// <summary>
	/// インデックス解放
	/// </summary>
	/// <param name="srvIndex"></param>
	void Free(uint32_t srvIndex);

	bool CanAllocate() const;

	/// <summary>
	/// getter
	/// </summary>
	/// <param name="index"></param>
	/// <returns></returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// setter
	/// </summary>
	/// <param name="RootParameterIndex"></param>
	/// <param name="srvIndex"></param>
	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);
};
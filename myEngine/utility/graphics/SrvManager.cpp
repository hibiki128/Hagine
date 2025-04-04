#include "SrvManager.h"
#include "DirectXCommon.h"

const uint32_t SrvManager::kMaxSRVCount = 8192;

SrvManager* SrvManager::instance = nullptr;

SrvManager* SrvManager::GetInstance()
{
	if (instance == nullptr) {
		instance = new SrvManager();
	}
	return instance;
}

void SrvManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void SrvManager::Initialize()
{
    this->dxCommon = DirectXCommon::GetInstance();

    // デスクリプタヒープの生成
    descriptorHeap = dxCommon->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
    // デスクリプタ1個分のサイズを取得して記録
    descriptorSize = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void SrvManager::PreDraw()
{
    // 描画用のDescriptorHeapの設定
    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorHeap.Get() };
    dxCommon->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
}

void SrvManager::CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

    srvDesc.Format = Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = MipLevels;

    dxCommon->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = structureByteStride;

	dxCommon->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handleCPU.ptr += (descriptorSize * index);
    return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handleGPU.ptr += (descriptorSize * index);
    return handleGPU;
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex)
{
    dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(RootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVforRenderTexture(uint32_t srvIndex, ID3D12Resource* pResource)
{
    // SRVの設定。FormatはResourceと同じにしておく
    D3D12_SHADER_RESOURCE_VIEW_DESC renderTextureSrvDesc{};
    renderTextureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    renderTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    renderTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    renderTextureSrvDesc.Texture2D.MipLevels = 1;

    // SRVの生成
    dxCommon->GetDevice()->CreateShaderResourceView(pResource, &renderTextureSrvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVforDepth(uint32_t srvIndex, ID3D12Resource* pResource)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSrvDesc{};
    //DXGI_FORMAT_D24_UNORM_S8_UINTのDepthを読むときはDZGI_FORMAT_R24_UNORM_X8_TYPELESS
    depthTextureSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthTextureSrvDesc.Texture2D.MipLevels = 1;

    dxCommon->GetDevice()->CreateShaderResourceView(pResource, &depthTextureSrvDesc, GetCPUDescriptorHandle(srvIndex));
}

uint32_t SrvManager::Allocate()
{
    // 空きインデックスがあれば、それを使用
    if (!freeIndices.empty()) {
        uint32_t index = freeIndices.front();
        freeIndices.pop();
        return index;
    }

    assert(useIndex < kMaxSRVCount);

    // returnする番号を一旦記録しておく
    uint32_t index = useIndex;
    // 次のインデックスへ進める
    useIndex++;
    // 上で記録した番号をreturn
    return index;
}

void SrvManager::Free(uint32_t srvIndex)
{
    // 解放するインデックスを空きリストに追加
    freeIndices.push(srvIndex);
}

// SRVの最大数チェック
bool SrvManager::CanAllocate() const
{
    // useIndexがkMaxSRVCount未満、もしくは空きインデックスがあればtrueを返す
    return useIndex < kMaxSRVCount || !freeIndices.empty();
}
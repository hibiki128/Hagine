#pragma once
#include "WinApp.h"
#include "chrono"
#include "d3d12.h"
#include "dxcapi.h"
#include "dxgi1_6.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "string"
#include "wrl.h"
#include <Vector4.h>

// DirectX基盤
class DirectXCommon {
  private:
    static DirectXCommon *instance;

    DirectXCommon() = default;
    ~DirectXCommon() = default;
    DirectXCommon(DirectXCommon &) = delete;
    DirectXCommon &operator=(DirectXCommon &) = delete;

  public: // メンバ関数
    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    /// <returns></returns>
    static DirectXCommon *GetInstance();

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(WinApp *winApp);

    /// <summary>
    /// オフスクリーンのSRV作成
    /// </summary>
    void CreateOffscreenSRV();

    /// <summary>
    /// depthのSRV作成
    /// </summary>
    void CreateDepthSRV();

    /// <summary>
    /// 描画前処理(RenderTexture)
    /// </summary>
    void PreRenderTexture();

    /// <summary>
    /// 描画前処理
    /// </summary>
    void PreDraw();

    /// <summary>
    /// 深度のバリア
    /// </summary>
    void TransitionDepthBarrier();

    /// <summary>
    /// 描画後処理
    /// </summary>
    void PostDraw();

    IDxcBlob *CompileShader(
        // CompilerするShaderファイルへのパス
        const std::wstring &filePath,
        // Compilerに使用するProfile
        const wchar_t *profile);

    // Resourceの作成
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

    // DirectX12のTextureResourceを作る
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata &metadata);

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_CLEAR_VALUE color);

    [[nodiscard]]
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage &mipImages);

#pragma region getter
    /// <summary>
    /// RTVの指定番号のCPUデスクリプタハンドルを取得する
    /// </summary>
    /// <param name="index"></param>
    /// <returns></returns>
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index);

    /// <summary>
    /// RTVの指定番号のGPUデスクリプタハンドルを取得する
    /// </summary>
    /// <param name="index"></param>
    /// <returns></returns>
    D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUDescriptorHandle(uint32_t index);

    /// <summary>
    /// DSVの指定番号のCPUデスクリプタハンドルを取得する
    /// </summary>
    /// <param name="index"></param>
    /// <returns></returns>
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle(uint32_t index);

    /// <summary>
    /// DSVの指定番号のGPUデスクリプタハンドルを取得する
    /// </summary>
    /// <param name="index"></param>
    /// <returns></returns>
    D3D12_GPU_DESCRIPTOR_HANDLE GetDSVGPUDescriptorHandle(uint32_t index);

    /// <summary>
    /// コマンドリストの取得
    /// </summary>
    /// <returns></returns>
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return commandList; }

    /// <summary>
    /// デバイスの取得
    /// </summary>
    /// <returns></returns>
    Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() { return device; }

    /// <summary>
    /// DescriptorHeapの作成
    /// </summary>
    /// <param name="heapType"></param>
    /// <param name="numDescriptors"></param>
    /// <param name="shaderVisible"></param>
    /// <returns></returns>
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
    ID3D12Resource *GetOffScreenResource() { return offScreenResource.Get(); }
    IDxcUtils *GetDxcUtils() { return dxcUtils; }
    IDxcCompiler3 *GetDxcCompiler() { return dxcCompiler; }

    Vector4 GetClearColor() const {
        return Vector4(
            clearColorValue.Color[0], // R
            clearColorValue.Color[1], // G
            clearColorValue.Color[2], // B
            clearColorValue.Color[3]  // A
        );
    }

    // バックバッファの数を取得
    size_t GetBackBufferCount() const { return backBuffers.size(); }

    D3D12_GPU_DESCRIPTOR_HANDLE GetOffScreenGPUHandle() { return offScreenSrvHandleGPU; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetOffScreenCPUHandle() { return offScreenSrvHandleCPU; }
    uint32_t GetOffScreenSrvIndex() { return offScreenSrvIndex; }

    D3D12_GPU_DESCRIPTOR_HANDLE GetDepthGPUHandle() { return depthSrvHandleGPU; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthCPUHandle() { return depthSrvHandleCPU; }
    uint32_t GetDepthSrvIndex() { return depthSrvIndex; }
#pragma endregion

  private: // メンバ関数
    /// <summary>
    /// デバイスの初期化
    /// </summary>
    void DeviceInitialize();

    /// <summary>
    /// コマンド関連の初期化
    /// </summary>
    void CommandInitialize();

    /// <summary>
    /// スワップチェーンの生成
    /// </summary>
    void CreateSwapChain();

    /// <summary>
    /// 深度バッファの生成
    /// </summary>
    void CreateDepthBaffer();

    /// <summary>
    /// 各種デスクリプタヒープの生成
    /// </summary>
    void CreateVariousDesctiptorHeap();

    /// <summary>
    /// レンダーターゲットビューの初期化
    /// </summary>
    void RenderTargetViewInitialize();

    /// <summary>
    /// 深度ステンシルビューの初期化
    /// </summary>
    void DepthStencilViewInitialize();

    /// <summary>
    /// フェンス生成
    /// </summary>
    void CreateFence();

    /// <summary>
    /// ビューポート矩形の初期化
    /// </summary>
    void ViewPortRectInitialize();

    /// <summary>
    /// シザリング矩形の初期化
    /// </summary>
    void ScissorRectInitialize();

    /// <summary>
    /// DXCコンパイラの生成
    /// </summary>
    void CreateDXCompiler();

    /// <summary>
    /// FPS固定初期化
    /// </summary>
    void InitializeFixFPS();

    /// <summary>
    /// FPS固定更新
    /// </summary>
    void UpdateFixFPS();

    /// <summary>
    ///  DepthStencilTextureの作成
    /// </summary>
    /// <param name="device"></param>
    /// <param name="width"></param>
    /// <param name="height"></param>
    /// <returns></returns>
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height);

    /// <summary>
    /// 指定番号のCPUデスクリプタハンドルを取得する
    /// </summary>
    /// <param name="descriptorHeap"></param>
    /// <param name="descriptorSize"></param>
    /// <param name="index"></param>
    /// <returns></returns>
    static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index); // CPU

    /// <summary>
    /// 指定番号のGPUデスクリプタハンドルを取得する
    /// </summary>
    /// <param name="descriptorHeap"></param>
    /// <param name="descriptorSize"></param>
    /// <param name="index"></param>
    /// <returns></returns>
    static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index); // GPU

    /// <summary>
    /// バリアを貼る
    /// </summary>
    /// <param name="pResource"></param>
    /// <param name="Before"></param>
    /// <param name="After"></param>
    void BarrierTransition(ID3D12Resource *pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After);

  private:
    // WindowsAPI
    WinApp *winApp_ = nullptr;
    // DirectX12デバイス
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    // DXGIファクトリ
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
    // コマンドキュー
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    // コマンドアロケータ
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    // コマンドリスト
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    // フェンス
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    // スワップチェーン
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers;
    Microsoft::WRL::ComPtr<ID3D12Resource> offScreenResource;
    D3D12_CLEAR_VALUE clearColorValue{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

  private:
    uint32_t descriptorSizeRTV;
    uint32_t descriptorSizeDSV;

    // DXCコンパイラ関連
    IDxcUtils *dxcUtils;
    IDxcCompiler3 *dxcCompiler;

    // RTVを2つ作るのでディスクリプタを2つ用意
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[3];
    // RTV
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    // スワップチェーン
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

    UINT64 fenceValue = 0;
    HANDLE fenceEvent;
    // ビューポート
    D3D12_VIEWPORT viewport{};
    // シザー矩形
    D3D12_RECT scissorRect{};
    // TransitionBarrierの設定
    D3D12_RESOURCE_BARRIER barrier{};
    // 現時点ではincludeはしないが、includeに対応するための設定を行っておく
    IDxcIncludeHandler *includeHandler;

    uint32_t offScreenSrvIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE offScreenSrvHandleCPU; // SRV作成時に必要なCPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE offScreenSrvHandleGPU; // 描画コマンドに必要なGPUハンドル

    uint32_t depthSrvIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE depthSrvHandleCPU; // SRV作成時に必要なCPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandleGPU; // 描画コマンドに必要なGPUハンドル

    // FPS固定用の時間計測
    std::chrono::steady_clock::time_point reference_;
    const double targetFPS_ = 60.0;
    const std::chrono::microseconds frameTime_{static_cast<uint64_t>(1000000.0 / targetFPS_)};
};

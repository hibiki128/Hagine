#include "PipeLineManager.h"
#include "ComputePipeLineManager.h"
#include <Debug/Log/Logger.h>
#include <d3dx12.h>
PipeLineManager *PipeLineManager::instance = nullptr;

PipeLineManager *PipeLineManager::GetInstance() {
    if (instance == nullptr) {
        instance = new PipeLineManager;
    }
    return instance;
}

void PipeLineManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void PipeLineManager::Initialize(DirectXCommon *dxCommon) {
    dxCommon_ = dxCommon;

    CreateAllPipelines();
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::GetPipeline(PipelineType type, BlendMode blendMode, ShaderMode shaderMode) {
    // キーを生成して対応するパイプラインを取得
    std::string key = MakePipelineKey(type, blendMode, shaderMode);

    // 対応するパイプラインが存在するか確認
    if (pipelines_.find(key) == pipelines_.end()) {
        // パイプラインが見つからない場合は警告を出して、デフォルトを返す
        assert(false && "指定されたパイプラインが存在しません");

        // デフォルトのパイプラインを返す (ここではStandard/Normal/Noneを想定)
        return pipelines_[MakePipelineKey(PipelineType::kStandard, BlendMode::kNormal, ShaderMode::kNone)];
    }

    return pipelines_[key];
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::GetRootSignature(PipelineType type, ShaderMode shaderMode) {
    // キーを生成して対応するルートシグネチャを取得
    std::string key = MakeRootSignatureKey(type, shaderMode);

    // 対応するルートシグネチャが存在するか確認
    if (rootSignatures_.find(key) == rootSignatures_.end()) {
        // ルートシグネチャが見つからない場合は警告を出して、デフォルトを返す
        assert(false && "指定されたルートシグネチャが存在しません");

        // デフォルトのルートシグネチャを返す
        return rootSignatures_[MakeRootSignatureKey(PipelineType::kStandard, ShaderMode::kNone)];
    }

    return rootSignatures_[key];
}

void PipeLineManager::DrawCommonSetting(PipelineType type, BlendMode blendMode, ShaderMode shaderMode) {
    // 指定されたタイプのパイプラインとルートシグネチャを取得
    auto pipeline = GetPipeline(type, blendMode, shaderMode);
    auto rootSignature = GetRootSignature(type, shaderMode);

    // グラフィックスコマンドリストにパイプラインとルートシグネチャを設定
    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    commandList->SetPipelineState(pipeline.Get());
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    if (type == PipelineType::kLine3d) {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    } else {
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
}

// スキニングパイプラインの作成
void PipeLineManager::CreateSkinningPipelines() {
    // ルートシグネチャを作成し、マップに格納
    auto rootSignature = CreateSkinningRootSignature();
    rootSignatures_[MakeRootSignatureKey(PipelineType::kSkinning, ShaderMode::kNone)] = rootSignature;

    // パイプラインを作成し、マップに格納
    auto pipeline = CreateSkinningGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(PipelineType::kSkinning, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

// 3Dラインパイプラインの作成
void PipeLineManager::CreateLine3dPipelines() {
    // ルートシグネチャを作成し、マップに格納
    auto rootSignature = CreateLine3dRootSignature();
    rootSignatures_[MakeRootSignatureKey(PipelineType::kLine3d, ShaderMode::kNone)] = rootSignature;

    // パイプラインを作成し、マップに格納
    auto pipeline = CreateLine3dGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(PipelineType::kLine3d, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

// キー文字列を生成するヘルパー関数
std::string PipeLineManager::MakePipelineKey(PipelineType type, BlendMode blendMode, ShaderMode shaderMode) {
    return std::format("Pipeline_{}_{}_{}",
                       static_cast<int>(type),
                       static_cast<int>(blendMode),
                       static_cast<int>(shaderMode));
}

std::string PipeLineManager::MakeRootSignatureKey(PipelineType type, ShaderMode shaderMode) {
    return std::format("RootSignature_{}_{}",
                       static_cast<int>(type),
                       static_cast<int>(shaderMode));
}

D3D12_STATIC_SAMPLER_DESC PipeLineManager::CreateCommonSamplerDesc() {
    D3D12_STATIC_SAMPLER_DESC desc{};
    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    desc.MaxLOD = D3D12_FLOAT32_MAX;
    desc.ShaderRegister = 0;
    desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    return desc;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateCommonRootSignature(bool hasCBV) {
    D3D12_DESCRIPTOR_RANGE range{};
    range.BaseShaderRegister = 0;
    range.NumDescriptors = 1;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[2]{};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[0].DescriptorTable.pDescriptorRanges = &range;
    params[0].DescriptorTable.NumDescriptorRanges = 1;

    UINT paramCount = 1;
    if (hasCBV) {
        params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        params[1].Descriptor.ShaderRegister = 0;
        paramCount = 2;
    }

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    desc.NumParameters = paramCount;
    desc.pParameters = params;

    auto sampler = CreateCommonSamplerDesc();
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &sampler;

    Microsoft::WRL::ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char *>(errBlob->GetBufferPointer()));
        assert(false);
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
    assert(SUCCEEDED(hr));
    return rootSig;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateFullScreenPostEffectPipeline(const std::wstring &psPath, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    IDxcBlob *vs = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    IDxcBlob *ps = dxCommon_->CompileShader(psPath.c_str(), L"ps_6_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSignature.Get();
    desc.InputLayout = {nullptr, 0};
    desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    desc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.DepthStencilState.DepthEnable = FALSE;
    desc.DepthStencilState.StencilEnable = FALSE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.SampleDesc.Count = 1;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState));
    assert(SUCCEEDED(hr));
    return pipelineState;
}

D3D12_DEPTH_STENCIL_DESC PipeLineManager::SettingDepthStencilDesc(bool depth) {
    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = depth;
    depthStencilDesc.StencilEnable = depth;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    return depthStencilDesc;
}

void PipeLineManager::CreateAllPipelines() {
    // 各種パイプラインの作成
    CreateStandardPipelines();
    CreateParticlePipelines();
    CreateSpritePipelines();
    CreateRenderPipelines();
    CreateSkinningPipelines();
    CreateLine3dPipelines();
    CreateSkyboxPipelines();
}

// 標準パイプラインの作成
void PipeLineManager::CreateStandardPipelines() {
    // ルートシグネチャを作成し、マップに格納
    auto rootSignature = CreateRootSignature();
    rootSignatures_[MakeRootSignatureKey(PipelineType::kStandard, ShaderMode::kNone)] = rootSignature;

    // 各ブレンドモード用のパイプラインを作成し、マップに格納
    for (int i = 0; i <= static_cast<int>(BlendMode::kScreen); i++) {
        BlendMode blendMode = static_cast<BlendMode>(i);
        auto pipeline = CreateGraphicsPipeLine(rootSignature, blendMode);
        pipelines_[MakePipelineKey(PipelineType::kStandard, blendMode, ShaderMode::kNone)] = pipeline;
    }
}

// スプライトパイプラインの作成
void PipeLineManager::CreateSpritePipelines() {
    // ルートシグネチャを作成し、マップに格納
    auto rootSignature = CreateSpriteRootSignature();
    rootSignatures_[MakeRootSignatureKey(PipelineType::kSprite, ShaderMode::kNone)] = rootSignature;

    // 各ブレンドモード用のパイプラインを作成し、マップに格納
    for (int i = 0; i <= static_cast<int>(BlendMode::kScreen); i++) {
        BlendMode blendMode = static_cast<BlendMode>(i);
        auto pipeline = CreateSpriteGraphicsPipeLine(rootSignature, blendMode);
        pipelines_[MakePipelineKey(PipelineType::kSprite, blendMode, ShaderMode::kNone)] = pipeline;
    }
}

// レンダーパイプラインの作成
void PipeLineManager::CreateRenderPipelines() {
    // 各シェーダーモード用のルートシグネチャとパイプラインを作成
    for (int i = 0; i <= static_cast<int>(ShaderMode::kCinematic); i++) {
        ShaderMode shaderMode = static_cast<ShaderMode>(i);

        // ルートシグネチャを作成し、マップに格納
        auto rootSignature = CreateRenderRootSignature(shaderMode);
        rootSignatures_[MakeRootSignatureKey(PipelineType::kRender, shaderMode)] = rootSignature;

        // パイプラインを作成し、マップに格納
        auto pipeline = CreateRenderGraphicsPipeLine(rootSignature, shaderMode);
        pipelines_[MakePipelineKey(PipelineType::kRender, BlendMode::kNormal, shaderMode)] = pipeline;
    }
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;
    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // DescriptorRange
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};

    // 通常の2Dテクスチャ t0
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].BaseShaderRegister = 0; // t0
    descriptorRange[0].RegisterSpace = 0;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE skyBoxDescriptorRange[1] = {};

    // 通常の2Dテクスチャ t0
    skyBoxDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    skyBoxDescriptorRange[0].NumDescriptors = 1;
    skyBoxDescriptorRange[0].BaseShaderRegister = 1; // t0
    skyBoxDescriptorRange[0].RegisterSpace = 0;
    skyBoxDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // RootParameter作成。複数設定できるので配列。
    D3D12_ROOT_PARAMETER rootParameters[8] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // VertexShaderで使う
    rootParameters[0].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVをつかう
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;               // PixelShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVを使う
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[3].Descriptor.ShaderRegister = 1;                                   // レジスタ番号1とバインド
    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVを使う
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[4].Descriptor.ShaderRegister = 2;                                   // レジスタ番号1とバインド
    rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVを使う
    rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[5].Descriptor.ShaderRegister = 3;                                   // レジスタ番号1とバインド
    rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVを使う
    rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[6].Descriptor.ShaderRegister = 4;                                   // レジスタ番号1とバインド
    descriptionRootSignature.pParameters = rootParameters;                             // ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters);                 // 配列の長さ
    rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[7].DescriptorTable.pDescriptorRanges = skyBoxDescriptorRange;
    rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(skyBoxDescriptorRange);



    // Smplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;   // バイリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0～1の範囲外をリピート
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;     // 比較しない
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;                       // ありったけのMipmapを使う
    staticSamplers[0].ShaderRegister = 0;                               // レジスタ番号0を使う
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // シリアライズしてパイナリする
    ID3DBlob *signatureBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                                     signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));
    return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateGraphicsPipeLine(
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
    HRESULT hr;

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStageの設定
    D3D12_BLEND_DESC blendDesc{};
    // すべての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;

    switch (blendMode) {
    case BlendMode::kNone:
        // ブレンドを無効化する
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        break;
    case BlendMode::kNormal:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        break;
    case BlendMode::kAdd:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    case BlendMode::kSubtract:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    case BlendMode::kMultiply:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
        break;
    case BlendMode::kScreen:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    default:
        break;
    }

    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    // ResiterzerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    // 裏面（時計回り）を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    // Shaderをコンパイルする
    IDxcBlob *vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Object/Object3d.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    IDxcBlob *pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Object/Object3d.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = true;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;        // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = blendDesc;                  // BlendState
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;        // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // DepthStencilの設定
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 利用するトロポジ（形状）のタイプ、三角形
    graphicsPipelineStateDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // どのように画面に色を打ち込むかの設定（気にしなくていい）
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    // 実際に生成
    hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
                                                             IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));
    return graphicsPipelineState;
}

void PipeLineManager::CreateParticlePipelines() {
    // ルートシグネチャを作成し、マップに格納
    auto rootSignature = CreateParticleRootSignature();
    rootSignatures_[MakeRootSignatureKey(PipelineType::kParticle, ShaderMode::kNone)] = rootSignature;

    // 各ブレンドモード用のパイプラインを作成し、マップに格納
    for (int i = 0; i <= static_cast<int>(BlendMode::kScreen); i++) {
        BlendMode blendMode = static_cast<BlendMode>(i);
        auto pipeline = CreateParticleGraphicsPipeLine(rootSignature, blendMode);
        pipelines_[MakePipelineKey(PipelineType::kParticle, blendMode, ShaderMode::kNone)] = pipeline;
    }
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateParticleRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // DescriptorRange
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;                                                   // 0から始まる
    descriptorRange[0].NumDescriptors = 1;                                                       // 数は1つ
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                              // SRVを使う
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

    // descriptorRangeForInstancing
    D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
    descriptorRangeForInstancing[0].BaseShaderRegister = 0;                                                   // 0から始まる
    descriptorRangeForInstancing[0].NumDescriptors = 1;                                                       // 数は1つ
    descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                              // SRVを使う
    descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

    // RootParameter作成。複数設定できるので配列。
    D3D12_ROOT_PARAMETER rootParameters[3] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
    rootParameters[0].Descriptor.ShaderRegister = 0;                    // レジスタ番号0とバインド

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;                   // CBVをつかう
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;                            // VertexShaderで使う
    rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;             // Tableの中身の配列を指定
    rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing); // Tableで利用する数

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

    descriptionRootSignature.pParameters = rootParameters;             // ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ

    // Smplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;   // バイリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0～1の範囲外をリピート
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;     // 比較しない
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;                       // ありったけのMipmapを使う
    staticSamplers[0].ShaderRegister = 0;                               // レジスタ番号0を使う
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // シリアライズしてパイナリする
    ID3DBlob *signatureBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    // パイナリを元に生成
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                                     signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));
    return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateParticleGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
    HRESULT hr;

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStageの設定
    D3D12_BLEND_DESC blendDesc{};
    // すべての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    // BlendMode = Add
    switch (blendMode) {
    case BlendMode::kNone:
        // ブレンドを無効化する
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        break;
    case BlendMode::kNormal:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        break;
    case BlendMode::kAdd:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    case BlendMode::kSubtract:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    case BlendMode::kMultiply:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
        break;
    case BlendMode::kScreen:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    default:
        break;
    }

    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;

    // ResiterzerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    // 裏面（時計回り）を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    // Shaderをコンパイルする
    IDxcBlob *vertexShaderBlob = dxCommon_->CompileShader(L"./Resources/shaders/Particle/Particle.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    IDxcBlob *pixelShaderBlob = dxCommon_->CompileShader(L"./Resources/shaders/Particle/Particle.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = true;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;        // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = blendDesc;                  // BlendState
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;        // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // DepthStencilの設定
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 利用するトロポジ（形状）のタイプ、三角形
    graphicsPipelineStateDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // どのように画面に色を打ち込むかの設定（気にしなくていい）
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    // 実際に生成
    hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
                                                             IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));

    return graphicsPipelineState;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateSpriteRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

    HRESULT hr;
    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // DescriptorRange
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;                                                   // 0から始まる
    descriptorRange[0].NumDescriptors = 1;                                                       // 数は1つ
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                              // SRVを使う
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

    // RootParameter作成。複数設定できるので配列。
    D3D12_ROOT_PARAMETER rootParameters[3] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[0].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVをつかう
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;               // VertexShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
    descriptionRootSignature.pParameters = rootParameters;                             // ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters);                 // 配列の長さ

    // Smplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;   // バイリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0～1の範囲外をリピート
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;     // 比較しない
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;                       // ありったけのMipmapを使う
    staticSamplers[0].ShaderRegister = 0;                               // レジスタ番号0を使う
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // シリアライズしてパイナリする
    ID3DBlob *signatureBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                                     signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));
    return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateSpriteGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
    HRESULT hr;

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStageの設定
    D3D12_BLEND_DESC blendDesc{};
    // すべての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;

    switch (blendMode) {
    case BlendMode::kNone:
        // ブレンドを無効化する
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        break;
    case BlendMode::kNormal:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        break;
    case BlendMode::kAdd:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    case BlendMode::kSubtract:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    case BlendMode::kMultiply:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
        break;
    case BlendMode::kScreen:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        break;
    default:
        break;
    }

    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    // ResiterzerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    // 裏面（時計回り）を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    // Shaderをコンパイルする
    IDxcBlob *vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Sprite/Sprite.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    IDxcBlob *pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Sprite/Sprite.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = true;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;        // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = blendDesc;                  // BlendState
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;        // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // DepthStencilの設定
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 利用するトロポジ（形状）のタイプ、三角形
    graphicsPipelineStateDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // どのように画面に色を打ち込むかの設定（気にしなくていい）
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    // 実際に生成
    hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
                                                             IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));
    return graphicsPipelineState;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateRenderRootSignature(ShaderMode shaderMode) {
    // シェーダーモードに応じて適切なルートシグネチャ作成メソッドを呼び出す
    switch (shaderMode) {
    case ShaderMode::kNone:
        return CreateBaseRootSignature();
    case ShaderMode::kGray:
        return CreateGrayRootSignature();
    case ShaderMode::kVigneet:
        return CreateVignetteRootSignature();
    case ShaderMode::kSmooth:
        return CreateSmoothRootSignature();
    case ShaderMode::kGauss:
        return CreateGaussRootSignature();
    case ShaderMode::kOutLine:
        return CreateOutLineRootSignature();
    case ShaderMode::kDepth:
        return CreateDepthRootSignature();
    case ShaderMode::kBlur:
        return CreateBlurRootSignature();
    case ShaderMode::kCinematic:
        return CreateCinematicRootSignature();
    default:
        return CreateBaseRootSignature();
    }
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateRenderGraphicsPipeLine(
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, ShaderMode shaderMode) {

    // シェーダーモードに応じて適切なパイプライン作成メソッドを呼び出す
    switch (shaderMode) {
    case ShaderMode::kNone:
        return CreateNoneGraphicsPipeLine(rootSignature);
    case ShaderMode::kGray:
        return CreateGrayGraphicsPipeLine(rootSignature);
    case ShaderMode::kVigneet:
        return CreateVigneetGraphicsPipeLine(rootSignature);
    case ShaderMode::kSmooth:
        return CreateSmoothGraphicsPipeLine(rootSignature);
    case ShaderMode::kGauss:
        return CreateGaussGraphicsPipeLine(rootSignature);
    case ShaderMode::kOutLine:
        return CreateOutLineGraphicsPipeLine(rootSignature);
    case ShaderMode::kDepth:
        return CreateDepthGraphicsPipeLine(rootSignature);
    case ShaderMode::kBlur:
        return CreateBlurGraphicsPipeLine(rootSignature);
    case ShaderMode::kCinematic:
        return CreateCinematicGraphicsPipeLine(rootSignature);
    default:
        return CreateNoneGraphicsPipeLine(rootSignature);
    }
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateSkinningRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;
    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // DescriptorRange - テクスチャ用（t0）
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};

    // 通常の2Dテクスチャ t0
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].BaseShaderRegister = 0; // t0
    descriptorRange[0].RegisterSpace = 0;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE skyBoxDescriptorRange[1] = {};

    // SkyBox用のテクスチャt1
    skyBoxDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    skyBoxDescriptorRange[0].NumDescriptors = 1;
    skyBoxDescriptorRange[0].BaseShaderRegister = 1; // t1
    skyBoxDescriptorRange[0].RegisterSpace = 0;
    skyBoxDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // descriptorRangeForInstancing（スキニング用）
    D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
    descriptorRangeForInstancing[0].BaseShaderRegister = 0;                                                   // t0から始まる（VertexShaderのStructuredBuffer用）
    descriptorRangeForInstancing[0].NumDescriptors = 1;                                                       // 数は1つ
    descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                              // SRVを使う
    descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

    // RootParameter作成。複数設定できるので配列。
    D3D12_ROOT_PARAMETER rootParameters[9] = {};

    // rootParameters[0]: Material (b0) - PixelShader
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    // rootParameters[1]: TransformationMatrix (b0) - VertexShader
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;

    // rootParameters[2]: テクスチャ用DescriptorTable (t0, t1) - PixelShader
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // 2つのテクスチャ

    // rootParameters[3]: DirectionalLight (b1) - PixelShader
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;

    // rootParameters[4]: Camera (b2) - PixelShader
    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[4].Descriptor.ShaderRegister = 2;

    // rootParameters[5]: PointLight (b3) - PixelShader
    rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[5].Descriptor.ShaderRegister = 3;

    // rootParameters[6]: SpotLight (b4) - PixelShader
    rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[6].Descriptor.ShaderRegister = 4;

    rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[7].DescriptorTable.pDescriptorRanges = skyBoxDescriptorRange;
    rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(skyBoxDescriptorRange);

    // rootParameters[8]: スキニング用StructuredBuffer (t0) - VertexShader
    rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[8].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
    rootParameters[8].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    // Samplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // シリアライズしてバイナリにする
    ID3DBlob *signatureBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                                     signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));
    return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateSkinningGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
    HRESULT hr;

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[5] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[2].SemanticName = "NORMAL";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[3].SemanticName = "WEIGHT";
    inputElementDescs[3].SemanticIndex = 0;
    inputElementDescs[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[3].InputSlot = 1; // 一番目のslotのVBVのことだと伝える
    inputElementDescs[4].SemanticName = "INDEX";
    inputElementDescs[4].SemanticIndex = 0;
    inputElementDescs[4].Format = DXGI_FORMAT_R32G32B32A32_SINT;
    inputElementDescs[4].InputSlot = 1;
    inputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStageの設定
    D3D12_BLEND_DESC blendDesc{};
    // すべての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    // ResiterzerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    // 裏面（時計回り）を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    // Shaderをコンパイルする
    IDxcBlob *vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Skinning/Skinning.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    IDxcBlob *pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Object/Object3d.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = true;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;        // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = blendDesc;                  // BlendState
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;        // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // DepthStencilの設定
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 利用するトロポジ（形状）のタイプ、三角形
    graphicsPipelineStateDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // どのように画面に色を打ち込むかの設定（気にしなくていい）
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    // 実際に生成
    hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
                                                             IID_PPV_ARGS(&graphicsPipelineState));

    assert(SUCCEEDED(hr));
    return graphicsPipelineState;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateLine3dRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;
    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // DescriptorRange
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;                                                   // 0から始まる
    descriptorRange[0].NumDescriptors = 1;                                                       // 数は1つ
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                              // SRVを使う
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

    // RootParameter作成。複数設定できるので配列。
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // CBVを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
    rootParameters[0].Descriptor.ShaderRegister = 0;                     // レジスタ番号0とバインド
    descriptionRootSignature.pParameters = rootParameters;               // ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters);   // 配列の長さ

    // Smplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;   // バイリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0～1の範囲外をリピート
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;     // 比較しない
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;                       // ありったけのMipmapを使う
    staticSamplers[0].ShaderRegister = 0;                               // レジスタ番号0を使う
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // シリアライズしてパイナリする
    ID3DBlob *signatureBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
        assert(false);
    }
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                                     signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));
    return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateLine3dGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
    HRESULT hr;

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "COLOR";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStageの設定
    D3D12_BLEND_DESC blendDesc{};
    // すべての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    // ResiterzerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    // 裏面（時計回り）を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    // Shaderをコンパイルする
    IDxcBlob *vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Line/Line3d.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    IDxcBlob *pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Line/Line3d.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = true;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;        // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = blendDesc;                  // BlendState
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;        // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // DepthStencilの設定
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 利用するトロポジ（形状）のタイプ、三角形
    graphicsPipelineStateDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    // どのように画面に色を打ち込むかの設定（気にしなくていい）
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    // 実際に生成
    hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
                                                             IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));
    return graphicsPipelineState;
}

void PipeLineManager::CreateSkyboxPipelines() {
    // ルートシグネチャを作成し、マップに格納
    auto rootSignature = CreateSkyboxRootSignature();
    rootSignatures_[MakeRootSignatureKey(PipelineType::kSkybox, ShaderMode::kNone)] = rootSignature;

    // パイプラインを作成し、マップに格納
    auto pipeline = CreateSkyboxGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(PipelineType::kSkybox, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateSkyboxRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // DescriptorRange
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // RootParameter作成
    D3D12_ROOT_PARAMETER rootParameters[3] = {};

    // 0番目：VertexShader用 CBV b0
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    // 1番目：PixelShader用 CBV b0
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 1;

    // 2番目：PixelShader用 SRV テーブル t0
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

    // 【修正】NumParametersを設定
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    // Samplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // シリアライズしてバイナリ化
    ID3DBlob *signatureBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                                     signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));

    // リソースを解放
    if (signatureBlob)
        signatureBlob->Release();
    if (errorBlob)
        errorBlob->Release();

    return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateSkyboxGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
    HRESULT hr;

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].InputSlot = 0;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[0].InstanceDataStepRate = 0;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // BlendStateの設定
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    // RasterizerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // Shaderをコンパイル
    IDxcBlob *vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Skybox/Skybox.VS.hlsl", L"vs_6_0");
    assert(vertexShaderBlob != nullptr);

    IDxcBlob *pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Skybox/Skybox.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // スカイボックスは深度書き込みしない
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // パイプラインステート作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // 【修正】スカイボックスは三角形で描画
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // パイプラインステート作成
    hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
                                                             IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));

    return graphicsPipelineState;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateBaseRootSignature() {
    return CreateCommonRootSignature(false);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateGrayRootSignature() {
    return CreateBaseRootSignature();
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateVignetteRootSignature() {
    return CreateCommonRootSignature(true);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateSmoothRootSignature() {
    return CreateCommonRootSignature(true);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateGaussRootSignature() {
    return CreateCommonRootSignature(true);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateOutLineRootSignature() {
    return CreateBaseRootSignature();
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateDepthRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    // RootSignatureの設定
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // DescriptorRangeの設定（SRV用: gTexture, gDepthTexture）
    D3D12_DESCRIPTOR_RANGE descriptorRanges[2] = {};
    descriptorRanges[0].BaseShaderRegister = 0; // gTexture用 (t0)
    descriptorRanges[0].NumDescriptors = 1;     // 1つのSRV
    descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    descriptorRanges[1].BaseShaderRegister = 1; // gDepthTexture用 (t1)
    descriptorRanges[1].NumDescriptors = 1;     // 1つのSRV
    descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // RootParameterの設定
    D3D12_ROOT_PARAMETER rootParameters[3] = {};

    // DescriptorTable (SRV用)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使用
    rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

    // ConstantBuffer用 (gMaterial)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].Descriptor.ShaderRegister = 0; // b0

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使用
    rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRanges[1];
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    // Samplerの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};

    // サンプラー: gSampler (s0)
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0; // s0
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // サンプラー: gSamplerPoint (s1)
    staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[1].ShaderRegister = 1; // s1
    staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // RootSignatureのシリアライズと作成
    ID3DBlob *signatureBlob = nullptr;
    ID3DBlob *errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
        }
        assert(false);
    }
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));

    return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateBlurRootSignature() {
    return CreateCommonRootSignature(true);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateCinematicRootSignature() {
    return CreateCommonRootSignature(true);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateNoneGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(false);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/CopyImage.PS.hlsl", rootSignature);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateGrayGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(false);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/GrayScale.PS.hlsl", rootSignature);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateVigneetGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(false);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/Vignette.PS.hlsl", rootSignature);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateSmoothGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(false);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/BoxFilter.PS.hlsl", rootSignature);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateGaussGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(false);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/GaussianFilter.PS.hlsl", rootSignature);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateOutLineGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(false);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/LuminanceBasedOutline.PS.hlsl", rootSignature);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateDepthGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(true);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/DepthBasedOutline.PS.hlsl", rootSignature);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateBlurGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(false);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/RadialBlur.PS.hlsl", rootSignature);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateCinematicGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    SettingDepthStencilDesc(false);
    return CreateFullScreenPostEffectPipeline(L"./resources/shaders/OffScreen/Cinematic.PS.hlsl", rootSignature);
}

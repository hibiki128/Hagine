#include "PipeLineManager.h"
#include "Log/Logger.h"
#include <d3dx12.h>

void PipeLineManager::Initialize(DirectXCommon *dxCommon) {
    dxCommon_ = dxCommon;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    D3D12_ROOT_PARAMETER rootParameters[7] = {};
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode_) {

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

    switch (blendMode_) {
    case BlendMode::kNone:
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateParticleRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateParticleGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode_) {

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
    switch (blendMode_) {
    case BlendMode::kNone:
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateSpriteRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateSpriteGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, BlendMode blendMode_) {

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

    switch (blendMode_) {
    case BlendMode::kNone:
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateRenderRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, ShaderMode shaderMode_) {
    switch (shaderMode_) {
    case ShaderMode::kNone:
    case ShaderMode::kGray:
        return CreateBaseRootSignature(rootSignature);
        break;
    case ShaderMode::kVigneet:
        return CreateVignetteRootSignature(rootSignature);
        break;
    case ShaderMode::kSmooth:
        return CreateSmoothRootSignature(rootSignature);
        break;
    case ShaderMode::kGauss:
        return CreateGaussRootSignature(rootSignature);
        break;
    case ShaderMode::kOutLine:
        return CreateBaseRootSignature(rootSignature);
        break;
    case ShaderMode::kDepth:
        return CreateDepthRootSignature(rootSignature);
        break;
    case ShaderMode::kBlur:
        return CreateBlurRootSignature(rootSignature);
        break;
    case ShaderMode::kCinematic:
        return CreateCinematicRootSignature(rootSignature);
    default:
        return 0;
        break;
    }

    return 0;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateRenderGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, ShaderMode shaderMode_) {
    switch (shaderMode_) {
    case ShaderMode::kNone:
        return CreateNoneGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    case ShaderMode::kGray:
        return CreateGrayGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    case ShaderMode::kVigneet:
        return CreateVigneetGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    case ShaderMode::kSmooth:
        return CreateSmoothGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    case ShaderMode::kGauss:
        return CreateGaussGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    case ShaderMode::kOutLine:
        return CreateOutLineGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    case ShaderMode::kDepth:
        return CreateDepthGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    case ShaderMode::kBlur:
        return CreateBlurGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    case ShaderMode::kCinematic:
        return CreateCinematicGraphicsPipeLine(graphicsPipelineState, rootSignature);
        break;
    default:
        return 0;
        break;
    }

    return 0;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateSkinningRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[7].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
    rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

    descriptionRootSignature.pParameters = rootParameters;             // ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateSkinningGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    IDxcBlob *vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/Object/SkinningObject3d.VS.hlsl", L"vs_6_0");
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateLine3dRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateLine3dGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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

void PipeLineManager::DrawCommonSetting(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    // RootSignatureを設定。PSOに設定しているけど別途設定が必要
    dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get()); // PSOを設定
    dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
    // 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいい
    dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateBaseRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateVignetteRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVをつかう
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateSmoothRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVをつかう
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateGaussRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVをつかう
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateDepthRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateBlurRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVをつかう
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> PipeLineManager::CreateCinematicRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
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
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                   // CBVをつかう
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;                                   // レジスタ番号0とバインド
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateNoneGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {

    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/CopyImage.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateGrayGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {

    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/Grayscale.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateVigneetGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {

    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/Vignette.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateSmoothGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {

    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/BoxFilter.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateGaussGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {

    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/GaussianFilter.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateOutLineGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {

    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/LuminanceBasedOutline.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateDepthGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/DepthBasedOutline.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.StencilEnable = true;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateBlurGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {

    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/RadialBlur.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipeLineManager::CreateCinematicGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    HRESULT hr;

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC layout{};
    layout.pInputElementDescs = nullptr;
    layout.NumElements = 0;

    IDxcBlob *vertexShaderBlob{};
    IDxcBlob *pixelShaderBlob{};
    vertexShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/FullScreen.VS.hlsl", L"vs_6_0");
    pixelShaderBlob = dxCommon_->CompileShader(L"./resources/shaders/OffScreen/Cinematic.PS.hlsl", L"ps_6_0");
    assert(pixelShaderBlob != nullptr);

    ///=========DepthStencilStateの設定==========
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を有効化する
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    // 書き込みします
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    // 比較関数はLessEqual。つまり、近ければ描画される
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ///==========================================

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
    graphicsPipelineStateDesc.InputLayout = layout;                 // InputLayout
    graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                    vertexShaderBlob->GetBufferSize()}; // vertexShader
    graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                    pixelShaderBlob->GetBufferSize()}; // PixelShader
    graphicsPipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ;                                                                                   // BlendState
    graphicsPipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // RasterizerState
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

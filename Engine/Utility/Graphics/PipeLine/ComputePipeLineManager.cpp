#include "ComputePipeLineManager.h"
#include <Debug/Log/Logger.h>

ComputePipeLineManager *ComputePipeLineManager::instance = nullptr;

ComputePipeLineManager *ComputePipeLineManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ComputePipeLineManager;
    }
    return instance;
}

void ComputePipeLineManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void ComputePipeLineManager::Initialize(DirectXCommon *dxCommon) {
    dxCommon_ = dxCommon;

    CreateAllPipelines();
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipeLineManager::GetPipeline(ComputePipelineType type, BlendMode blendMode, ShaderMode shaderMode) {
    // キーを生成して対応するパイプラインを取得
    std::string key = MakePipelineKey(type, blendMode, shaderMode);

    // 対応するパイプラインが存在するか確認
    if (pipelines_.find(key) == pipelines_.end()) {
        // パイプラインが見つからない場合は警告を出して、デフォルトを返す
        assert(false && "指定されたパイプラインが存在しません");

        // デフォルトのパイプラインを返す (ここではStandard/Normal/Noneを想定)
        return pipelines_[MakePipelineKey(ComputePipelineType::kSkinning, BlendMode::kNormal, ShaderMode::kNone)];
    }

    return pipelines_[key];
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::GetRootSignature(ComputePipelineType type, ShaderMode shaderMode) {
    // キーを生成して対応するルートシグネチャを取得
    std::string key = MakeRootSignatureKey(type, shaderMode);

    // 対応するルートシグネチャが存在するか確認
    if (rootSignatures_.find(key) == rootSignatures_.end()) {
        // ルートシグネチャが見つからない場合は警告を出して、デフォルトを返す
        assert(false && "指定されたルートシグネチャが存在しません");

        // デフォルトのルートシグネチャを返す
        return rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kSkinning, ShaderMode::kNone)];
    }

    return rootSignatures_[key];
}

void ComputePipeLineManager::DrawCommonSetting(ComputePipelineType type, BlendMode blendMode, ShaderMode shaderMode) {
    // 指定されたタイプのパイプラインとルートシグネチャを取得
    auto pipeline = GetPipeline(type, blendMode, shaderMode);
    auto rootSignature = GetRootSignature(type, shaderMode);

    // グラフィックスコマンドリストにパイプラインとルートシグネチャを設定
    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    commandList->SetPipelineState(pipeline.Get());
    commandList->SetComputeRootSignature(rootSignature.Get());
}

void ComputePipeLineManager::CreateSkinningPipelines() {
    // ルートシグネチャを作成し、マップに格納
    auto rootSignature = CreateSkinningRootSignature();
    rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kSkinning, ShaderMode::kNone)] = rootSignature;

    // パイプラインを作成し、マップに格納
    auto pipeline = CreateSkinningGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(ComputePipelineType::kSkinning, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

void ComputePipeLineManager::CreateAllPipelines() {
    CreateSkinningPipelines();
}

std::string ComputePipeLineManager::MakePipelineKey(ComputePipelineType type, BlendMode blendMode, ShaderMode shaderMode) {
    return std::format("Pipeline_{}_{}_{}",
                       static_cast<int>(type),
                       static_cast<int>(blendMode),
                       static_cast<int>(shaderMode));
}

std::string ComputePipeLineManager::MakeRootSignatureKey(ComputePipelineType type, ShaderMode shaderMode) {
    return std::format("RootSignature_{}_{}",
                       static_cast<int>(type),
                       static_cast<int>(shaderMode));
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::CreateSkinningRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    // t0
    D3D12_DESCRIPTOR_RANGE srvRange0[1] = {};
    srvRange0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange0[0].NumDescriptors = 1;
    srvRange0[0].BaseShaderRegister = 0;
    srvRange0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // t1
    D3D12_DESCRIPTOR_RANGE srvRange1[1] = {};
    srvRange1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange1[0].NumDescriptors = 1;
    srvRange1[0].BaseShaderRegister = 1;
    srvRange1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // t2
    D3D12_DESCRIPTOR_RANGE srvRange2[1] = {};
    srvRange2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange2[0].NumDescriptors = 1;
    srvRange2[0].BaseShaderRegister = 2;
    srvRange2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // u0
    D3D12_DESCRIPTOR_RANGE uavRange[1] = {};
    uavRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange[0].NumDescriptors = 1;
    uavRange[0].BaseShaderRegister = 0;
    uavRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // RootParameter作成。複数設定できるので配列。
    D3D12_ROOT_PARAMETER rootParameters[5] = {};

    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(srvRange0);
    rootParameters[0].DescriptorTable.pDescriptorRanges = srvRange0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(srvRange1);
    rootParameters[1].DescriptorTable.pDescriptorRanges = srvRange1;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(srvRange2);
    rootParameters[2].DescriptorTable.pDescriptorRanges = srvRange2;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(uavRange);
    rootParameters[3].DescriptorTable.pDescriptorRanges = uavRange;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[4].Descriptor.ShaderRegister = 0;
    rootParameters[4].Descriptor.RegisterSpace = 0;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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
   
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipeLineManager::CreateSkinningGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
    
    IDxcBlob *computerShaderBlob = nullptr;
    computerShaderBlob = dxCommon_->CompileShader(L"./Resources/shaders/Skinning/Skinning.CS.hlsl", L"cs_6_0");
    assert(computerShaderBlob != nullptr);

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
    computePipelineStateDesc.CS = {
        .pShaderBytecode = computerShaderBlob->GetBufferPointer(),
        .BytecodeLength = computerShaderBlob->GetBufferSize(),
    };
    computePipelineStateDesc.pRootSignature = rootSignature.Get();
    HRESULT hr = dxCommon_->GetDevice()->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));

    assert(SUCCEEDED(hr));
    return graphicsPipelineState;
}

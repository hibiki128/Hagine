#include "ComputePipeLineManager.h"
#include <Debug/Log/Logger.h>

namespace Hagine {
void ComputePipeLineManager::Finalize() {
    pipelines_.clear();
    rootSignatures_.clear();
}

void ComputePipeLineManager::Initialize(DirectXCommon *dxCommon) {
    dxCommon_ = dxCommon;

    CreateAllPipelines();
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipeLineManager::GetPipeline(ComputePipelineType type, BlendMode blendMode, ShaderMode shaderMode) {
    std::string key = MakePipelineKey(type, blendMode, shaderMode);

    if (pipelines_.find(key) == pipelines_.end()) {
        assert(false && "指定されたパイプラインが存在しません");
        return pipelines_[MakePipelineKey(ComputePipelineType::kSkinning, BlendMode::kNormal, ShaderMode::kNone)];
    }

    return pipelines_[key];
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::GetRootSignature(ComputePipelineType type, ShaderMode shaderMode) {
    std::string key = MakeRootSignatureKey(type, shaderMode);

    if (rootSignatures_.find(key) == rootSignatures_.end()) {
        assert(false && "指定されたルートシグネチャが存在しません");
        return rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kSkinning, ShaderMode::kNone)];
    }

    return rootSignatures_[key];
}

void ComputePipeLineManager::DrawCommonSetting(ComputePipelineType type, BlendMode blendMode, ShaderMode shaderMode, ID3D12GraphicsCommandList *cmdList) {
    auto pipeline = GetPipeline(type, blendMode, shaderMode);
    auto rootSignature = GetRootSignature(type, shaderMode);

    ID3D12GraphicsCommandList *commandList = cmdList ? cmdList : dxCommon_->GetCommandList().Get();
    commandList->SetPipelineState(pipeline.Get());
    commandList->SetComputeRootSignature(rootSignature.Get());
}

void ComputePipeLineManager::CreateSkinningPipelines() {
    auto rootSignature = CreateSkinningRootSignature();
    rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kSkinning, ShaderMode::kNone)] = rootSignature;

    auto pipeline = CreateSkinningGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(ComputePipelineType::kSkinning, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

void ComputePipeLineManager::CreateInitParticlePipelines() {
    auto rootSignature = CreateInitParticleRootSignature();
    rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kInitParticle, ShaderMode::kNone)] = rootSignature;

    auto pipeline = CreateInitParticleGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(ComputePipelineType::kInitParticle, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

void ComputePipeLineManager::CreateEmitterPipelines() {
    auto rootSignature = CreateEmitterRootSignature();
    rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kEmitter, ShaderMode::kNone)] = rootSignature;

    auto pipeline = CreateEmitterGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(ComputePipelineType::kEmitter, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

void ComputePipeLineManager::CreateUpdateEmitterPipelines() {
    auto rootSignature = CreateUpdateEmitterRootSignature();
    rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kUpdateEmitter, ShaderMode::kNone)] = rootSignature;

    auto pipeline = CreateUpdateEmitterGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(ComputePipelineType::kUpdateEmitter, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

void ComputePipeLineManager::CreateCountPipelines() {
    auto rootSignature = CreateCountRootSignature();
    rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kCount, ShaderMode::kNone)] = rootSignature;

    auto pipeline = CreateCountGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(ComputePipelineType::kCount, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

void ComputePipeLineManager::CreateResetArgsPipelines() {
    auto rootSignature = CreateResetArgsRootSignature();
    rootSignatures_[MakeRootSignatureKey(ComputePipelineType::kResetArgs, ShaderMode::kNone)] = rootSignature;

    auto pipeline = CreateResetArgsGraphicsPipeLine(rootSignature);
    pipelines_[MakePipelineKey(ComputePipelineType::kResetArgs, BlendMode::kNormal, ShaderMode::kNone)] = pipeline;
}

void ComputePipeLineManager::CreateAllPipelines() {
    CreateSkinningPipelines();
    CreateInitParticlePipelines();
    CreateEmitterPipelines();
    CreateUpdateEmitterPipelines();
    CreateCountPipelines();
    CreateResetArgsPipelines();
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

// =============================================
// Skinning
// =============================================
Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::CreateSkinningRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    D3D12_DESCRIPTOR_RANGE srvRange0[1] = {};
    srvRange0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange0[0].NumDescriptors = 1;
    srvRange0[0].BaseShaderRegister = 0;
    srvRange0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE srvRange1[1] = {};
    srvRange1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange1[0].NumDescriptors = 1;
    srvRange1[0].BaseShaderRegister = 1;
    srvRange1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE srvRange2[1] = {};
    srvRange2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange2[0].NumDescriptors = 1;
    srvRange2[0].BaseShaderRegister = 2;
    srvRange2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange[1] = {};
    uavRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange[0].NumDescriptors = 1;
    uavRange[0].BaseShaderRegister = 0;
    uavRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

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

// =============================================
// InitParticle
// =============================================
Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::CreateInitParticleRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    D3D12_DESCRIPTOR_RANGE uavRange0[1] = {};
    uavRange0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange0[0].NumDescriptors = 1;
    uavRange0[0].BaseShaderRegister = 0;
    uavRange0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange1[1] = {};
    uavRange1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange1[0].NumDescriptors = 1;
    uavRange1[0].BaseShaderRegister = 1;
    uavRange1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange2[1] = {};
    uavRange2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange2[0].NumDescriptors = 1;
    uavRange2[0].BaseShaderRegister = 2;
    uavRange2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange3[1] = {};
    uavRange3[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange3[0].NumDescriptors = 1;
    uavRange3[0].BaseShaderRegister = 3;
    uavRange3[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[5] = {};

    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.pDescriptorRanges = uavRange0;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(uavRange0);
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.pDescriptorRanges = uavRange1;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(uavRange1);
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable.pDescriptorRanges = uavRange2;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(uavRange2);
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[3].DescriptorTable.pDescriptorRanges = uavRange3;
    rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(uavRange3);
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[4].Descriptor.ShaderRegister = 0;
    rootParameters[4].Descriptor.RegisterSpace = 0;

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumStaticSamplers = 0;
    descriptionRootSignature.pStaticSamplers = nullptr;
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipeLineManager::CreateInitParticleGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

    IDxcBlob *computerShaderBlob = nullptr;
    computerShaderBlob = dxCommon_->CompileShader(L"./Resources/shaders/Particle/CSParticle/InitParticle.CS.hlsl", L"cs_6_0");
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

// =============================================
// Emitter
// =============================================
Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::CreateEmitterRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    // SoA UAV (u0-u5: Life/DrawCore/SimCore/Trail/Rotation/Override)
    //   + フリーリスト UAV (u6-u8) = 計9本。
    D3D12_DESCRIPTOR_RANGE uavRanges[9] = {};
    for (UINT i = 0; i < 9; ++i) {
        uavRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uavRanges[i].NumDescriptors = 1;
        uavRanges[i].BaseShaderRegister = i; // u0..u8
        uavRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }
    // SRV (t0:TriangleInfo / t1:TriangleCDF / t2:EdgeInfo / t3:ParticleField)
    D3D12_DESCRIPTOR_RANGE srvRanges[4] = {};
    for (UINT i = 0; i < 4; ++i) {
        srvRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRanges[i].NumDescriptors = 1;
        srvRanges[i].BaseShaderRegister = i; // t0..t3
        srvRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    // スロット対応:
    //   [0..8]   u0..u8 (SoA6本 + フリーリスト3本)
    //   [9..12]  b0..b3 (EmitterMesh / PerFrame / Settings / FieldCB)
    //   [13..16] t0..t3 (TriangleInfo / TriangleCDF / EdgeInfo / ParticleField)
    D3D12_ROOT_PARAMETER rootParameters[17] = {};
    for (UINT i = 0; i < 9; ++i) {
        rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[i].DescriptorTable.pDescriptorRanges = &uavRanges[i];
        rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }
    for (UINT i = 0; i < 4; ++i) {
        rootParameters[9 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[9 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[9 + i].Descriptor.ShaderRegister = i; // b0..b3
        rootParameters[9 + i].Descriptor.RegisterSpace = 0;
    }
    for (UINT i = 0; i < 4; ++i) {
        rootParameters[13 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[13 + i].DescriptorTable.pDescriptorRanges = &srvRanges[i];
        rootParameters[13 + i].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[13 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumStaticSamplers = 0;
    descriptionRootSignature.pStaticSamplers = nullptr;
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipeLineManager::CreateEmitterGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

    IDxcBlob *computerShaderBlob = nullptr;
    computerShaderBlob = dxCommon_->CompileShader(L"./Resources/shaders/Particle/CSParticle/EmitParticle.CS.hlsl", L"cs_6_0");
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

// =============================================
// =============================================
// UpdateEmitter（SoA）
//
// スロット対応表:
//   [0]  u0  : gLife          (UAV) ★SoA
//   [1]  u1  : gDrawCore      (UAV) ★SoA
//   [2]  u2  : gSimCore       (UAV) ★SoA
//   [3]  u3  : gTrail         (UAV) ★SoA
//   [4]  u4  : gRotation      (UAV) ★SoA
//   [5]  u5  : gOverride      (UAV) ★SoA
//   [6]  u6  : gFreeListIndex     (UAV)
//   [7]  u7  : gFreeList          (UAV)
//   [8]  u8  : gFreeListTailIndex (UAV)
//   [9]  u9  : gAliveList         (UAV) ★生存コンパクション
//   [10] u10 : gAliveCounter      (UAV) ★生存コンパクション
//   [11] b0  : gPerFrame      (CBV)
//   [12] b1  : gSettings      (CBV)
//   [13] b2  : gFieldCB       (CBV)
//   [14] t0  : gFields        (SRV)
//   [15] t1  : gFieldsOverride(SRV)
// =============================================
Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::CreateUpdateEmitterRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    // SoA UAV (u0-u5: Life/DrawCore/SimCore/Trail/Rotation/Override)
    //   + フリーリスト UAV (u6-u8) + 生存コンパクション UAV (u9-u10) = 計11本。
    D3D12_DESCRIPTOR_RANGE uavRanges[11] = {};
    for (UINT i = 0; i < 11; ++i) {
        uavRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uavRanges[i].NumDescriptors = 1;
        uavRanges[i].BaseShaderRegister = i; // u0..u10
        uavRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }
    // SRV (t0:gFields / t1:gFieldsOverride)
    D3D12_DESCRIPTOR_RANGE srvRanges[2] = {};
    for (UINT i = 0; i < 2; ++i) {
        srvRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRanges[i].NumDescriptors = 1;
        srvRanges[i].BaseShaderRegister = i; // t0..t1
        srvRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    // スロット対応:
    //   [0..10]  u0..u10 (SoA6本 + フリーリスト3本 + 生存コンパクション2本)
    //   [11..13] b0..b2  (PerFrame / Settings / FieldCB)
    //   [14..15] t0..t1  (Fields / FieldsOverride)
    D3D12_ROOT_PARAMETER rootParameters[16] = {};
    for (UINT i = 0; i < 11; ++i) {
        rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[i].DescriptorTable.pDescriptorRanges = &uavRanges[i];
        rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }
    for (UINT i = 0; i < 3; ++i) {
        rootParameters[11 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[11 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[11 + i].Descriptor.ShaderRegister = i; // b0..b2
        rootParameters[11 + i].Descriptor.RegisterSpace = 0;
    }
    for (UINT i = 0; i < 2; ++i) {
        rootParameters[14 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[14 + i].DescriptorTable.pDescriptorRanges = &srvRanges[i];
        rootParameters[14 + i].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[14 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumStaticSamplers = 0;
    descriptionRootSignature.pStaticSamplers = nullptr;
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipeLineManager::CreateUpdateEmitterGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

    IDxcBlob *computerShaderBlob = nullptr;
    computerShaderBlob = dxCommon_->CompileShader(L"./Resources/shaders/Particle/CSParticle/UpdateParticle.CS.hlsl", L"cs_6_0");
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

// =============================================
// ResetArgs
//   生存コンパクションカウンタ(u0)を毎フレーム 0 にリセットする 1スレッドパス
//   スロット対応表:
//     [0] u0 : gAliveCounter (UAV)
// =============================================
Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::CreateResetArgsRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    D3D12_DESCRIPTOR_RANGE uavRange0[1] = {};
    uavRange0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange0[0].NumDescriptors = 1;
    uavRange0[0].BaseShaderRegister = 0;
    uavRange0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.pDescriptorRanges = uavRange0;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(uavRange0);
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumStaticSamplers = 0;
    descriptionRootSignature.pStaticSamplers = nullptr;
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipeLineManager::CreateResetArgsGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

    IDxcBlob *computerShaderBlob = nullptr;
    computerShaderBlob = dxCommon_->CompileShader(L"./Resources/shaders/Particle/CSParticle/ResetArgs.CS.hlsl", L"cs_6_0");
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

// =============================================
// Count
// =============================================
Microsoft::WRL::ComPtr<ID3D12RootSignature> ComputePipeLineManager::CreateCountRootSignature() {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    HRESULT hr;

    D3D12_DESCRIPTOR_RANGE uavRange0[1] = {};
    uavRange0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange0[0].NumDescriptors = 1;
    uavRange0[0].BaseShaderRegister = 0;
    uavRange0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange1[1] = {};
    uavRange1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange1[0].NumDescriptors = 1;
    uavRange1[0].BaseShaderRegister = 1;
    uavRange1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[3] = {};

    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.pDescriptorRanges = uavRange0;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(uavRange0);
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable.pDescriptorRanges = uavRange1;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(uavRange1);
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumStaticSamplers = 0;
    descriptionRootSignature.pStaticSamplers = nullptr;
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

Microsoft::WRL::ComPtr<ID3D12PipelineState> ComputePipeLineManager::CreateCountGraphicsPipeLine(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

    IDxcBlob *computerShaderBlob = nullptr;
    computerShaderBlob = dxCommon_->CompileShader(L"./Resources/shaders/Particle/CSParticle/CountParticle.CS.hlsl", L"cs_6_0");
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

} // namespace Hagine

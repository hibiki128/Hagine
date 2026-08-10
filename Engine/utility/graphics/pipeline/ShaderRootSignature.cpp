#include "ShaderRootSignature.h"
#include "DirectXCommon.h"
#include <algorithm>
#include <cassert>
#include <debug/log/Logger.h>

namespace Hagine {
namespace {

/// プリセットから D3D12_STATIC_SAMPLER_DESC を作る
D3D12_STATIC_SAMPLER_DESC MakeStaticSampler(ShaderRootSignature::SamplerPreset preset, UINT shaderRegister)
{
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.ShaderRegister = shaderRegister;
    sampler.RegisterSpace = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    // ポストエフェクトは全画面パスなので、可視性は絞らず全ステージから見えるようにしておく
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    switch (preset)
    {
    case ShaderRootSignature::SamplerPreset::PointClamp:
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        break;
    case ShaderRootSignature::SamplerPreset::LinearWrap:
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    case ShaderRootSignature::SamplerPreset::LinearClamp:
    default:
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        break;
    }
    return sampler;
}

} // namespace

bool ShaderRootSignature::BuildFromReflection(DirectXCommon *pDxCommon,
                                              ID3D12ShaderReflection *pReflection,
                                              const std::vector<SamplerPreset> &samplerPresets,
                                              const std::string &debugName)
{
    if (!pDxCommon || !pReflection)
    {
        Logger::Error("ShaderRootSignature: 引数が不正です (" + debugName + ")");
        return false;
    }

    D3D12_SHADER_DESC shaderDesc{};
    if (FAILED(pReflection->GetDesc(&shaderDesc)))
    {
        Logger::Error("ShaderRootSignature: シェーダー情報の取得に失敗 (" + debugName + ")");
        return false;
    }

    // ---- シェーダーが宣言しているリソースを種類ごとに集める ----
    std::vector<UINT> cbvRegisters;
    UINT maxSrvRegister = 0;
    UINT maxUavRegister = 0;
    UINT samplerCount = 0;
    bool hasSrv = false;
    bool hasUav = false;

    for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
        if (FAILED(pReflection->GetResourceBindingDesc(i, &bindDesc)))
        {
            continue;
        }

        switch (bindDesc.Type)
        {
        case D3D_SIT_CBUFFER:
            cbvRegisters.push_back(bindDesc.BindPoint);
            break;

        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS:
        case D3D_SIT_TBUFFER:
            hasSrv = true;
            maxSrvRegister = (std::max)(maxSrvRegister, bindDesc.BindPoint + bindDesc.BindCount - 1);
            break;

        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
            hasUav = true;
            maxUavRegister = (std::max)(maxUavRegister, bindDesc.BindPoint + bindDesc.BindCount - 1);
            break;

        case D3D_SIT_SAMPLER:
            ++samplerCount;
            break;

        default:
            break;
        }
    }

    // t0..tN / u0..uN を連続レンジ1本で張るので、レジスタは0から詰めて宣言する前提。
    // （飛び番にすると間のスロットも確保されるだけで、動作自体は壊れない）
    srvCount_ = hasSrv ? (maxSrvRegister + 1) : 0;
    uavCount_ = hasUav ? (maxUavRegister + 1) : 0;

    std::sort(cbvRegisters.begin(), cbvRegisters.end());

    // ---- ルートパラメータを組み立てる ----
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;
    rootParameters.reserve(cbvRegisters.size() + 2);

    // b# はルートCBVにする（デスクリプタテーブルを経由しないぶん設定が軽い）
    cbvRootIndices_.assign(cbvRegisters.empty() ? 0 : (cbvRegisters.back() + 1), UINT_MAX);
    for (UINT reg : cbvRegisters)
    {
        D3D12_ROOT_PARAMETER param{};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.Descriptor.ShaderRegister = reg;
        param.Descriptor.RegisterSpace = 0;
        cbvRootIndices_[reg] = static_cast<UINT>(rootParameters.size());
        rootParameters.push_back(param);
    }

    // デスクリプタレンジは D3D12_ROOT_PARAMETER がポインタで持つため、
    // CreateRootSignature を呼ぶまで生存させておく必要がある
    D3D12_DESCRIPTOR_RANGE srvRange{};
    D3D12_DESCRIPTOR_RANGE uavRange{};

    if (srvCount_ > 0)
    {
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = srvCount_;
        srvRange.BaseShaderRegister = 0;
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER param{};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &srvRange;
        srvTableIndex_ = static_cast<UINT>(rootParameters.size());
        rootParameters.push_back(param);
    }

    if (uavCount_ > 0)
    {
        uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uavRange.NumDescriptors = uavCount_;
        uavRange.BaseShaderRegister = 0;
        uavRange.RegisterSpace = 0;
        uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER param{};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &uavRange;
        uavTableIndex_ = static_cast<UINT>(rootParameters.size());
        rootParameters.push_back(param);
    }

    // ---- スタティックサンプラー ----
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    staticSamplers.reserve(samplerCount);
    for (UINT i = 0; i < samplerCount; ++i)
    {
        const SamplerPreset preset = (i < samplerPresets.size()) ? samplerPresets[i] : SamplerPreset::LinearClamp;
        staticSamplers.push_back(MakeStaticSampler(preset, i));
    }

    // ---- 生成 ----
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = static_cast<UINT>(rootParameters.size());
    desc.pParameters = rootParameters.empty() ? nullptr : rootParameters.data();
    desc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
    desc.pStaticSamplers = staticSamplers.empty() ? nullptr : staticSamplers.data();
    // コンピュートシェーダーでは入力アセンブラを使わないのでフラグは立てない
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            Logger::Error("ShaderRootSignature: シリアライズ失敗 (" + debugName + "): " +
                          std::string(static_cast<const char *>(errorBlob->GetBufferPointer())));
        }
        return false;
    }

    hr = pDxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                                     signatureBlob->GetBufferSize(),
                                                     IID_PPV_ARGS(&rootSignature_));
    if (FAILED(hr))
    {
        Logger::Error("ShaderRootSignature: 生成失敗 (" + debugName + ")");
        return false;
    }

    return true;
}

UINT ShaderRootSignature::GetRootParameterIndex(D3D12_DESCRIPTOR_RANGE_TYPE type, UINT shaderRegister) const
{
    switch (type)
    {
    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
        return (shaderRegister < cbvRootIndices_.size()) ? cbvRootIndices_[shaderRegister] : UINT_MAX;
    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
        return srvTableIndex_;
    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
        return uavTableIndex_;
    default:
        return UINT_MAX;
    }
}
} // namespace Hagine

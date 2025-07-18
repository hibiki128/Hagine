#include"Skinning.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct Skinned
{
    float4 position;
    float3 normal;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
StructuredBuffer<Well> gMatrixPalette : register(t0);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));

    return output;
}
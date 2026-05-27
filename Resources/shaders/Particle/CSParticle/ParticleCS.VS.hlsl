#include "../Particle.hlsli"

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

StructuredBuffer<Particle> gParticles : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    Particle particle = gParticles[instanceId];

    // --- XYZ 3軸回転行列 (Rz * Ry * Rx の順で合成) ---
    float sx, cx, sy, cy, sz, cz;
    sincos(particle.rotation.x, sx, cx);
    sincos(particle.rotation.y, sy, cy);
    sincos(particle.rotation.z, sz, cz);

    float4x4 rotXYZ = float4x4(
        cy * cz, cy * sz, -sy, 0,
        sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy, 0,
        cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy, 0,
        0, 0, 0, 1
    );

    // --- スケール → XYZ回転 → ビルボード → 平行移動 ---
    float4x4 worldMatrix;

    if (gPerView.enableVelocityStretch != 0)
    {
        // 速度方向に引き伸ばすストレッチビルボード
        float3 vel = particle.velocity;
        float3 camRight = normalize(gPerView.billboardMatrix[0].xyz);
        float3 camUp    = normalize(gPerView.billboardMatrix[1].xyz);
        float vRight = dot(vel, camRight);
        float vUp    = dot(vel, camUp);
        float2 screenVel = float2(vRight, vUp);
        float speed = length(screenVel);

        if (speed > 0.001f)
        {
            float2 stretchDir = screenVel / speed;
            float3 newUp    = normalize(stretchDir.x * camRight + stretchDir.y * camUp);
            float3 camBack  = normalize(gPerView.billboardMatrix[2].xyz);
            float3 newRight = normalize(cross(newUp, camBack));
            float stretchScale = 1.0f + speed * gPerView.velocityStretchFactor;

            worldMatrix[0] = float4(newRight * particle.scale.x, 0.0f);
            worldMatrix[1] = float4(newUp * (particle.scale.y * stretchScale), 0.0f);
            worldMatrix[2] = float4(camBack * particle.scale.z, 0.0f);
            worldMatrix[3] = float4(particle.translate, 1.0f);
        }
        else
        {
            // 速度ゼロ時は通常ビルボード
            worldMatrix = gPerView.billboardMatrix;
            worldMatrix[0] *= particle.scale.x;
            worldMatrix[1] *= particle.scale.y;
            worldMatrix[2] *= particle.scale.z;
            worldMatrix[3].xyz = particle.translate;
        }
    }
    else
    {
        // 通常ビルボード
        worldMatrix = gPerView.billboardMatrix;
        worldMatrix[0] *= particle.scale.x;
        worldMatrix[1] *= particle.scale.y;
        worldMatrix[2] *= particle.scale.z;
        worldMatrix[3].xyz = particle.translate;
    }

    output.position = mul(input.position, mul(rotXYZ, mul(worldMatrix, gPerView.viewProjection)));
    output.texcoord = input.texcoord;
    output.color = particle.color;
    return output;
}
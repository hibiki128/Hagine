#include "../Particle.hlsli"

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

StructuredBuffer<Particle> gParticles : register(t0);
// 生存コンパクション: instanceId -> 生存パーティクルの実 slot index
StructuredBuffer<uint> gAliveList : register(t2);
// 生存コンパクション: 当該フレームの生存数（これを超える instanceId は破棄）
StructuredBuffer<uint> gAliveCount : register(t3);
ConstantBuffer<PerView> gPerView : register(b0);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;

    // instanceCount は生存数の概算(CPU側の1〜2F遅延値+マージン)で発行されるため、
    // 実際の生存数を超える instanceId はここで確実に破棄する。
    if (instanceId >= gAliveCount[0])
    {
        output.position = float4(0.0f, 0.0f, 0.0f, 0.0f); // クリップされる縮退頂点
        output.texcoord = float2(0.0f, 0.0f);
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }

    uint particleSlot = gAliveList[instanceId];
    Particle particle = gParticles[particleSlot];

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

    // ワールド×ビュープロジェクション。回転を使うグループのみ回転行列を合成する
    // （回転なしグループでは sincos×3＋行列積をスキップ＝全頂点ぶんの無駄を削減）。
    float4x4 wvp = mul(worldMatrix, gPerView.viewProjection);
    if (gPerView.enableRotation != 0)
    {
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
        wvp = mul(rotXYZ, wvp);
    }
    output.position = mul(input.position, wvp);
    output.texcoord = input.texcoord;
    output.color = particle.color;
    return output;
}
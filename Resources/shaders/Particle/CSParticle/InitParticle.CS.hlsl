#include "../Particle.hlsli"

ConstantBuffer<ParticleCSSettings> gSettings : register(b0);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
    if (particleIndex < gSettings.maxParticleCount)
    {
        gParticles[particleIndex].translate = float3(0, 0, 0);
        gParticles[particleIndex].scale = float3(0, 0, 0);
        gParticles[particleIndex].lifeTime = 0.0f;
        gParticles[particleIndex].velocity = float3(0, 0, 0);
        gParticles[particleIndex].currentTime = 0.0f;
        gParticles[particleIndex].color = float4(0, 0, 0, 0);
        gParticles[particleIndex].initialScale = float3(0, 0, 0);
        gParticles[particleIndex].padding = 0.0f;
        gParticles[particleIndex].isTrailParticle = 0;
        gParticles[particleIndex].parentIndex = 0xFFFFFFFF;
        gParticles[particleIndex].trailSpawnTimer = 0.0f;
        gParticles[particleIndex].trailSpawnInterval = 0.0f;
        
        // フリーリストの設定
        gFreeList[particleIndex] = particleIndex;
        
        if (particleIndex == 0)
        {
            gFreeListIndex[0] = gSettings.maxParticleCount - 1;
        }
    }
}
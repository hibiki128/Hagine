#include "../Particle.hlsli"

ConstantBuffer<ParticleCSSettings> gSettings : register(b0);
RWStructuredBuffer<uint> gAliveCount : register(u0);
RWStructuredBuffer<Particle> gParticles : register(u1);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = DTid.x;
    
    // 最初のスレッドでカウンターを初期化
    if (particleIndex == 0)
    {
        gAliveCount[0] = 0;
    }
    
    // 全スレッドが初期化を待つ
    GroupMemoryBarrierWithGroupSync();
    
    if (particleIndex < gSettings.maxParticleCount)
    {
        // アルファ値が0より大きければ生存中
        if (gParticles[particleIndex].color.a > 0.0f)
        {
            InterlockedAdd(gAliveCount[0], 1);
        }
    }
}
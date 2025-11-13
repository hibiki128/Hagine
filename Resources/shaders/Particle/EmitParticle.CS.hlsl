#include"Particle.hlsli"
#include"../Random/Random.hlsli"

ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    RandomGenerator generator;
    // グループIDを含めてより一意なシードを生成
    generator.seed = float3(
    DTid.x + gPerFrame.time * 1000.0f + gPerFrame.groupId * 9973.0f,
    DTid.x * 73.0f + gPerFrame.time * 127.0f + gPerFrame.groupId * 7919.0f,
    DTid.x * 151.0f + gPerFrame.time * 223.0f + gPerFrame.groupId * 6547.0f
);
    
    if (DTid.x >= gEmitter.count || gEmitter.emit == 0)
    {
        return;
    }
    
    int freeListIndex;
    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
    if (0 <= freeListIndex && freeListIndex < kMaxParticles)
    {
        int particleIndex = gFreeList[freeListIndex];
        
        gParticles[particleIndex].scale = generator.Generate3d();
        
        // 複数回Generate3d()を呼んで異なる値を確実に取得
        float3 rawDirection = generator.Generate3d() * 2.0f - 1.0f;
        
        // ゼロベクトル対策
        if (length(rawDirection) < 0.001f)
        {
            rawDirection = float3(0.577f, 0.577f, 0.577f); // 正規化済みデフォルト方向
        }
        
        float3 randomDirection = normalize(rawDirection);
        float randomRadius = pow(max(generator.Generate1d(), 0.001f), 1.0f / 3.0f) * gEmitter.radius;
        
        gParticles[particleIndex].translate = gEmitter.translate + randomDirection * randomRadius;
        
        gParticles[particleIndex].color.rgb = generator.Generate3d();
        gParticles[particleIndex].color.a = 1.0f;
        float3 vel = (generator.Generate3d() * 2.0f - 1.0f) * 0.25f;
        gParticles[particleIndex].velocity = vel;
        gParticles[particleIndex].lifeTime = 3.0f;
        gParticles[particleIndex].currentTime = 0.0f;
    }
    else
    {
        InterlockedAdd(gFreeListIndex[0], 1);
    }
}
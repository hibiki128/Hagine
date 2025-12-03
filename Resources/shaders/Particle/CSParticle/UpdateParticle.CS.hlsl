#include "../Particle.hlsli"
#include "../../Random/Random.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
ConstantBuffer<ParticleCSSettings> gSettings : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
RWStructuredBuffer<int> gFreeListTailIndex : register(u3);

void SpawnTrailParticles(int particleIndex, float3 currentPosition)
{
    float parentLifeTime = gParticles[particleIndex].lifeTime;
    float parentCurrentTime = gParticles[particleIndex].currentTime;
    float parentRemainingLife = max(0.0f, parentLifeTime - parentCurrentTime);

    if (parentRemainingLife < gSettings.trailMinLifeTime * 0.2f)
        return;

    float3 lastPos = gParticles[particleIndex].lastTrailPosition;
    float targetDistance = gParticles[particleIndex].trailSpawnDistance;
    
    if (targetDistance <= 0.001f)
        return;

    float3 moveVector = currentPosition - lastPos;
    float totalDistance = length(moveVector);
    
    if (totalDistance < targetDistance)
        return;

    int trailsToSpawn = int(totalDistance / targetDistance);
    trailsToSpawn = min(trailsToSpawn, gSettings.maxTrailPerParticle);
    
    if (trailsToSpawn <= 0)
        return;

    float3 direction = normalize(moveVector);
    int requiredCount = trailsToSpawn;
    uint originalHead;
    InterlockedAdd(gFreeListIndex[0], requiredCount, originalHead);
    uint newHead = originalHead + (requiredCount - 1);
    uint capacity = gSettings.maxParticleCount;
    uint tail = gFreeListTailIndex[0];

    if (newHead >= tail)
    {
        int dummyRollback;
        InterlockedAdd(gFreeListIndex[0], -requiredCount, dummyRollback);
        return;
    }

    float3 parentCurrentScale = gParticles[particleIndex].scale;
    float3 parentVelocity = gParticles[particleIndex].velocity;
    float4 parentColor = gParticles[particleIndex].color;

    float desiredTrailLife = parentRemainingLife * gSettings.trailLifeTimeScale;
    float trailLifeTime = max(desiredTrailLife, gSettings.trailMinLifeTime);
    trailLifeTime = clamp(trailLifeTime, gSettings.trailMinLifeTime, 10.0f);

    for (int i = 0; i < requiredCount; ++i)
    {
        int slot = (originalHead + i) % capacity;
        int trailIndex = gFreeList[slot];

        if (trailIndex < 0 || trailIndex >= gSettings.maxParticleCount)
            continue;

        float spawnDistance = targetDistance * (float(i) + 1.0f);
        float3 spawnPosition = lastPos + direction * spawnDistance;

        RandomGenerator generator;
        generator.InitSeed(
            uint3(particleIndex, gPerFrame.groupId + i, uint(gPerFrame.time * 1000.0f)),
            gPerFrame.time + float(particleIndex) + float(i) * 0.1f
        );

        gParticles[trailIndex].translate = spawnPosition;
        gParticles[trailIndex].initialScale = parentCurrentScale * gSettings.trailScaleMultiplier;
        gParticles[trailIndex].scale = gParticles[trailIndex].initialScale;

        if (gSettings.trailInheritVelocity != 0)
        {
            gParticles[trailIndex].velocity = parentVelocity * gSettings.trailVelocityScale;
        }
        else
        {
            gParticles[trailIndex].velocity = float3(0.0f, 0.0f, 0.0f);
        }

        gParticles[trailIndex].color = parentColor * gSettings.trailColorMultiplier;
        gParticles[trailIndex].lifeTime = trailLifeTime;
        gParticles[trailIndex].currentTime = 0.0f;
        gParticles[trailIndex].isTrailParticle = 1;
        gParticles[trailIndex].parentIndex = particleIndex;
        gParticles[trailIndex].lastTrailPosition = spawnPosition;
        gParticles[trailIndex].trailSpawnDistance = gSettings.trailSpawnDistance;
    }

    float consumedDistance = float(requiredCount) * targetDistance;
    gParticles[particleIndex].lastTrailPosition = lastPos + direction * consumedDistance;
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
    if (particleIndex < gSettings.maxParticleCount)
    {
        if (gParticles[particleIndex].color.a <= 0.0f)
        {
            return;
        }
        
        if (gSettings.enableGravity)
        {
            gParticles[particleIndex].velocity += gSettings.gravity * gPerFrame.deltaTime;
        }
        
        float3 previousPosition = gParticles[particleIndex].translate;
        
        gParticles[particleIndex].translate += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
        gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
        
        float3 currentPosition = gParticles[particleIndex].translate;
        
        float lifeRatio = gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime;
        float alpha = 1.0f - lifeRatio;
        
        if (!gSettings.enableRandomColor)
        {
            gParticles[particleIndex].color = lerp(gSettings.startColor, gSettings.endColor, lifeRatio);
        }
        else
        {
            gParticles[particleIndex].color.a = saturate(alpha);
        }
        
        if (gSettings.enableLifetimeScale)
        {
            float scaleMultiplier = 1.0f - lifeRatio;
            gParticles[particleIndex].scale = gParticles[particleIndex].initialScale * scaleMultiplier;
        }

        if (gSettings.enableSinScale)
        {
            float sinWave = sin(gParticles[particleIndex].currentTime * gSettings.sinScaleFrequency) * 0.5f + 0.5f;
            float sinMultiplier = 1.0f + (sinWave * 2.0f - 1.0f) * gSettings.sinScaleAmplitude;
            gParticles[particleIndex].scale = gParticles[particleIndex].initialScale * sinMultiplier;
        }

        if (gSettings.enableLifetimeScale && gSettings.enableSinScale)
        {
            float lifetimeMultiplier = 1.0f - lifeRatio;
            float sinWave = sin(gParticles[particleIndex].currentTime * gSettings.sinScaleFrequency) * 0.5f + 0.5f;
            float sinMultiplier = 1.0f + (sinWave * 2.0f - 1.0f) * gSettings.sinScaleAmplitude;
            gParticles[particleIndex].scale = gParticles[particleIndex].initialScale * lifetimeMultiplier * sinMultiplier;
        }
        
        gParticles[particleIndex].color.a = saturate(alpha);
        
        if (gSettings.enableTrail &&
            gParticles[particleIndex].isTrailParticle == 0 &&
            gParticles[particleIndex].color.a > 0.05f)
        {
            SpawnTrailParticles(particleIndex, currentPosition);
        }
        
        if (gParticles[particleIndex].color.a <= 0.0f)
        {
            gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
            gParticles[particleIndex].lastTrailPosition = float3(0.0f, 0.0f, 0.0f);
            
            int oldTail;
            InterlockedAdd(gFreeListTailIndex[0], 1, oldTail);
            
            int slot = oldTail % gSettings.maxParticleCount;
            gFreeList[slot] = particleIndex;
        }
    }
}
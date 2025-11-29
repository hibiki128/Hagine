#include "../Particle.hlsli"
#include "../../Random/Random.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
ConstantBuffer<ParticleCSSettings> gSettings : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

void SpawnTrailParticleAtPosition(uint parentIndex, float3 spawnPosition, RandomGenerator generator)
{
    if (parentIndex >= gSettings.maxParticleCount)
    {
        return;
    }
    
    int freeListIndex;
    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
    
    if (freeListIndex < 0 || freeListIndex >= gSettings.maxParticleCount)
    {
        return;
    }
    
    int trailIndex = gFreeList[freeListIndex];
    
    if (trailIndex < 0 || trailIndex >= gSettings.maxParticleCount)
    {
        return;
    }
    
    // 親パーティクルの情報を取得
    float3 parentCurrentScale = gParticles[parentIndex].scale;
    float3 parentVelocity = gParticles[parentIndex].velocity;
    float4 parentColor = gParticles[parentIndex].color;
    float parentLifeTime = gParticles[parentIndex].lifeTime;
    float parentCurrentTime = gParticles[parentIndex].currentTime;
    
    // 親の残り寿命を計算
    float parentRemainingLife = max(0.0f, parentLifeTime - parentCurrentTime);
    
    // ★ 修正: 残り寿命チェックを緩和（最小寿命の20%まで許容）
    if (parentRemainingLife < gSettings.trailMinLifeTime * 0.2f)
    {
        return;
    }
    
    // トレイルの設定
    gParticles[trailIndex].translate = spawnPosition;
    gParticles[trailIndex].initialScale = parentCurrentScale * gSettings.trailScaleMultiplier;
    gParticles[trailIndex].scale = gParticles[trailIndex].initialScale;
    
    if (gSettings.trailInheritVelocity)
    {
        gParticles[trailIndex].velocity = parentVelocity * gSettings.trailVelocityScale;
    }
    else
    {
        gParticles[trailIndex].velocity = float3(0, 0, 0);
    }
    
    gParticles[trailIndex].color = parentColor * gSettings.trailColorMultiplier;
    
    // ★ 修正: トレイルの寿命計算を改善
    // 親の残り寿命に関わらず、最小寿命は保証する
    float desiredTrailLife = parentRemainingLife * gSettings.trailLifeTimeScale;
    float trailLifeTime = max(desiredTrailLife, gSettings.trailMinLifeTime);
    
    // ★ 重要: 親より長生きしてもOKにする（トレイルが残像として残る）
    trailLifeTime = clamp(trailLifeTime, gSettings.trailMinLifeTime, 10.0f);
    
    gParticles[trailIndex].lifeTime = trailLifeTime;
    gParticles[trailIndex].currentTime = 0.0f;
    
    gParticles[trailIndex].isTrailParticle = 1;
    gParticles[trailIndex].parentIndex = parentIndex;
    gParticles[trailIndex].lastTrailPosition = spawnPosition;
    gParticles[trailIndex].trailSpawnDistance = gSettings.trailSpawnDistance;
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
        
        // ★ トレイル生成処理（条件を簡略化）
        if (gSettings.enableTrail &&
            gParticles[particleIndex].isTrailParticle == 0 &&
            gParticles[particleIndex].color.a > 0.05f)  // ★ 閾値を下げる
        {
            float3 lastPos = gParticles[particleIndex].lastTrailPosition;
            float targetDistance = gParticles[particleIndex].trailSpawnDistance;
            
            if (targetDistance > 0.001f)
            {
                float3 moveVector = currentPosition - lastPos;
                float totalDistance = length(moveVector);
                
                if (totalDistance >= targetDistance)
                {
                    int trailsToSpawn = int(totalDistance / targetDistance);
                    trailsToSpawn = min(trailsToSpawn, gSettings.maxTrailPerParticle);
                    
                    if (trailsToSpawn > 0)
                    {
                        float3 direction = normalize(moveVector);
                        
                        for (int i = 0; i < trailsToSpawn; i++)
                        {
                            float spawnDistance = targetDistance * (float(i) + 1.0f);
                            float3 spawnPosition = lastPos + direction * spawnDistance;
                            
                            RandomGenerator generator;
                            generator.InitSeed(
                                uint3(particleIndex, gPerFrame.groupId + i, uint(gPerFrame.time * 1000.0f)),
                                gPerFrame.time + float(particleIndex) + float(i) * 0.1f
                            );
                            
                            SpawnTrailParticleAtPosition(particleIndex, spawnPosition, generator);
                        }
                        
                        float consumedDistance = float(trailsToSpawn) * targetDistance;
                        gParticles[particleIndex].lastTrailPosition = lastPos + direction * consumedDistance;
                    }
                }
            }
        }
        
        if (gParticles[particleIndex].color.a <= 0.0f)
        {
            gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
            gParticles[particleIndex].lastTrailPosition = float3(0.0f, 0.0f, 0.0f);
            
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
            
            if (freeListIndex + 1 < gSettings.maxParticleCount)
            {
                gFreeList[freeListIndex + 1] = particleIndex;
            }
        }
    }
}
#include "../Particle.hlsli"
#include "../../Random/Random.hlsli"
#include "CurlNoise.hlsli"

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
            return;
        
        // 1. 加速度処理
        if (gSettings.enableAcceleration)
        {
            gParticles[particleIndex].velocity += gSettings.acceleration * gPerFrame.deltaTime;
        }
        
        // 2. 重力処理
        if (gSettings.enableGravity)
        {
            gParticles[particleIndex].velocity += gSettings.gravity * gPerFrame.deltaTime;
        }
        
        // 3. 速度減衰処理
        if (gSettings.enableVelocityDamping)
        {
            gParticles[particleIndex].velocity *= pow(gSettings.velocityDampingFactor, gPerFrame.deltaTime * 60.0f);
        }
        
        // 4. ライフタイムに応じた速度減衰
        if (gSettings.enableLifetimeVelocityDamping)
        {
            float lifeRatio = gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime;
            
            if (lifeRatio >= gSettings.lifetimeVelocityDampingStart)
            {
                float dampingProgress = (lifeRatio - gSettings.lifetimeVelocityDampingStart) /
                                       (1.0f - gSettings.lifetimeVelocityDampingStart);
                float dampingMultiplier = 1.0f - (dampingProgress * dampingProgress);
                gParticles[particleIndex].velocity *= dampingMultiplier;
            }
        }
        
        // 5. ギャザー処理
        if (gSettings.enableGather)
        {
            bool isTrail = (gParticles[particleIndex].isTrailParticle != 0);
            
            if (!isTrail || gSettings.enableGatherForTrail)
            {
                float lifeRatio = gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime;
        
                if (lifeRatio >= gSettings.gatherStartRatio)
                {
                    float3 targetPosition = gSettings.gatherTarget;
                    float3 toTarget = targetPosition - gParticles[particleIndex].translate;
                    float distance = length(toTarget);

                    if (distance > 0.01f)
                    {
                        float3 dirToTarget = normalize(toTarget);
                        float t = (lifeRatio - gSettings.gatherStartRatio) / (1.0f - gSettings.gatherStartRatio);
                        t = t * t;
                        
                        float3 desiredVelocity = dirToTarget * gSettings.gatherStrength;
                        float lerpFactor = 5.0f * gPerFrame.deltaTime;
                
                        gParticles[particleIndex].velocity = lerp(gParticles[particleIndex].velocity, desiredVelocity, lerpFactor * t * 10.0f);
                    }
                }
            }
        }

        // 6. 渦巻き（Vortex）処理
        if (gSettings.enableVortex)
        {
            bool isTrail = (gParticles[particleIndex].isTrailParticle != 0);
            
            if (!isTrail || gSettings.enableVortexForTrail)
            {
                float3 center = gSettings.vortexTarget;
                float3 toParticle = gParticles[particleIndex].translate - center;
                float dist = length(toParticle);
                
                if (dist > 0.05f)
                {
                    float3 axis = gSettings.vortexAxis;
                    if (length(axis) < 0.001f)
                        axis = float3(0, 1, 0);
                    else
                        axis = normalize(axis);

                    float3 tangent = cross(normalize(toParticle), axis);
                    gParticles[particleIndex].velocity += tangent * gSettings.vortexStrength * gPerFrame.deltaTime;
                }
            }
        }
        
        // 7. Curl Noise による速度場
        // ポイント：velocity を「完全に置き換える」ことで
        // パーティクルが常にフィールドに沿って動く流体的な挙動になる。
        // 加算方式では加速し続けてバラバラに飛散してしまう。
        if (gSettings.enableCurlNoise)
        {
            bool isTrail = (gParticles[particleIndex].isTrailParticle != 0);
            if (!isTrail)
            {
                float3 pos = gParticles[particleIndex].translate;
                
                // Curl Noise 速度場を計算して velocity を置き換え
                float3 curlVel = ComputeCurlNoise(
                    pos,
                    gSettings.curlNoiseScale,
                    gPerFrame.time * gSettings.curlNoiseTimeScale,
                    (int) gSettings.curlNoiseOctaves
                ) * gSettings.curlNoiseStrength;
                
                // 引き戻し力（Attract）
                // パーティクルをエミッター付近に留まらせながら流れさせる。
                // curlNoiseAttractStrength = 0 で無効（純粋なCurlのみ）。
                if (gSettings.curlNoiseAttractStrength > 0.0f)
                {
                    float3 toCenter = gSettings.curlNoiseAttractCenter - pos;
                    float dist = length(toCenter);
                    if (dist > 0.001f)
                    {
                        // 距離に比例した引き戻し力（遠いほど強く引く）
                        float3 attractVel = normalize(toCenter) * dist * gSettings.curlNoiseAttractStrength;
                        curlVel += attractVel;
                    }
                }
                
                // velocity を完全置き換え
                gParticles[particleIndex].velocity = curlVel;
            }
        }
        
        // 8. 移動更新
        float3 previousPosition = gParticles[particleIndex].translate;
        gParticles[particleIndex].translate += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
        gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
        float3 currentPosition = gParticles[particleIndex].translate;
        
        // 9. 各種パラメータ更新
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
        
        // 10. トレイル生成
        if (gSettings.enableTrail &&
            gParticles[particleIndex].isTrailParticle == 0 &&
            gParticles[particleIndex].color.a > 0.05f)
        {
            SpawnTrailParticles(particleIndex, currentPosition);
        }
        
        // 死亡判定
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
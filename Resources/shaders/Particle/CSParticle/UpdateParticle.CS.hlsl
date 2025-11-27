#include "../Particle.hlsli"
#include "../../Random/Random.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
ConstantBuffer<ParticleCSSettings> gSettings : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

// トレイルパーティクルを生成する関数
void SpawnTrailParticle(uint parentIndex, RandomGenerator generator)
{
    // 親パーティクルのインデックスが有効か確認
    if (parentIndex >= gSettings.maxParticleCount)
    {
        return;
    }
    
    // 親パーティクルが生きているか確認
    if (gParticles[parentIndex].color.a <= 0.0f)
    {
        return;
    }
    
    // フリーリストから新しいパーティクルのインデックスを取得
    int freeListIndex;
    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
    
    if (freeListIndex < 0 || freeListIndex >= gSettings.maxParticleCount)
    {
        return;
    }
    
    int trailIndex = gFreeList[freeListIndex];
    
    // トレイルインデックスが有効か確認
    if (trailIndex < 0 || trailIndex >= gSettings.maxParticleCount)
    {
        // インデックスを戻す
        InterlockedAdd(gFreeListIndex[0], 1);
        return;
    }
    
    // 親パーティクルの情報を安全に読み取る
    float3 parentTranslate = gParticles[parentIndex].translate;
    float3 parentScale = gParticles[parentIndex].scale;
    float3 parentInitialScale = gParticles[parentIndex].initialScale;
    float3 parentVelocity = gParticles[parentIndex].velocity;
    float4 parentColor = gParticles[parentIndex].color;
    float parentLifeTime = gParticles[parentIndex].lifeTime;
    float parentCurrentTime = gParticles[parentIndex].currentTime;
    
    // スケールの異常値チェック（安全策）
    if (any(isinf(parentScale)) || any(isnan(parentScale)) ||
        length(parentScale) > 1000.0f || length(parentScale) < 0.001f)
    {
        parentScale = parentInitialScale;
    }
    
    // トレイルパーティクルの設定
    gParticles[trailIndex].translate = parentTranslate;
    
    // initialScaleを使用して計算（より安定）
    float3 trailScale = parentInitialScale * gSettings.trailScaleMultiplier;
    gParticles[trailIndex].scale = trailScale;
    gParticles[trailIndex].initialScale = trailScale;
    
    // 速度の継承
    if (gSettings.trailInheritVelocity)
    {
        gParticles[trailIndex].velocity = parentVelocity * gSettings.trailVelocityScale;
    }
    else
    {
        gParticles[trailIndex].velocity = float3(0, 0, 0);
    }
    
    // 色の設定（親の色 × トレイル倍率）
    gParticles[trailIndex].color = parentColor * gSettings.trailColorMultiplier;
    
    // アルファ値が異常に高くならないようにクランプ
    gParticles[trailIndex].color.a = saturate(gParticles[trailIndex].color.a);
    
    // 寿命の設定（親の残り寿命 × トレイル寿命倍率）
    float parentRemainingLife = max(0.0f, parentLifeTime - parentCurrentTime);
    float trailLifeTime = parentRemainingLife * gSettings.trailLifeTimeScale;
    
    // 寿命の最小値と最大値を設定（異常値防止）
    trailLifeTime = clamp(trailLifeTime, 0.1f, 10.0f);
    
    gParticles[trailIndex].lifeTime = trailLifeTime;
    gParticles[trailIndex].currentTime = 0.0f;
    
    // トレイルフラグを立てる
    gParticles[trailIndex].isTrailParticle = 1;
    gParticles[trailIndex].parentIndex = parentIndex;
    gParticles[trailIndex].trailSpawnTimer = 0.0f;
    gParticles[trailIndex].trailSpawnInterval = 0.0f; // トレイルはさらにトレイルを生成しない
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
    if (particleIndex < gSettings.maxParticleCount)
    {
        if (gParticles[particleIndex].color.a != 0)
        {
            // 重力の適用
            if (gSettings.enableGravity)
            {
                gParticles[particleIndex].velocity += gSettings.gravity * gPerFrame.deltaTime;
            }
            
            // 位置更新
            gParticles[particleIndex].translate += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
            
            // 時間更新
            gParticles[particleIndex].currentTime += gPerFrame.deltaTime;
            
            // 寿命に基づくアルファ値計算
            float lifeRatio = gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime;
            float alpha = 1.0f - lifeRatio;
            
            // ランダムカラーでない場合、色補間
            if (!gSettings.enableRandomColor)
            {
                gParticles[particleIndex].color = lerp(gSettings.startColor, gSettings.endColor, lifeRatio);
            }
            else
            {
                gParticles[particleIndex].color.a = saturate(alpha);
            }
            
            // 寿命に応じてスケール変更
            if (gSettings.enableLifetimeScale)
            {
                float scaleMultiplier = 1.0f - lifeRatio;
                gParticles[particleIndex].scale = gParticles[particleIndex].initialScale * scaleMultiplier;
            }

            // Sin波による拡縮
            if (gSettings.enableSinScale)
            {
                float sinWave = sin(gParticles[particleIndex].currentTime * gSettings.sinScaleFrequency) * 0.5f + 0.5f;
                float sinMultiplier = 1.0f + (sinWave * 2.0f - 1.0f) * gSettings.sinScaleAmplitude;
                gParticles[particleIndex].scale = gParticles[particleIndex].initialScale * sinMultiplier;
            }

            // 両方有効な場合は組み合わせる
            if (gSettings.enableLifetimeScale && gSettings.enableSinScale)
            {
                float lifetimeMultiplier = 1.0f - lifeRatio;
                float sinWave = sin(gParticles[particleIndex].currentTime * gSettings.sinScaleFrequency) * 0.5f + 0.5f;
                float sinMultiplier = 1.0f + (sinWave * 2.0f - 1.0f) * gSettings.sinScaleAmplitude;
                gParticles[particleIndex].scale = gParticles[particleIndex].initialScale * lifetimeMultiplier * sinMultiplier;
            }
            
            // アルファ値設定
            gParticles[particleIndex].color.a = saturate(alpha);
            
            // トレイル生成処理（親パーティクルのみ、かつ十分に生きている場合）
            if (gSettings.enableTrail &&
                gParticles[particleIndex].isTrailParticle == 0 &&
                gParticles[particleIndex].color.a > 0.1f &&
                gParticles[particleIndex].currentTime < gParticles[particleIndex].lifeTime * 0.9f)
            {
                gParticles[particleIndex].trailSpawnTimer += gPerFrame.deltaTime;
                
                if (gParticles[particleIndex].trailSpawnTimer >= gSettings.trailSpawnInterval)
                {
                    gParticles[particleIndex].trailSpawnTimer -= gSettings.trailSpawnInterval;
                    
                    // タイマーが異常に大きくなるのを防ぐ
                    gParticles[particleIndex].trailSpawnTimer = clamp(gParticles[particleIndex].trailSpawnTimer, 0.0f, gSettings.trailSpawnInterval);
                    
                    // ランダムジェネレータを初期化
                    RandomGenerator generator;
                    generator.InitSeed(
                        uint3(particleIndex, gPerFrame.groupId, uint(gPerFrame.time * 1000.0f)),
                        gPerFrame.time + float(particleIndex)
                    );
                    
                    // トレイルパーティクルを生成
                    SpawnTrailParticle(particleIndex, generator);
                }
            }
            
            // パーティクルが死んだかチェック
            if (gParticles[particleIndex].color.a <= 0.0f)
            {
                gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
                
                int freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
                
                if (freeListIndex + 1 < gSettings.maxParticleCount)
                {
                    gFreeList[freeListIndex + 1] = particleIndex;
                }
            }
        }
    }
}
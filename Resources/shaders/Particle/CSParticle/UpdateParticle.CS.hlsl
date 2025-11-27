#include "../Particle.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
ConstantBuffer<ParticleCSSettings> gSettings : register(b1);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

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
            
            // パーティクルが死んだかチェック
            if (gParticles[particleIndex].color.a <= 0.0f)
            {
                gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
                
                int freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
                
                // 範囲チェックしてから書き込み（Interlocked不要）
                if (freeListIndex + 1 < gSettings.maxParticleCount)
                {
                    gFreeList[freeListIndex + 1] = particleIndex;
                }
            }
        }
    }
}
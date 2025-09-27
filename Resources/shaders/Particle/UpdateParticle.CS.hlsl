#include "Particle.hlsli"

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
                gParticles[particleIndex].scale *= scaleMultiplier;
            }
            
            // アルファ値設定
            gParticles[particleIndex].color.a = saturate(alpha);
        }
        
        // パーティクルが死んだ場合の処理
        if (gParticles[particleIndex].color.a <= 0.0f)
        {
            gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
            if ((freeListIndex + 1) < gSettings.maxParticleCount)
            {
                gFreeList[freeListIndex + 1] = particleIndex;
            }
            else
            {
                InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
            }
        }
    }
}
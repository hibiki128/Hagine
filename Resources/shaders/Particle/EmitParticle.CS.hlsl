#include"Particle.hlsli"
#include"../Random/Random.hlsli"

ConstantBuffer<EmitterMesh> gEmitterMesh : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
ConstantBuffer<ParticleCSSettings> gSettings : register(b2);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
StructuredBuffer<TriangleInfo> gTriangles : register(t0);
StructuredBuffer<float> gTriangleCDF : register(t1);

float3x3 CreateRotationMatrix(float3 rotation)
{
    float cosX = cos(rotation.x);
    float sinX = sin(rotation.x);
    float cosY = cos(rotation.y);
    float sinY = sin(rotation.y);
    float cosZ = cos(rotation.z);
    float sinZ = sin(rotation.z);
    
    float3x3 rotX = float3x3(1.0f, 0.0f, 0.0f, 0.0f, cosX, -sinX, 0.0f, sinX, cosX);
    float3x3 rotY = float3x3(cosY, 0.0f, sinY, 0.0f, 1.0f, 0.0f, -sinY, 0.0f, cosY);
    float3x3 rotZ = float3x3(cosZ, -sinZ, 0.0f, sinZ, cosZ, 0.0f, 0.0f, 0.0f, 1.0f);
    
    return mul(mul(rotZ, rotY), rotX);
}

float3 ApplyScale(float3 vertex, float3 scale)
{
    return vertex * scale;
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitterMesh.emit == 0)
        return;
    
    if (DTid.x >= gSettings.emitCount)
        return;
    
    RandomGenerator generator;
    generator.seed = float3(
        DTid.x + gPerFrame.time * 1000.0f + gPerFrame.groupId * 9973.0f,
        DTid.x * 73.0f + gPerFrame.time * 127.0f + gPerFrame.groupId * 7919.0f,
        DTid.x * 151.0f + gPerFrame.time * 223.0f + gPerFrame.groupId * 6547.0f
    );
    
    int freeListIndex;
    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
    if (0 <= freeListIndex && freeListIndex < gSettings.maxParticleCount)
    {
        int particleIndex = gFreeList[freeListIndex];
        
        float scaleValue = lerp(gSettings.scaleMin, gSettings.scaleMax, generator.Generate1d());
        gParticles[particleIndex].scale = float3(scaleValue, scaleValue, scaleValue);
        
        float3 emitPosition;

        if (gEmitterMesh.triangleCount > 0)
        {
            // ハイブリッド方式：基本は順序的だが、ランダムなブレを加える
            float baseRatio = float(DTid.x) / float(gSettings.emitCount);
            float randomOffset = (generator.Generate1d() - 0.5f) * 0.2f; // ±10%のブレ
            float particleRatio = saturate(baseRatio + randomOffset);
            
            // 二分探索で三角形を選択
            uint triIndex = 0;
            uint left = 0;
            uint right = gEmitterMesh.triangleCount - 1;
            
            while (left < right)
            {
                uint mid = (left + right) / 2;
                if (gTriangleCDF[mid] < particleRatio)
                {
                    left = mid + 1;
                }
                else
                {
                    right = mid;
                }
            }
            triIndex = left;
            
            float3 v0 = gTriangles[triIndex].v0;
            float3 v1 = gTriangles[triIndex].v1;
            float3 v2 = gTriangles[triIndex].v2;
            
            // 三角形面上のランダムな点
            float u = generator.Generate1d();
            float v = generator.Generate1d();
            if (u + v > 1.0f)
            {
                u = 1.0f - u;
                v = 1.0f - v;
            }
            float3 randomPoint = v0 + u * (v1 - v0) + v * (v2 - v0);
            
            randomPoint = ApplyScale(randomPoint, gEmitterMesh.scale);
            float3x3 rotMatrix = CreateRotationMatrix(gEmitterMesh.rotation);
            randomPoint = mul(rotMatrix, randomPoint);
            
            emitPosition = gEmitterMesh.translate + randomPoint;
        }
        else
        {
            emitPosition = gEmitterMesh.translate;
        }

        gParticles[particleIndex].translate = emitPosition;
        
        if (gSettings.enableRandomColor)
        {
            gParticles[particleIndex].color.rgb = generator.Generate3d();
            gParticles[particleIndex].color.a = 1.0f;
        }
        else
        {
            gParticles[particleIndex].color = gSettings.startColor;
        }
        
        float3 vel = float3(
            lerp(gSettings.velocityMin.x, gSettings.velocityMax.x, generator.Generate1d()),
            lerp(gSettings.velocityMin.y, gSettings.velocityMax.y, generator.Generate1d()),
            lerp(gSettings.velocityMin.z, gSettings.velocityMax.z, generator.Generate1d())
        );
        gParticles[particleIndex].velocity = vel;
        
        gParticles[particleIndex].lifeTime = lerp(gSettings.lifeTimeMin, gSettings.lifeTimeMax, generator.Generate1d());
        gParticles[particleIndex].currentTime = 0.0f;
    }
    else
    {
        InterlockedAdd(gFreeListIndex[0], 1);
    }
}
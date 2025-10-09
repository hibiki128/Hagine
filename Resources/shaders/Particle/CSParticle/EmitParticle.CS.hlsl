#include "../../Random/Random.hlsli"
#include"../Particle.hlsli"

ConstantBuffer<EmitterMesh> gEmitterMesh : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
ConstantBuffer<ParticleCSSettings> gSettings : register(b2);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
StructuredBuffer<TriangleInfo> gTriangles : register(t0);
StructuredBuffer<float> gTriangleCDF : register(t1);
StructuredBuffer<EdgeInfo> gEdges : register(t2);

float3x3 CreateRotationMatrixFromQuaternion(float4 q)
{
    float x = -q.x, y = -q.y, z = -q.z, w = q.w;
    
    return float3x3(
        1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y),
        2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x),
        2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)
    );
}
float3 ApplyScale(float3 vertex, float3 scale)
{
    return vertex * scale;
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitterMesh.emit == 0)
        return;
    
    if (DTid.x >= gSettings.emitCount)
        return;
    
    RandomGenerator generator;
    generator.InitSeed(
        uint3(DTid.x, gPerFrame.groupId, DTid.x * 7919),
        gPerFrame.time
    );
    
    int freeListIndex;
    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
    if (0 <= freeListIndex && freeListIndex < gSettings.maxParticleCount)
    {
        int particleIndex = gFreeList[freeListIndex];
        
        float scaleValue = lerp(gSettings.scaleMin, gSettings.scaleMax, generator.Generate1d());
        gParticles[particleIndex].scale = float3(scaleValue, scaleValue, scaleValue);
        gParticles[particleIndex].initialScale = float3(scaleValue, scaleValue, scaleValue);
        
        float3 emitPosition;

        if (gEmitterMesh.triangleCount > 0 || gEmitterMesh.edgeCount > 0)
        {
            float3 randomPoint;
    
            if (gEmitterMesh.emitFromSurface == 2 && gEmitterMesh.edgeCount > 0)
            {
                // エッジモード: 線上に発生
                uint edgeIndex = uint(generator.Generate1d() * float(gEmitterMesh.edgeCount)) % gEmitterMesh.edgeCount;
                float t = generator.Generate1d();
            
                float3 v0 = gEdges[edgeIndex].v0;
                float3 v1 = gEdges[edgeIndex].v1;
            
                randomPoint = lerp(v0, v1, t);
            }
            else if (gEmitterMesh.emitFromSurface == 1 && gEmitterMesh.triangleCount > 0)
            {
                // 表面モード: 三角形の表面に発生
                float particleRatio = generator.Generate1d();
        
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
        
                float u = generator.Generate1d();
                float v = generator.Generate1d();
                if (u + v > 1.0f)
                {
                    u = 1.0f - u;
                    v = 1.0f - v;
                }
                randomPoint = v0 + u * (v1 - v0) + v * (v2 - v0);
            }
            else
            {
                // 内部モード: ボリューム内に発生
                randomPoint = float3(
                    generator.Generate1d() * 2.0f - 1.0f,
                    generator.Generate1d() * 2.0f - 1.0f,
                    generator.Generate1d() * 2.0f - 1.0f
                );
            }
    
            randomPoint = ApplyScale(randomPoint, gEmitterMesh.scale);
            float3x3 rotMatrix = CreateRotationMatrixFromQuaternion(gEmitterMesh.rotation);
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
            gParticles[particleIndex].color.rgb = generator.Generate3d() * 0.5f + 0.5f;
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
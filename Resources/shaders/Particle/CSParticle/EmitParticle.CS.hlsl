#include "../../Random/Random.hlsli"
#include"../Particle.hlsli"

ConstantBuffer<EmitterMesh> gEmitterMesh : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
ConstantBuffer<ParticleCSSettings> gSettings : register(b2);
ConstantBuffer<FieldCountCB> gFieldCB : register(b3);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
RWStructuredBuffer<int> gFreeListTailIndex : register(u3);
StructuredBuffer<TriangleInfo> gTriangles : register(t0);
StructuredBuffer<float> gTriangleCDF : register(t1);
StructuredBuffer<EdgeInfo> gEdges : register(t2);
StructuredBuffer<ParticleField> gFields : register(t3);

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

// enableEmitSpawn=1 のフィールドが存在する場合、pos がそのいずれかの範囲内にあるか判定。
// enableEmitSpawn=1 のフィールドが1つも存在しない場合は true を返す（制限なし）。
// hitFieldIndex: 最初にヒットしたフィールドのインデックス（-1=ヒットなし）
bool ShouldEmitAtPosition(float3 pos, out int hitFieldIndex)
{
    hitFieldIndex = -1;
    bool hasEmitSpawnField = false;

    for (uint i = 0; i < gFieldCB.fieldCount; i++)
    {
        if (gFields[i].enableEmitSpawn == 0)
            continue;

        hasEmitSpawnField = true;

        float3 diff = pos - gFields[i].position;
        if (dot(diff, diff) < gFields[i].radius * gFields[i].radius)
        {
            hitFieldIndex = (int) i;
            return true;
        }
    }

    return !hasEmitSpawnField;
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitterMesh.emit == 0)
        return;
    
    if (DTid.x >= gSettings.emitCount)
        return;
    
    uint headOld;
    InterlockedAdd(gFreeListIndex[0], 1, headOld);
    uint tailVal = gFreeListTailIndex[0];
    
    if (headOld >= tailVal)
    {
        int dummy;
        InterlockedAdd(gFreeListIndex[0], -1, dummy);
        return;
    }
    
    uint freePos = headOld % gSettings.maxParticleCount;
    uint particleIndex = gFreeList[freePos];
    
    RandomGenerator generator;
    generator.InitSeed(
        uint3(DTid.x, gPerFrame.groupId, DTid.x * 7919),
        gPerFrame.time
    );

    // -------------------------------------------------------
    // emitPosition を先に計算する
    // (フィールド判定を scale 書き込みより前に行うため)
    // -------------------------------------------------------
    float3 emitPosition;

    if (gEmitterMesh.triangleCount > 0 || gEmitterMesh.edgeCount > 0)
    {
        float3 randomPoint;

        if (gEmitterMesh.emitFromSurface == 2 && gEmitterMesh.edgeCount > 0)
        {
            uint edgeIndex = uint(generator.Generate1d() * float(gEmitterMesh.edgeCount)) % gEmitterMesh.edgeCount;
            float t = generator.Generate1d();
        
            float3 v0 = gEdges[edgeIndex].v0;
            float3 v1 = gEdges[edgeIndex].v1;
        
            randomPoint = lerp(v0, v1, t);
        }
        else if (gEmitterMesh.emitFromSurface == 1 && gEmitterMesh.triangleCount > 0)
        {
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
            float3 randomPoint01 = float3(
                generator.Generate1d(),
                generator.Generate1d(),
                generator.Generate1d()
            );
            
            float3 offset = (gEmitterMesh.anchorPoint - 0.5f) * 4.0f;
            float3 rangeMin = -1.0f + offset;
            float3 rangeMax = 1.0f + offset;
            
            randomPoint = lerp(rangeMin, rangeMax, randomPoint01);
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

    // ===== Emit位置フィールド判定 =====
    // enableEmitSpawn=1 のフィールドが存在する場合、フィールド外ならキャンセル。
    // scale をまだ書いていないため、スロットをそのまま tail に返せる。
    // (gParticles[particleIndex] は前回の値が残っているが
    //  freeList に返却されれば次のEmitで上書きされるので問題なし)
    int hitFieldIndex;
    if (!ShouldEmitAtPosition(emitPosition, hitFieldIndex))
    {
        int oldTail;
        InterlockedAdd(gFreeListTailIndex[0], 1, oldTail);
        int slot = oldTail % (int) gSettings.maxParticleCount;
        gFreeList[slot] = particleIndex;
        return;
    }
    // ===================================

    // フィールド判定通過後に scale を書き込む
    float scaleValue = lerp(gSettings.scaleMin, gSettings.scaleMax, generator.Generate1d());
    gParticles[particleIndex].scale = float3(scaleValue, scaleValue, scaleValue);
    gParticles[particleIndex].initialScale = float3(scaleValue, scaleValue, scaleValue);

    gParticles[particleIndex].translate = emitPosition;
    gParticles[particleIndex].lastTrailPosition = emitPosition;
    
    if (gSettings.enableRandomColor)
    {
        gParticles[particleIndex].color.rgb = generator.Generate3d() * 0.5f + 0.5f;
        gParticles[particleIndex].color.a = 1.0f;
    }
    else
    {
        gParticles[particleIndex].color = gSettings.startColor;
    }
    
    float3 vel;
    
    if (gSettings.enableRadialVelocity)
    {
        float3 radialDirection = normalize(emitPosition - gSettings.radialVelocityCenter);
        
        if (gSettings.radialVelocityRandomness > 0.0f)
        {
            float3 randomOffset = float3(
                (generator.Generate1d() - 0.5f) * 2.0f,
                (generator.Generate1d() - 0.5f) * 2.0f,
                (generator.Generate1d() - 0.5f) * 2.0f
            ) * gSettings.radialVelocityRandomness;
            
            radialDirection = normalize(radialDirection + randomOffset);
        }
        
        float speed = lerp(gSettings.velocityMin.x, gSettings.velocityMax.x, generator.Generate1d());
        vel = radialDirection * speed * gSettings.radialVelocityStrength;
    }
    else
    {
        vel = float3(
            lerp(gSettings.velocityMin.x, gSettings.velocityMax.x, generator.Generate1d()),
            lerp(gSettings.velocityMin.y, gSettings.velocityMax.y, generator.Generate1d()),
            lerp(gSettings.velocityMin.z, gSettings.velocityMax.z, generator.Generate1d())
        );
    }
    
    gParticles[particleIndex].velocity = vel;
    
    gParticles[particleIndex].lifeTime = lerp(gSettings.lifeTimeMin, gSettings.lifeTimeMax, generator.Generate1d());
    gParticles[particleIndex].currentTime = 0.0f;

    // enableEmitSpawn フィールドにヒットしていれば寿命をフィールド値で上書き
    if (hitFieldIndex >= 0 && gFields[hitFieldIndex].emitSpawnLifeTimeMax > 0.0f)
    {
        gParticles[particleIndex].lifeTime = lerp(
            gFields[hitFieldIndex].emitSpawnLifeTimeMin,
            gFields[hitFieldIndex].emitSpawnLifeTimeMax,
            generator.Generate1d()
        );
    }
    
    gParticles[particleIndex].isTrailParticle = 0;
    gParticles[particleIndex].parentIndex = 0xFFFFFFFF;
    gParticles[particleIndex].trailSpawnDistance = gSettings.trailSpawnDistance;
    gParticles[particleIndex].settingsOverrideFlags = uint2(0u, 0u);
    
    // ---- 終了スケール ----
    gParticles[particleIndex].endScale = gSettings.endScaleValue;
    
    // ---- 初期回転 ----
    if (gSettings.enableRandomRotation)
    {
        gParticles[particleIndex].rotation = lerp(gSettings.rotationMin, gSettings.rotationMax, generator.Generate1d());
    }
    else
    {
        gParticles[particleIndex].rotation = 0.0f;
    }
    
    // ---- 角速度 ----
    if (gSettings.enableRandomAngularVelocity)
    {
        gParticles[particleIndex].angularVelocity = lerp(gSettings.angularVelocityMin, gSettings.angularVelocityMax, generator.Generate1d());
    }
    else
    {
        gParticles[particleIndex].angularVelocity = 0.0f;
    }
}

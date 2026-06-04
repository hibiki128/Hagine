// =============================================
// EmitParticleIndirect.CS  (Phase 3: ping-pong + DispatchIndirect)
//
//   EmitParticle.CS の派生版。発生ロジックは完全に同一だが、発生に成功した
//   スロットを「このフレームの出力 aliveList B」にも append する点だけが異なる。
//
//   理由: indirect 版 Update は入力 A（前フレームの生存スロット）だけを処理する
//   ため、今フレーム新規発生したスロットは Update では拾われない。発生時点で
//   B に積んでおくことで、新規パーティクルも当フレームから描画対象になる。
//
//   旧 EmitParticle.CS は非改変のまま残し、フラグ OFF 時は従来パスが動く。
// =============================================
#include "../../Random/Random.hlsli"
#include "../Particle.hlsli"

ConstantBuffer<EmitterMesh> gEmitterMesh : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
ConstantBuffer<ParticleCSSettings> gSettings : register(b2);
ConstantBuffer<FieldCountCB> gFieldCB : register(b3);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
RWStructuredBuffer<int> gFreeListTailIndex : register(u3);
// ★ Phase 3: このフレームの出力 aliveList B（新規発生スロットを append）
RWStructuredBuffer<uint> gAliveList : register(u4);
RWStructuredBuffer<uint> gAliveCounter : register(u5);
// Candidate A: コンパクト描画属性バッファ（新規発生スロットの描画属性を書き出す。VS が読む）。
//   indirect 版 Update は入力 A しか処理しないため、当フレーム発生分はここで書かないと
//   VS が stale な属性を読む（emit が Out に append するのと同じ理由）。
RWStructuredBuffer<ParticleDrawAttrib> gDrawAttribs : register(u6);
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

float3 LocalToWorld(float3 localPos, float3x3 rotMatrix)
{
    return mul(rotMatrix, localPos * gEmitterMesh.scale) + gEmitterMesh.translate;
}

float3 RandomPointOnTriangle(float3 v0, float3 v1, float3 v2, float u, float v)
{
    if (u + v > 1.0f)
    {
        u = 1.0f - u;
        v = 1.0f - v;
    }
    return v0 + u * (v1 - v0) + v * (v2 - v0);
}

uint SampleTriangleByCDF(float r)
{
    uint left = 0;
    uint right = gEmitterMesh.triangleCount - 1;
    while (left < right)
    {
        uint mid = (left + right) / 2;
        if (gTriangleCDF[mid] < r)
            left = mid + 1;
        else
            right = mid;
    }
    return left;
}

int CheckFieldContact(float3 worldPos)
{
    for (uint i = 0; i < gFieldCB.fieldCount; i++)
    {
        if (gFields[i].enableEmitSpawn == 0)
            continue;

        bool groupMatch = (gFields[i].groupId == -1) ||
                          (gPerFrame.emitterFieldGroupId == -1) ||
                          (gFields[i].groupId == gPerFrame.emitterFieldGroupId);
        if (!groupMatch)
            continue;

        float3 diff = worldPos - gFields[i].position;
        if (dot(diff, diff) < gFields[i].radius * gFields[i].radius)
            return (int) i;
    }
    return -1;
}

bool HasEmitSpawnField()
{
    for (uint i = 0; i < gFieldCB.fieldCount; i++)
    {
        if (gFields[i].enableEmitSpawn == 0)
            continue;
        bool groupMatch = (gFields[i].groupId == -1) ||
                          (gPerFrame.emitterFieldGroupId == -1) ||
                          (gFields[i].groupId == gPerFrame.emitterFieldGroupId);
        if (groupMatch)
            return true;
    }
    return false;
}

bool TryFieldContactEmit(inout RandomGenerator rng, float3x3 rotMatrix,
                          out float3 outPos, out int outFieldIndex)
{
    outPos = gEmitterMesh.translate;
    outFieldIndex = -1;

    if (!HasEmitSpawnField())
        return false;

    static const int kMaxRetry = 32;

    if (gEmitterMesh.triangleCount > 0)
    {
        [loop]
        for (int retry = 0; retry < kMaxRetry; retry++)
        {
            uint triIndex = SampleTriangleByCDF(rng.Generate1d());

            float3 v0 = LocalToWorld(gTriangles[triIndex].v0, rotMatrix);
            float3 v1 = LocalToWorld(gTriangles[triIndex].v1, rotMatrix);
            float3 v2 = LocalToWorld(gTriangles[triIndex].v2, rotMatrix);

            float u = rng.Generate1d();
            float v = rng.Generate1d();
            float3 candidate = RandomPointOnTriangle(v0, v1, v2, u, v);

            int fieldIdx = CheckFieldContact(candidate);
            if (fieldIdx >= 0)
            {
                outPos = candidate;
                outFieldIndex = fieldIdx;
                return true;
            }
        }
    }
    else if (gEmitterMesh.edgeCount > 0)
    {
        [loop]
        for (int retry = 0; retry < kMaxRetry; retry++)
        {
            uint edgeIndex = uint(rng.Generate1d() * float(gEmitterMesh.edgeCount)) % gEmitterMesh.edgeCount;
            float t = rng.Generate1d();
            float3 v0 = LocalToWorld(gEdges[edgeIndex].v0, rotMatrix);
            float3 v1 = LocalToWorld(gEdges[edgeIndex].v1, rotMatrix);
            float3 candidate = lerp(v0, v1, t);

            int fieldIdx = CheckFieldContact(candidate);
            if (fieldIdx >= 0)
            {
                outPos = candidate;
                outFieldIndex = fieldIdx;
                return true;
            }
        }
    }
    else
    {
        int fieldIdx = CheckFieldContact(gEmitterMesh.translate);
        if (fieldIdx >= 0)
        {
            outPos = gEmitterMesh.translate;
            outFieldIndex = fieldIdx;
            return true;
        }
    }

    return false;
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

    float3x3 rotMatrix = CreateRotationMatrixFromQuaternion(gEmitterMesh.rotation);

    float3 emitPosition;
    int hitFieldIndex = -1;

    if (gFieldCB.fieldCount > 0)
    {
        float3 fieldPos;
        int fieldIdx;
        bool success = TryFieldContactEmit(generator, rotMatrix, fieldPos, fieldIdx);

        if (!success)
        {
            int oldTail;
            InterlockedAdd(gFreeListTailIndex[0], 1, oldTail);
            int slot = oldTail % (int) gSettings.maxParticleCount;
            gFreeList[slot] = particleIndex;
            return;
        }

        emitPosition = fieldPos;
        hitFieldIndex = fieldIdx;
    }
    else
    {
        if (gEmitterMesh.triangleCount > 0 || gEmitterMesh.edgeCount > 0)
        {
            float3 randomPoint;

            if (gEmitterMesh.emitFromSurface == 2 && gEmitterMesh.edgeCount > 0)
            {
                uint edgeIndex = uint(generator.Generate1d() * float(gEmitterMesh.edgeCount)) % gEmitterMesh.edgeCount;
                float t = generator.Generate1d();
                randomPoint = lerp(gEdges[edgeIndex].v0, gEdges[edgeIndex].v1, t);
                randomPoint = mul(rotMatrix, randomPoint * gEmitterMesh.scale);
            }
            else if (gEmitterMesh.emitFromSurface == 1 && gEmitterMesh.triangleCount > 0)
            {
                uint triIndex = SampleTriangleByCDF(generator.Generate1d());
                float3 v0 = gTriangles[triIndex].v0;
                float3 v1 = gTriangles[triIndex].v1;
                float3 v2 = gTriangles[triIndex].v2;
                float u = generator.Generate1d();
                float v = generator.Generate1d();
                randomPoint = RandomPointOnTriangle(v0, v1, v2, u, v);
                randomPoint = mul(rotMatrix, randomPoint * gEmitterMesh.scale);
            }
            else
            {
                float3 r01 = float3(generator.Generate1d(), generator.Generate1d(), generator.Generate1d());
                float3 offset = (gEmitterMesh.anchorPoint - 0.5f) * 4.0f;
                randomPoint = lerp(-1.0f + offset, 1.0f + offset, r01);
                randomPoint = mul(rotMatrix, randomPoint * gEmitterMesh.scale);
            }

            emitPosition = gEmitterMesh.translate + randomPoint;
        }
        else
        {
            if (gSettings.emitShape == 1)
            {
                float3 rnd = float3(
                    generator.Generate1d() * 2.0f - 1.0f,
                    generator.Generate1d() * 2.0f - 1.0f,
                    generator.Generate1d() * 2.0f - 1.0f
                );
                float len = length(rnd);
                float3 dir = (len > 0.001f) ? rnd / len : float3(0.0f, 1.0f, 0.0f);
                float r = (gSettings.emitSphereRadius > 0.001f)
                              ? gSettings.emitSphereRadius
                              : max(max(gEmitterMesh.scale.x, gEmitterMesh.scale.y), gEmitterMesh.scale.z);
                emitPosition = gEmitterMesh.translate + mul(rotMatrix, dir * r);
            }
            else if (gSettings.emitShape == 2)
            {
                float theta = generator.Generate1d() * 6.28318530f;
                float cosMax = cos(gSettings.emitConeAngle);
                float cosAngle = lerp(cosMax, 1.0f, generator.Generate1d());
                float sinAngle = sqrt(max(0.0f, 1.0f - cosAngle * cosAngle));
                float3 dir = float3(sinAngle * cos(theta), cosAngle, sinAngle * sin(theta));
                float r = (gSettings.emitSphereRadius > 0.001f)
                              ? gSettings.emitSphereRadius
                              : max(max(gEmitterMesh.scale.x, gEmitterMesh.scale.y), gEmitterMesh.scale.z);
                emitPosition = gEmitterMesh.translate + mul(rotMatrix, dir * r);
            }
            else
            {
                float3 r01 = float3(generator.Generate1d(), generator.Generate1d(), generator.Generate1d());
                emitPosition = gEmitterMesh.translate + mul(rotMatrix, (r01 * 2.0f - 1.0f) * gEmitterMesh.scale);
            }
        }
    }

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
        float3 radialDir = normalize(emitPosition - gSettings.radialVelocityCenter);
        if (gSettings.radialVelocityRandomness > 0.0f)
        {
            float3 rndOfs = (float3(generator.Generate1d(), generator.Generate1d(), generator.Generate1d()) * 2.0f - 1.0f)
                            * gSettings.radialVelocityRandomness;
            radialDir = normalize(radialDir + rndOfs);
        }
        float speed = lerp(gSettings.velocityMin.x, gSettings.velocityMax.x, generator.Generate1d());
        vel = radialDir * speed * gSettings.radialVelocityStrength;
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
    gParticles[particleIndex].endScale = gSettings.endScaleValue;

    if (gSettings.enableRandomRotation)
    {
        gParticles[particleIndex].rotation = float3(
            lerp(gSettings.rotationMin.x, gSettings.rotationMax.x, generator.Generate1d()),
            lerp(gSettings.rotationMin.y, gSettings.rotationMax.y, generator.Generate1d()),
            lerp(gSettings.rotationMin.z, gSettings.rotationMax.z, generator.Generate1d())
        );
    }
    else
        gParticles[particleIndex].rotation = float3(0, 0, 0);

    if (gSettings.enableRandomAngularVelocity)
    {
        gParticles[particleIndex].angularVelocity = float3(
            lerp(gSettings.angularVelocityMin.x, gSettings.angularVelocityMax.x, generator.Generate1d()),
            lerp(gSettings.angularVelocityMin.y, gSettings.angularVelocityMax.y, generator.Generate1d()),
            lerp(gSettings.angularVelocityMin.z, gSettings.angularVelocityMax.z, generator.Generate1d())
        );
    }
    else
        gParticles[particleIndex].angularVelocity = float3(0, 0, 0);

    // ★ Phase 3: 発生に成功したスロットを当フレームの出力 aliveList B に append。
    //   入力 A には含まれないため Update と二重 append にはならない。
    uint dstIndex;
    InterlockedAdd(gAliveCounter[0], 1, dstIndex);
    gAliveList[dstIndex] = particleIndex;

    // Candidate A: 当フレーム発生分の描画属性をコンパクトバッファへ書き出す（VS が読む）。
    //   rotation/color は直前に gParticles へ書いた値を読み戻す（同一スレッド・ハザードなし）。
    if (gSettings.enableCompactDraw != 0)
    {
        ParticleDrawAttrib attrib;
        attrib.translate = emitPosition;
        attrib.scale = float3(scaleValue, scaleValue, scaleValue);
        attrib.velocity = vel;
        attrib.rotation = gParticles[particleIndex].rotation;
        attrib.color = gParticles[particleIndex].color;
        gDrawAttribs[particleIndex] = attrib;
    }
}

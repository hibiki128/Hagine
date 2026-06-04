// =============================================
// UpdateParticleIndirect.CS  (Phase 3: ping-pong + DispatchIndirect)
//
//   UpdateParticle.CS の派生版。物理更新ロジックは完全に同一だが、
//   入出力モデルだけが異なる:
//     - 入力 : aliveListIn(A) ＝ 前フレームの生存スロット列。
//              本シェーダは A[tid] のスロットだけを処理する
//              （= 生存数ぶんだけ DispatchIndirect される）。
//     - 出力 : aliveList(B) ＝ このフレームの生存スロット＋新規トレイル子を
//              InterlockedAdd で詰める（first-class トレイル append）。
//
//   旧 UpdateParticle.CS は非改変のまま残し、フラグ OFF 時は従来パスが動く。
//   本シェーダはフラグ ON 時のみ PSO がコンパイル/起動される。
//
//   ※ 入力 A には生存スロットのみが入る前提だが、安全のため
//      lifeTime<=0 を二重チェックして弾く。
// =============================================
#include "../Particle.hlsli"
#include "../../Random/Random.hlsli"
#include "CurlNoise.hlsli"

ConstantBuffer<PerFrame> gPerFrame : register(b0);
ConstantBuffer<ParticleCSSettings> gSettings : register(b1);
ConstantBuffer<FieldCountCB> gFieldCB : register(b2);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
RWStructuredBuffer<int> gFreeListTailIndex : register(u3);
// 出力 aliveList B: このフレームの生存スロット＋新規トレイル子を詰める
RWStructuredBuffer<uint> gAliveList : register(u4);
RWStructuredBuffer<uint> gAliveCounter : register(u5);
StructuredBuffer<ParticleField> gFields : register(t0);
StructuredBuffer<ParticleFieldSettingsOverrideData> gFieldsOverride : register(t1);
// 入力 aliveList A: 前フレームの生存スロット列（このシェーダの処理対象）。
// A/B とも UAV(UNORDERED_ACCESS) 状態で作られるため、SRV で読むと状態不一致になる。
// 読み取り専用だが RWStructuredBuffer(UAV) で束ねることで状態遷移を一切不要にする。
RWStructuredBuffer<uint> gAliveListIn : register(u6);
RWStructuredBuffer<uint> gAliveCounterIn : register(u7);
// Phase 5: グループ単位トレイル予算カウンタ（このフレームに生成したトレイル本数）。
// 毎フレーム 0 にリセットされ、gSettings.maxTrailBudgetPerGroup を上限にする。
RWStructuredBuffer<int> gTrailBudget : register(u8);
// Candidate A: コンパクト描画属性バッファ（slot index で書き込む。VS が読む）。
RWStructuredBuffer<ParticleDrawAttrib> gDrawAttribs : register(u9);

// =============================================
// フィールド適用結果
// =============================================
struct FieldEffectResult
{
    float3 velocity;
    float lifeTimeDrain;
    uint forceTrail;
    float trailDistOverride;
    float4 colorMultiplier;
};

// =============================================
// フィールド適用関数（UpdateParticle.CS と同一）
// =============================================
FieldEffectResult ApplyFields(float3 velocity, float3 particlePos, float deltaTime)
{
    FieldEffectResult result;
    result.velocity = velocity;
    result.lifeTimeDrain = 0.0f;
    result.forceTrail = 0;
    result.trailDistOverride = 0.0f;
    result.colorMultiplier = float4(1, 1, 1, 1);

    for (uint fi = 0; fi < gFieldCB.fieldCount; fi++)
    {
        ParticleField f = gFields[fi];

        bool groupMatch = (f.groupId == -1) ||
                          (gPerFrame.emitterFieldGroupId == -1) ||
                          (f.groupId == gPerFrame.emitterFieldGroupId);
        if (!groupMatch)
            continue;

        float3 toParticle = particlePos - f.position;
        float distSq = dot(toParticle, toParticle);
        float radiusSq = f.radius * f.radius;

        if (distSq >= radiusSq)
            continue;

        float dist = sqrt(distSq);
        float t = 1.0f - saturate(dist / f.radius);
        float influence = (f.falloff == 1.0f) ? t : t * t;

        if (f.fieldType == 0) // Wind
        {
            result.velocity += f.direction * f.strength * influence * deltaTime;
        }
        else if (f.fieldType == 1) // Attract
        {
            if (dist > 0.001f)
            {
                float3 dir = -toParticle / dist;
                result.velocity += dir * f.strength * influence * deltaTime;
            }
        }
        else if (f.fieldType == 2) // Repel
        {
            if (dist > 0.001f)
            {
                float3 dir = toParticle / dist;
                result.velocity += dir * f.strength * influence * deltaTime;
            }
        }
        else if (f.fieldType == 3) // Vortex
        {
            if (dist > 0.001f)
            {
                float3 tangent = cross(toParticle / dist, f.direction);
                result.velocity += tangent * f.strength * influence * deltaTime;
            }
        }

        if (f.enableLifeDrain != 0)
        {
            result.lifeTimeDrain += f.lifeTimeDrain * influence * deltaTime;
        }

        if (f.enableForceTrail != 0)
        {
            result.forceTrail = 1;
            if (f.trailSpawnDistanceOverride > 0.0f)
            {
                if (result.trailDistOverride <= 0.0f)
                    result.trailDistOverride = f.trailSpawnDistanceOverride;
                else
                    result.trailDistOverride = min(result.trailDistOverride, f.trailSpawnDistanceOverride);
            }
        }

        if (f.enableColorMultiply != 0)
        {
            float4 blendedColor = lerp(float4(1, 1, 1, 1), f.colorMultiplier, influence);
            result.colorMultiplier *= blendedColor;
        }
    }

    return result;
}

// =============================================
// 一度きり設定上書き処理（UpdateParticle.CS と同一）
// =============================================
void ApplySettingsOverride(inout Particle p, uint fi)
{
    ParticleFieldSettingsOverrideData ov = gFieldsOverride[fi];
    if (ov.overrideMask.x == 0u && ov.overrideMask.y == 0u)
        return;

    uint2 pFlags = p.settingsOverrideFlags;
    uint2 pending;
    pending.x = ov.overrideMask.x & ~pFlags.x;
    pending.y = ov.overrideMask.y & ~pFlags.y;

    if (pending.x == 0u && pending.y == 0u)
        return;

    if (pending.x & (1u << OB_LifeTimeMin))
        p.lifeTime = ov.lifeTimeMin;

    if (pending.x & (1u << OB_LifeTimeMax))
        p.lifeTime = ov.lifeTimeMax;

    if (pending.x & (1u << OB_ScaleMin))
    {
        float3 newScale = float3(ov.scaleMin, ov.scaleMin, ov.scaleMin);
        p.initialScale = newScale;
        p.scale = newScale;
    }

    if (pending.x & (1u << OB_ScaleMax))
    {
        float3 newScale = float3(ov.scaleMax, ov.scaleMax, ov.scaleMax);
        p.initialScale = newScale;
        p.scale = newScale;
    }

    if (pending.x & (1u << OB_VelocityMin))
        p.velocity = max(p.velocity, ov.velocityMin);

    if (pending.x & (1u << OB_VelocityMax))
        p.velocity = min(p.velocity, ov.velocityMax);

    if (pending.x & (1u << OB_StartColor))
        p.color = ov.startColor;

    if (pending.x & (1u << OB_EndColor))
        p.color = ov.endColor;

    if (pending.x & (1u << OB_TrailSpawnDistance))
    {
        p.trailSpawnDistance = ov.trailSpawnDistance;
    }

    if (pending.x & (1u << OB_GatherTarget))
    {
        float3 toTarget = ov.gatherTarget - p.translate;
        float dist = length(toTarget);
        if (dist > 0.01f)
            p.velocity = normalize(toTarget) * length(p.velocity);
    }

    if (pending.y & (1u << (OB_Acceleration - 32u)))
    {
        p.velocity += ov.acceleration;
    }

    if (pending.y & (1u << (OB_VelocityDampingFactor - 32u)))
    {
        p.velocity *= ov.velocityDampingFactor;
    }

    p.settingsOverrideFlags.x |= pending.x;
    p.settingsOverrideFlags.y |= pending.y;
}

// =============================================
// トレイル生成（UpdateParticle.CS と同一だが、生成した子スロットを
//   このフレームの出力 aliveList B にも append する（first-class トレイル）。
//   入力 A には子は含まれないため、同フレーム append でも二重処理は起きない。
// =============================================
void SpawnTrailParticles(inout Particle p, int particleIndex, float3 currentPosition)
{
    float parentLifeTime = p.lifeTime;
    float parentCurrentTime = p.currentTime;
    float parentRemainingLife = max(0.0f, parentLifeTime - parentCurrentTime);

    if (parentRemainingLife < gSettings.trailMinLifeTime * 0.2f)
        return;

    float3 lastPos = p.lastTrailPosition;
    float targetDistance = p.trailSpawnDistance;

    if (targetDistance <= 0.001f)
        return;

    float3 moveVector = currentPosition - lastPos;
    float totalDistance = length(moveVector);

    if (totalDistance < targetDistance)
        return;

    int desiredTrails = int(totalDistance / targetDistance);
    int trailsToSpawn = min(desiredTrails, gSettings.maxTrailPerParticle);

    if (trailsToSpawn <= 0)
        return;

    // --- Phase 5: グループ単位トレイル予算 ---
    // freeList 予約より前に予算を原子的に確保し、超過分は本数を切り詰める。
    // freeList 予約より前に行うのは、予算切れ時に freeList を無駄に消費しないため。
    if (gSettings.enableTrailBudget != 0)
    {
        int budgetMax = (int) gSettings.maxTrailBudgetPerGroup;
        int budgetPrev;
        InterlockedAdd(gTrailBudget[0], trailsToSpawn, budgetPrev);
        if (budgetPrev >= budgetMax)
        {
            // 予算超過: 予約分を返して何も生成しない。
            int dummyBudget;
            InterlockedAdd(gTrailBudget[0], -trailsToSpawn, dummyBudget);
            return;
        }
        int budgetRemaining = budgetMax - budgetPrev;
        if (trailsToSpawn > budgetRemaining)
        {
            // 余剰分を返して残予算ぶんに切り詰める。
            int giveBackBudget;
            InterlockedAdd(gTrailBudget[0], -(trailsToSpawn - budgetRemaining), giveBackBudget);
            trailsToSpawn = budgetRemaining;
        }
    }

    uint capacity = gSettings.maxParticleCount;
    uint originalHead;
    InterlockedAdd(gFreeListIndex[0], trailsToSpawn, originalHead);
    uint tail = gFreeListTailIndex[0];
    int available = (int) (tail - originalHead);

    if (available <= 0)
    {
        int dummyRollback;
        InterlockedAdd(gFreeListIndex[0], -trailsToSpawn, dummyRollback);
        return;
    }
    if (available < trailsToSpawn)
    {
        int giveBack;
        InterlockedAdd(gFreeListIndex[0], -(trailsToSpawn - available), giveBack);
        trailsToSpawn = available;
    }

    bool capped = (desiredTrails > trailsToSpawn);
    float spacing = capped ? (totalDistance / float(trailsToSpawn)) : targetDistance;

    float3 direction = normalize(moveVector);
    int requiredCount = trailsToSpawn;

    float3 parentCurrentScale = p.scale;
    float3 parentVelocity = p.velocity;
    float4 parentColor = p.color;

    float desiredTrailLife = parentRemainingLife * gSettings.trailLifeTimeScale;
    float trailLifeTime = max(desiredTrailLife, gSettings.trailMinLifeTime);
    trailLifeTime = clamp(trailLifeTime, gSettings.trailMinLifeTime, 10.0f);

    for (int i = 0; i < requiredCount; ++i)
    {
        int slot = (originalHead + i) % capacity;
        int trailIndex = gFreeList[slot];

        if (trailIndex < 0 || trailIndex >= gSettings.maxParticleCount)
            continue;

        float spawnDistance = spacing * (float(i) + 1.0f);
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

        // first-class トレイル: 生成した子をこのフレームの出力リスト B に append。
        // 入力 A には含まれないため二重処理は起きず、同フレームで描画対象になる。
        uint dstTrail;
        InterlockedAdd(gAliveCounter[0], 1, dstTrail);
        gAliveList[dstTrail] = (uint) trailIndex;

        // Candidate A: 同フレーム描画のため、生成したトレイルの描画属性も書き出す。
        // トレイルは回転を設定しないため rotation は 0（回転グループでも次フレームに自スレッドが更新）。
        if (gSettings.enableCompactDraw != 0)
        {
            ParticleDrawAttrib trailAttrib;
            trailAttrib.translate = spawnPosition;
            trailAttrib.scale = gParticles[trailIndex].scale;
            trailAttrib.velocity = gParticles[trailIndex].velocity;
            trailAttrib.rotation = float3(0.0f, 0.0f, 0.0f);
            trailAttrib.color = gParticles[trailIndex].color;
            gDrawAttribs[trailIndex] = trailAttrib;
        }
    }

    float consumedDistance = capped ? totalDistance : (float(requiredCount) * targetDistance);
    p.lastTrailPosition = lastPos + direction * consumedDistance;
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint tid = DTid.x;
    // DispatchIndirect は groupX = ceil(countA/1024) で起動されるため、
    // 末尾グループには countA を超える tid が含まれる。ここで弾く。
    if (tid >= gAliveCounterIn[0])
        return;

    int particleIndex = (int) gAliveListIn[tid];
    if (particleIndex < 0 || particleIndex >= (int) gSettings.maxParticleCount)
        return;

    Particle p = gParticles[particleIndex];

    // 入力 A は生存スロットのみの想定だが、安全のため二重チェック。
    if (p.lifeTime <= 0.0f)
        return;

    // 1. 加速度処理
    if (gSettings.enableAcceleration)
    {
        p.velocity += gSettings.acceleration * gPerFrame.deltaTime;
    }

    // 2. 重力処理
    if (gSettings.enableGravity)
    {
        p.velocity += gSettings.gravity * gPerFrame.deltaTime;
    }

    // 3. 速度減衰処理
    if (gSettings.enableVelocityDamping)
    {
        p.velocity *= pow(gSettings.velocityDampingFactor, gPerFrame.deltaTime * 60.0f);
    }

    // 4. ライフタイムに応じた速度減衰
    if (gSettings.enableLifetimeVelocityDamping)
    {
        float lifeRatio = p.currentTime / p.lifeTime;

        if (lifeRatio >= gSettings.lifetimeVelocityDampingStart)
        {
            float dampingProgress = (lifeRatio - gSettings.lifetimeVelocityDampingStart) /
                                       (1.0f - gSettings.lifetimeVelocityDampingStart);
            float dampingMultiplier = 1.0f - (dampingProgress * dampingProgress);
            p.velocity *= dampingMultiplier;
        }
    }

    // 5. ギャザー処理
    if (gSettings.enableGather)
    {
        bool isTrail = (p.isTrailParticle != 0);

        if (!isTrail || gSettings.enableGatherForTrail)
        {
            float lifeRatio = p.currentTime / p.lifeTime;

            if (lifeRatio >= gSettings.gatherStartRatio)
            {
                float3 targetPosition = gSettings.gatherTarget;
                float3 toTarget = targetPosition - p.translate;
                float distance = length(toTarget);

                if (distance > 0.01f)
                {
                    float3 dirToTarget = normalize(toTarget);
                    float t = (lifeRatio - gSettings.gatherStartRatio) / (1.0f - gSettings.gatherStartRatio);
                    t = t * t;

                    float3 desiredVelocity = dirToTarget * gSettings.gatherStrength;
                    float lerpFactor = 5.0f * gPerFrame.deltaTime;

                    p.velocity = lerp(p.velocity, desiredVelocity, lerpFactor * t * 10.0f);
                }
            }
        }
    }

    // 6. 渦巻き（Vortex）処理
    if (gSettings.enableVortex)
    {
        bool isTrail = (p.isTrailParticle != 0);

        if (!isTrail || gSettings.enableVortexForTrail)
        {
            float3 center = gSettings.vortexTarget;
            float3 toParticle = p.translate - center;
            float dist = length(toParticle);

            if (dist > 0.05f)
            {
                float3 axis = gSettings.vortexAxis;
                if (length(axis) < 0.001f)
                    axis = float3(0, 1, 0);
                else
                    axis = normalize(axis);

                float3 tangent = cross(normalize(toParticle), axis);
                p.velocity += tangent * gSettings.vortexStrength * gPerFrame.deltaTime;
            }
        }
    }

    // 7. タービュランス（per-particle ランダム振動力）
    if (gSettings.enableTurbulence)
    {
        float fi = float(particleIndex);
        float px = frac(sin(fi * 127.1f) * 43758.5f) * 6.28318f;
        float py = frac(sin(fi * 311.7f) * 43758.5f) * 6.28318f;
        float pz = frac(sin(fi * 74.7f) * 43758.5f) * 6.28318f;
        float t = gPerFrame.time * gSettings.turbulenceFrequency;
        float3 turbForce = float3(
            sin(t + px),
            cos(t + py),
            sin(t + pz + 1.0471f)
        ) * gSettings.turbulenceStrength;
        p.velocity += turbForce * gPerFrame.deltaTime;
    }

    // 8. Curl Noise による速度場
    if (gSettings.enableCurlNoise)
    {
        bool isTrail = (p.isTrailParticle != 0);
        if (!isTrail)
        {
            float3 pos = p.translate;

            if (gSettings.curlNoisePosRandomStrength > 0.0f)
            {
                float fi = float(particleIndex);
                float3 idOffset = float3(
                        frac(sin(fi * 127.1f) * 43758.5f),
                        frac(sin(fi * 311.7f) * 43758.5f),
                        frac(sin(fi * 74.7f) * 43758.5f)
                    ) * 2.0f - 1.0f;

                pos += idOffset * gSettings.curlNoisePosRandomStrength;
            }

            float3 curlVel = ComputeCurlNoise(
                    pos,
                    gSettings.curlNoiseScale,
                    gPerFrame.time * gSettings.curlNoiseTimeScale,
                    (int) gSettings.curlNoiseOctaves
                ) * gSettings.curlNoiseStrength;

            if (gSettings.curlNoiseAttractStrength > 0.0f)
            {
                float3 attractTarget = gSettings.curlNoiseAttractCenter;
                float3 toCenter = attractTarget - p.translate;
                float dist = length(toCenter);
                if (dist > 0.001f)
                {
                    float3 attractVel = normalize(toCenter) * dist * gSettings.curlNoiseAttractStrength;
                    curlVel += attractVel;
                }
            }

            if (gSettings.curlNoiseBlendMode == 0)
            {
                p.velocity = curlVel;
            }
            else
            {
                p.velocity += curlVel * gPerFrame.deltaTime;
            }
        }
    }

    // 7.5. フィールド処理
    uint fieldForceTrail = 0;
    float fieldTrailDistOverride = 0.0f;
    float4 fieldColorMultiplier = float4(1, 1, 1, 1);
    if (gFieldCB.fieldCount > 0)
    {
        FieldEffectResult fieldResult = ApplyFields(
                p.velocity,
                p.translate,
                gPerFrame.deltaTime);

        p.velocity = fieldResult.velocity;

        if (fieldResult.lifeTimeDrain > 0.0f)
        {
            p.currentTime =
                    min(p.currentTime + fieldResult.lifeTimeDrain,
                        p.lifeTime);
        }

        fieldColorMultiplier = fieldResult.colorMultiplier;
        fieldForceTrail = fieldResult.forceTrail;
        fieldTrailDistOverride = fieldResult.trailDistOverride;

        for (uint fi = 0; fi < gFieldCB.fieldCount; fi++)
        {
            if (gFields[fi].enableSettingsOverride == 0u)
                continue;

            float3 toP = p.translate - gFields[fi].position;
            if (dot(toP, toP) >= gFields[fi].radius * gFields[fi].radius)
                continue;

            ApplySettingsOverride(p, fi);
        }
    }

    // 9. 移動更新
    p.translate += p.velocity * gPerFrame.deltaTime;
    p.currentTime += gPerFrame.deltaTime;
    float3 currentPosition = p.translate;

    // 10. 各種パラメータ更新
    float lifeRatio = p.currentTime / p.lifeTime;

    if (!gSettings.enableRandomColor)
    {
        float4 lerpedColor;
        if (gSettings.enableMidColor)
        {
            float r = saturate(gSettings.midColorRatio);
            if (lifeRatio < r)
            {
                float t = (r > 0.001f) ? (lifeRatio / r) : 0.0f;
                lerpedColor = lerp(gSettings.startColor, gSettings.midColor, t);
            }
            else
            {
                float span = 1.0f - r;
                float t = (span > 0.001f) ? ((lifeRatio - r) / span) : 1.0f;
                lerpedColor = lerp(gSettings.midColor, gSettings.endColor, t);
            }
        }
        else
        {
            lerpedColor = lerp(gSettings.startColor, gSettings.endColor, lifeRatio);
        }
        p.color = float4(lerpedColor.rgb, saturate(lerpedColor.a));
    }
    else
    {
        p.color.a = saturate(lerp(gSettings.startColor.a, gSettings.endColor.a, lifeRatio));
    }

    {
        float lifetimeMul = gSettings.enableLifetimeScale ? (1.0f - lifeRatio) : 1.0f;
        float sinMul = 1.0f;
        if (gSettings.enableSinScale)
        {
            float sinWave = sin(p.currentTime * gSettings.sinScaleFrequency) * 0.5f + 0.5f;
            sinMul = 1.0f + (sinWave * 2.0f - 1.0f) * gSettings.sinScaleAmplitude;
        }
        if (gSettings.enableEndScale)
        {
            p.scale = lerp(p.initialScale, p.endScale, lifeRatio) * sinMul;
        }
        else if (gSettings.enableLifetimeScale || gSettings.enableSinScale)
        {
            p.scale = p.initialScale * lifetimeMul * sinMul;
        }
    }

    // 回転更新
    p.rotation += p.angularVelocity * gPerFrame.deltaTime;

    // --- フィールドによるカラー乗算 ---
    if (fieldColorMultiplier.r != 1.0f ||
            fieldColorMultiplier.g != 1.0f ||
            fieldColorMultiplier.b != 1.0f ||
            fieldColorMultiplier.a != 1.0f)
    {
        float savedAlpha = p.color.a;
        p.color *= fieldColorMultiplier;
        p.color.a = savedAlpha * fieldColorMultiplier.a;
    }

    // 11. トレイル生成
    bool doTrail = (gSettings.enableTrail != 0) || (fieldForceTrail != 0);
    if (doTrail &&
            p.isTrailParticle == 0 &&
            p.color.a > 0.05f)
    {
        float savedDist = p.trailSpawnDistance;
        if (fieldForceTrail != 0 && fieldTrailDistOverride > 0.0f)
        {
            p.trailSpawnDistance = fieldTrailDistOverride;
        }
        else if (fieldForceTrail != 0 && p.trailSpawnDistance <= 0.0f)
        {
            p.trailSpawnDistance =
                    (gSettings.trailSpawnDistance > 0.0f) ? gSettings.trailSpawnDistance : 0.3f;
        }

        SpawnTrailParticles(p, particleIndex, currentPosition);

        if (fieldForceTrail != 0 && gSettings.enableTrail == 0)
        {
            p.trailSpawnDistance = savedDist;
        }
    }

    // 死亡判定
    if (p.currentTime >= p.lifeTime)
    {
        p.scale = float3(0.0f, 0.0f, 0.0f);
        p.lastTrailPosition = float3(0.0f, 0.0f, 0.0f);
        p.lifeTime = 0.0f;

        int oldTail;
        InterlockedAdd(gFreeListTailIndex[0], 1, oldTail);

        int slot = oldTail % gSettings.maxParticleCount;
        gFreeList[slot] = particleIndex;
    }

    gParticles[particleIndex] = p;

    // =============================================
    // 生存コンパクション（出力 B へ append）
    //   入力 A の生存スロットのうち、今フレームも生存しているものを
    //   B に詰める。トレイル子は SpawnTrailParticles 内で既に append 済み。
    // =============================================
    if (IsAliveParticle(p))
    {
        uint dstIndex;
        InterlockedAdd(gAliveCounter[0], 1, dstIndex);
        gAliveList[dstIndex] = (uint) particleIndex;

        // Candidate A: 生存スロットの描画属性をコンパクトバッファへ書き出す（VS が 64B で読む）。
        if (gSettings.enableCompactDraw != 0)
        {
            ParticleDrawAttrib attrib;
            attrib.translate = p.translate;
            attrib.scale = p.scale;
            attrib.velocity = p.velocity;
            attrib.rotation = p.rotation;
            attrib.color = p.color;
            gDrawAttribs[particleIndex] = attrib;
        }
    }
}

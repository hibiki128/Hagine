#include"Particle.hlsli"
#include"../Random/Random.hlsli"

ConstantBuffer<EmitterMesh> gEmitterMesh : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);
ConstantBuffer<ParticleCSSettings> gSettings : register(b2);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
StructuredBuffer<SurfacePoint> gSurfacePoints : register(t0);

// 3x3回転行列を作成する関数
float3x3 CreateRotationMatrix(float3 rotation)
{
    float cosX = cos(rotation.x);
    float sinX = sin(rotation.x);
    float cosY = cos(rotation.y);
    float sinY = sin(rotation.y);
    float cosZ = cos(rotation.z);
    float sinZ = sin(rotation.z);
    
    // X軸回転行列
    float3x3 rotX = float3x3(
        1.0f, 0.0f, 0.0f,
        0.0f, cosX, -sinX,
        0.0f, sinX, cosX
    );
    
    // Y軸回転行列
    float3x3 rotY = float3x3(
        cosY, 0.0f, sinY,
        0.0f, 1.0f, 0.0f,
        -sinY, 0.0f, cosY
    );
    
    // Z軸回転行列
    float3x3 rotZ = float3x3(
        cosZ, -sinZ, 0.0f,
        sinZ, cosZ, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    
    return mul(mul(rotZ, rotY), rotX);
}

// スケール行列を適用する関数
float3 ApplyScale(float3 vertex, float3 scale)
{
    return vertex * scale;
}

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (gEmitterMesh.emit == 0)
    {
        return;
    }
    
    if (DTid.x >= gSettings.emitCount)
    {
        return;
    }
    
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

        // ランダムにポイントを選択
        uint pointIndex = uint(generator.Generate1d() * gEmitterMesh.triangleCount) % gEmitterMesh.triangleCount;
        float3 randomPoint = gSurfacePoints[pointIndex].position;

        // スケールを適用
        randomPoint = ApplyScale(randomPoint, gEmitterMesh.scale);

        // 回転を適用
        float3x3 rotMatrix = CreateRotationMatrix(gEmitterMesh.rotation);
        randomPoint = mul(rotMatrix, randomPoint);

        emitPosition = gEmitterMesh.translate + randomPoint;

        gParticles[particleIndex].translate = emitPosition;
        
        // 色、速度、寿命設定は既存のコードと同じ
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
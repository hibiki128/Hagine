#include"Particle.hlsli"
#include"../Random/Random.hlsli"

ConstantBuffer<EmitterSettings> gEmitterSettings : register(b0);
ConstantBuffer<EmitterSphere> gEmitterSphere : register(b1);
ConstantBuffer<EmitterMesh> gEmitterMesh : register(b2);
ConstantBuffer<PerFrame> gPerFrame : register(b3);
ConstantBuffer<ParticleCSSettings> gSettings : register(b4);
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
StructuredBuffer<Triangle> gTriangles : register(t0);

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
        if (gEmitterSettings.isSphere)
        {
            // 球体の処理（楕円体に対応）
            float3 rawDirection = generator.Generate3d() * 2.0f - 1.0f;
            if (length(rawDirection) < 0.001f)
            {
                rawDirection = float3(0.577f, 0.577f, 0.577f);
            }
            float3 randomDirection = normalize(rawDirection);
            
            // 楕円体の半径を適用
            float3 ellipsoidRadius = gEmitterSphere.radius * gEmitterSphere.scale;
            
            // 楕円体内部のランダムな点を生成
            float r1 = pow(generator.Generate1d(), 1.0f / 3.0f);
            float3 randomPoint = randomDirection * r1;
            
            // 楕円体の形状を適用
            randomPoint *= ellipsoidRadius;
            
            // 回転を適用
            float3x3 rotMatrix = CreateRotationMatrix(gEmitterSphere.rotation);
            randomPoint = mul(rotMatrix, randomPoint);
            
            emitPosition = gEmitterSphere.translate + randomPoint;
        }
        else
        {
            // メッシュの処理
            uint triangleIndex = uint(generator.Generate1d() * gEmitterMesh.triangleCount) % gEmitterMesh.triangleCount;
            Triangle tri = gTriangles[triangleIndex];
            
            float r1 = generator.Generate1d();
            float r2 = generator.Generate1d();
            if (r1 + r2 > 1.0f)
            {
                r1 = 1.0f - r1;
                r2 = 1.0f - r2;
            }
            float r3 = 1.0f - r1 - r2;
            
            float3 randomPoint = r1 * tri.v0 + r2 * tri.v1 + r3 * tri.v2;
            
            // スケールを適用
            randomPoint = ApplyScale(randomPoint, gEmitterMesh.scale);
            
            // 回転を適用
            float3x3 rotMatrix = CreateRotationMatrix(gEmitterMesh.rotation);
            randomPoint = mul(rotMatrix, randomPoint);
            
            emitPosition = gEmitterMesh.translate + randomPoint;
        }
        
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
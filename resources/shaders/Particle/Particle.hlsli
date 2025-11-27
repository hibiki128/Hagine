// Particle.hlsli

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct Particle
{
    float3 translate;
    float3 scale;
    float lifeTime;
    float3 velocity;
    float currentTime;
    float4 color;
    float3 initialScale;
    float padding;
    uint isTrailParticle;
    uint parentIndex;
    float trailSpawnTimer;
    float trailSpawnInterval;
};

Particle CreateEmptyParticle()
{
    Particle p;
    p.translate = float3(0, 0, 0);
    p.scale = float3(0, 0, 0);
    p.lifeTime = 0.0f;
    p.velocity = float3(0, 0, 0);
    p.currentTime = 0.0f;
    p.color = float4(0, 0, 0, 0);
    p.initialScale = float3(0, 0, 0);
    p.padding = 0.0f;
    p.isTrailParticle = 0;
    p.parentIndex = 0xFFFFFFFF;
    p.trailSpawnTimer = 0.0f;
    p.trailSpawnInterval = 0.0f;
    return p;
}

struct PerView
{
    float4x4 viewProjection;
    float4x4 billboardMatrix;
};

struct EmitterMesh
{
    float3 translate;
    uint triangleCount;
    float4 rotation;
    uint emitFromSurface;
    float3 scale;
    float frequency;
    float frequencyTime;
    uint emit;
    uint edgeCount;
    float3 anchorPoint;
};

struct PerFrame
{
    float time;
    float deltaTime;
    int groupId;
};

struct ParticleCSSettings
{
    float lifeTimeMin;
    float lifeTimeMax;
    float scaleMin;
    float scaleMax;
    float3 velocityMin;
    float padding1;
    float3 velocityMax;
    float padding2;
    float4 startColor;
    float4 endColor;
    int enableLifetimeScale;
    int enableRandomColor;
    int enableSinScale;
    int emitCount;
    int maxParticleCount;
    float sinScaleFrequency;
    float sinScaleAmplitude;
    int enableGravity;
    float3 gravity;
    int enableTrail;
    float trailSpawnInterval;
    int maxTrailPerParticle;
    float trailLifeTimeScale;
    float3 trailScaleMultiplier;
    float padding3;
    float4 trailColorMultiplier;
    float trailVelocityScale;
    int trailInheritVelocity;
    float padding4[2];
};

struct EdgeInfo
{
    float3 v0;
    float padding0;
    float3 v1;
    float padding1;
};

struct TriangleInfo
{
    float3 v0;
    float padding0;
    float3 v1;
    float padding1;
    float3 v2;
    float padding2;
};
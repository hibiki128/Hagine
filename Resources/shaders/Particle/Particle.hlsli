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
    float3 lastTrailPosition;
    float trailSpawnDistance;
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
    uint enableLifetimeScale;
    uint enableRandomColor;
    uint enableSinScale;
    uint emitCount;
    uint maxParticleCount;
    float sinScaleFrequency;
    float sinScaleAmplitude;
    uint enableGravity;
    float3 gravity;
    uint enableTrail;
    float trailSpawnDistance;
    uint maxTrailPerParticle;
    float trailLifeTimeScale;
    float paddingTrail;
    float3 trailScaleMultiplier;
    float padding3;
    float4 trailColorMultiplier;
    float trailVelocityScale;
    uint trailInheritVelocity;
    float trailMinLifeTime;
    float padding4;
    uint enableGather;
    float gatherStartRatio;
    float gatherStrength;
    float padding5;
    float3 gatherTarget;
    float padding6;
    float3 gatherTargetOffset;
    uint enableGatherForTrail;
    uint enableVortex;
    float3 vortexTarget;
    float3 vortexTargetOffset;
    float vortexStrength;
    uint enableVortexForTrail;
    float3 vortexAxis;
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
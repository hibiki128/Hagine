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
};

struct PerView
{
    float4x4 viewProjection;
    float4x4 billboardMatrix;
};

struct EmitterSphere
{
    float3 translate;
    float padding1;
    float3 radius;
    float frequency;
    float frequencyTime;
    int emit;
    float3 rotation;
    float padding2;
    float3 scale;  
    float padding3;
};

struct EmitterMesh
{
    float3 translate;
    uint triangleCount;
    float3 rotation;
    float padding1;
    float3 scale;
    float frequency;
    float frequencyTime;
    uint emit;
    float padding2;
};

struct EmitterSettings
{
    uint isSphere;
    float3 padding;
};

struct Triangle
{
    float3 v0;
    float3 v1;
    float3 v2;
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
    int emitCount;
    int maxParticleCount;
    float padding3;
};
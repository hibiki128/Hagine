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
};

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

struct EdgeInfo
{
    float3 v0;
    float padding0;
    float3 v1;
    float padding1;
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
    float padding3;
};

struct SurfacePoint
{
    float3 position;
    float padding;
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

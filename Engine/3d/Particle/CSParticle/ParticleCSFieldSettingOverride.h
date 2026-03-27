#pragma once
#include "type/Vector3.h"
#include "type/Vector4.h"
#include <cstdint>

/// =====================================================================
/// ParticleCSSettings 上書き用ビット定数
/// フィールド側の overrideMask / パーティクル側の settingsOverrideFlags
/// で共通使用する。
/// uint64_t なので最大64項目まで対応。
/// =====================================================================
namespace ParticleSettingsOverrideBits {
static constexpr uint64_t LifeTimeMin = 1ULL << 0;
static constexpr uint64_t LifeTimeMax = 1ULL << 1;
static constexpr uint64_t ScaleMin = 1ULL << 2;
static constexpr uint64_t ScaleMax = 1ULL << 3;
static constexpr uint64_t VelocityMin = 1ULL << 4;
static constexpr uint64_t VelocityMax = 1ULL << 5;
static constexpr uint64_t StartColor = 1ULL << 6;
static constexpr uint64_t EndColor = 1ULL << 7;
static constexpr uint64_t EnableLifetimeScale = 1ULL << 8;
static constexpr uint64_t EnableRandomColor = 1ULL << 9;
static constexpr uint64_t EnableSinScale = 1ULL << 10;
static constexpr uint64_t SinScaleFrequency = 1ULL << 11;
static constexpr uint64_t SinScaleAmplitude = 1ULL << 12;
static constexpr uint64_t EnableGravity = 1ULL << 13;
static constexpr uint64_t Gravity = 1ULL << 14;
static constexpr uint64_t EnableTrail = 1ULL << 15;
static constexpr uint64_t TrailSpawnDistance = 1ULL << 16;
static constexpr uint64_t MaxTrailPerParticle = 1ULL << 17;
static constexpr uint64_t TrailLifeTimeScale = 1ULL << 18;
static constexpr uint64_t TrailScaleMultiplier = 1ULL << 19;
static constexpr uint64_t TrailColorMultiplier = 1ULL << 20;
static constexpr uint64_t TrailVelocityScale = 1ULL << 21;
static constexpr uint64_t TrailInheritVelocity = 1ULL << 22;
static constexpr uint64_t TrailMinLifeTime = 1ULL << 23;
static constexpr uint64_t EnableGather = 1ULL << 24;
static constexpr uint64_t GatherStartRatio = 1ULL << 25;
static constexpr uint64_t GatherStrength = 1ULL << 26;
static constexpr uint64_t GatherTarget = 1ULL << 27;
static constexpr uint64_t EnableVortex = 1ULL << 28;
static constexpr uint64_t VortexStrength = 1ULL << 29;
static constexpr uint64_t VortexAxis = 1ULL << 30;
static constexpr uint64_t EnableAcceleration = 1ULL << 31;
static constexpr uint64_t Acceleration = 1ULL << 32;
static constexpr uint64_t EnableVelocityDamping = 1ULL << 33;
static constexpr uint64_t VelocityDampingFactor = 1ULL << 34;
static constexpr uint64_t EnableLifetimeVelDamping = 1ULL << 35;
static constexpr uint64_t LifetimeVelDampingStart = 1ULL << 36;
static constexpr uint64_t EnableCurlNoise = 1ULL << 37;
static constexpr uint64_t CurlNoiseScale = 1ULL << 38;
static constexpr uint64_t CurlNoiseStrength = 1ULL << 39;
static constexpr uint64_t CurlNoiseTimeScale = 1ULL << 40;
static constexpr uint64_t CurlNoiseOctaves = 1ULL << 41;
static constexpr uint64_t CurlNoiseAttractStrength = 1ULL << 42;
static constexpr uint64_t CurlNoiseBlendMode = 1ULL << 43;
static constexpr uint64_t CurlNoisePosRandom = 1ULL << 44;
// 必要があれば 45〜63 を追加可能
} // namespace ParticleSettingsOverrideBits

/// =====================================================================
/// フィールドがパーティクルに適用する「一度きりの設定上書き」データ
/// overrideMask のビットが立っている項目だけ上書きされる。
/// パーティクル側の settingsOverrideFlags に同じビットが既に立っていたら
/// 上書きをスキップし、一度きり保証を実現する。
/// =====================================================================
struct ParticleFieldSettingsOverride {
    /// 上書きするかどうかのビットマスク（0=上書きしない）
    /// ParticleSettingsOverrideBits の組み合わせ
    uint64_t overrideMask = 0;

    // ---------- 上書き値 ----------
    // overrideMask の対応ビットが立っているときのみ使用される

    float lifeTimeMin = 1.0f;
    float lifeTimeMax = 3.0f;
    float scaleMin = 0.5f;
    float scaleMax = 1.5f;
    Vector3 velocityMin = {-0.5f, -0.5f, -0.5f};
    Vector3 velocityMax = {0.5f, 0.5f, 0.5f};
    Vector4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 endColor = {1.0f, 1.0f, 1.0f, 0.0f};
    uint32_t enableLifetimeScale = 0;
    uint32_t enableRandomColor = 0;
    uint32_t enableSinScale = 0;
    float sinScaleFrequency = 1.0f;
    float sinScaleAmplitude = 0.5f;
    uint32_t enableGravity = 0;
    Vector3 gravity = {0.0f, -9.8f, 0.0f};
    uint32_t enableTrail = 0;
    float trailSpawnDistance = 0.1f;
    uint32_t maxTrailPerParticle = 5;
    float trailLifeTimeScale = 0.5f;
    Vector3 trailScaleMultiplier = {0.8f, 0.8f, 0.8f};
    Vector4 trailColorMultiplier = {1.0f, 1.0f, 1.0f, 0.7f};
    float trailVelocityScale = 0.3f;
    uint32_t trailInheritVelocity = 1;
    float trailMinLifeTime = 0.3f;
    uint32_t enableGather = 0;
    float gatherStartRatio = 0.5f;
    float gatherStrength = 2.0f;
    Vector3 gatherTarget = {0.0f, 0.0f, 0.0f};
    uint32_t enableVortex = 0;
    float vortexStrength = 5.0f;
    Vector3 vortexAxis = {0.0f, 1.0f, 0.0f};
    uint32_t enableAcceleration = 0;
    Vector3 acceleration = {0.0f, 0.0f, 0.0f};
    uint32_t enableVelocityDamping = 0;
    float velocityDampingFactor = 0.95f;
    uint32_t enableLifetimeVelDamping = 0;
    float lifetimeVelDampingStart = 0.5f;
    uint32_t enableCurlNoise = 0;
    float curlNoiseScale = 1.0f;
    float curlNoiseStrength = 1.0f;
    float curlNoiseTimeScale = 1.0f;
    uint32_t curlNoiseOctaves = 3;
    float curlNoiseAttractStrength = 0.0f;
    uint32_t curlNoiseBlendMode = 0;
    float curlNoisePosRandom = 0.0f;
};
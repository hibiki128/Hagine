#pragma once
#include "Graphics/PipeLine/PipeLineManager.h"
#include "type/Matrix4x4.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include <Model/ModelStructs.h>
#include <Transform/WorldTransform.h>
#include <cstdint>
#include <d3d12.h>
#include <list>
#include <string>
#include <vector>

struct ParticleMaterial {
    Vector4 color;
    Matrix4x4 uvTransform;
    float padding[3];
    std::string textureFilePath;
    uint32_t textureIndex = 0;
};

/// ===== GPUParticle =====

struct EmitterMesh {
    Vector3 translate;
    uint32_t triangleCount;
    Quaternion rotation;
    uint32_t emitFromSurface;
    Vector3 scale;
    float frequency;
    float frequencyTime;
    uint32_t emit;
    uint32_t edgeCount;
};

struct CSParticle {
    Vector3 translate;
    Vector3 scale;
    float lifeTime;
    Vector3 velocity;
    float currentTime;
    Vector4 color;
    Vector3 initialScale;
    float padding;
};

struct PerView {
    Matrix4x4 viewProjection;
    Matrix4x4 billboardMatrix;
};

struct TriangleInfo {
    Vector3 v0;
    float padding0;
    Vector3 v1;
    float padding1;
    Vector3 v2;
    float padding2;
};

struct PerFrame {
    float time;
    float deltaTime;
    uint32_t groupId;
    float padding;
};

struct EmitterData {
    EmitterMesh mesh;
};

struct SurfacePoint {
    Vector3 position;
    float padding;
};

 struct EdgeInfo {
    Vector3 v0;
    float padding0;
    Vector3 v1;
    float padding1;
};

struct ParticleCSGroupData {
    // マテリアルデータ
    std::vector<ParticleMaterial> materials;
    // パーティクルのリスト (std::list<Particle> 型)
    std::list<CSParticle> particles;
    // グループ名
    std::string groupName;
    // ブレンドモード
    BlendMode blendMode = BlendMode::kAdd;
};

static const uint32_t kMaxParticleCount = 100000; // 最大パーティクル数
extern uint32_t threadsPerGroup_;                 // 1グループあたりのスレッド数
extern int threadGroupSize_;                      // スレッドグループの数

struct ParticleCSSettings {
    float lifeTimeMin = 1.0f;
    float lifeTimeMax = 3.0f;
    float scaleMin = 0.5f;
    float scaleMax = 1.5f;
    Vector3 velocityMin = {-0.5f, -0.5f, -0.5f};
    float padding1{};
    Vector3 velocityMax = {0.5f, 0.5f, 0.5f};
    float padding2{};
    Vector4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 endColor = {1.0f, 1.0f, 1.0f, 0.0f};
    uint32_t enableLifetimeScale = 0;
    uint32_t enableRandomColor = 0;
    uint32_t enableSinScale = 0;
    uint32_t emitCount = 0;
    uint32_t maxParticleCount = 10000;
    float sinScaleFrequency{};
    float sinScaleAmplitude{};
    uint32_t enableGravity = 0;           
    Vector3 gravity = {0.0f, -9.8f, 0.0f};
    float padding3[1]{};
};

/// =======================

/// ====== CPUParticle ======

struct ParticleSetting {
    int maxTrailParticles; // 最大軌跡パーティクル数
    float gatherStartRatio = 0.5f;
    float gatherStrength = 2.0f;
    float trailSpawnInterval; // 軌跡パーティクル生成間隔
    float trailLifeScale{};   // 軌跡パーティクルの寿命スケール
    float lifeTimeMin{};
    float lifeTimeMax{};
    float gravity{};
    float alphaMin{};
    float alphaMax{};
    float scaleMin{};
    float scaleMax{};
    float trailVelocityScale{}; // 軌跡の速度スケール
    Vector3 translate{};
    Vector3 rotation{};
    Vector3 scale{};
    Vector3 velocityMin{};
    Vector3 velocityMax{};
    Vector3 particleStartScale{};
    Vector3 particleEndScale{};
    Vector3 startAcce{};
    Vector3 endAcce{};
    Vector3 startRote{};
    Vector3 endRote{};
    Vector3 rotateVelocityMin{};
    Vector3 rotateVelocityMax{};
    Vector3 allScaleMax{};
    Vector3 allScaleMin{};
    Vector3 rotateStartMax{};
    Vector3 rotateStartMin{};
    Vector3 trailScaleMultiplier{}; // 軌跡パーティクルのサイズ倍率
    Vector4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 endColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 trailColorMultiplier{}; // 軌跡パーティクルの色倍率
    uint32_t count{};
    bool enableTrail{};        // 軌跡機能を有効にするか
    bool trailInheritVelocity{}; // 軌跡が親の速度を継承するか
    bool isRandomColor{};
    bool isBillboard = false;
    bool isBillboardX = false;
    bool isBillboardY = false;
    bool isBillboardZ = false;
    bool isRandomRotate = false;
    bool isRotateVelocity = false;
    bool isAcceMultiply = false;
    bool isRandomSize = false;
    bool isRandomAllSize = false;
    bool isSinMove = false;
    bool isFaceDirection = false;
    bool isEndScale = false;
    bool isEmitOnEdge = false;
    bool isGatherMode = false;

    BlendMode blendMode = BlendMode::kAdd;

    ParticleSetting() : enableTrail(false), trailSpawnInterval(0.05f),
                        maxTrailParticles(1), trailLifeScale(0.5f),
                        trailScaleMultiplier({0.8f, 0.8f, 0.8f}),
                        trailColorMultiplier({1.0f, 1.0f, 1.0f, 0.7f}),
                        trailInheritVelocity(true), trailVelocityScale(0.3f) {}
};

struct ParticleStats {
    size_t count = 0;
    size_t instanceCount = 0; // 同じ名前のエミッター数
};

struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
};

struct Particle {
    WorldTransform transform{}; // 位置
    Vector3 emitterPosition{};
    Vector3 velocity{}; // 速度
    Vector3 Acce{};
    Vector3 startScale{};
    Vector3 endScale{};
    Vector3 startAcce{};
    Vector3 endAcce{};
    Vector3 startRote{};
    Vector3 endRote{};
    Vector3 rotateVelocity{};
    Vector3 fixedDirection{};
    Vector4 color{};   // 色
    float lifeTime{};  // ライフタイム
    float currentTime{}; // 現在の時間
    float initialAlpha{};
    // std::weak_ptr<Particle> parent;                  // 親パーティクルへの弱参照
    // std::vector<std::shared_ptr<Particle>> children; // 子パーティクルのリスト
    Vector3 relativePosition{}; // 親からの相対位置
    Vector3 parentOffset{};     // 親に対するオフセット
    bool isChild{};             // 子パーティクルかどうか
    bool createTrail{};         // 軌跡を作成するか
    float trailSpawnTimer{};    // 軌跡生成のタイマー
    float trailSpawnInterval{}; // 軌跡生成間隔
    int maxChildren{};          // 最大子供数
    float childLifeScale{};     // 子の寿命スケール（親より短く）

    BlendMode blendMode = BlendMode::kAdd;

    Particle() : isChild(false), createTrail(false), trailSpawnTimer(0.0f),
                 trailSpawnInterval(0.1f), maxChildren(10), childLifeScale(0.8f) {}
};

struct ParticleGroupData {
    // マテリアルデータ
    std::vector<ParticleMaterial> materials;
    // パーティクルのリスト (std::list<Particle> 型)
    std::list<Particle> particles;
    // インスタンシングデータ用SRVインデックス
    uint32_t instancingSRVIndex = 0;
    // インスタンシングリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = nullptr;
    // インスタンス数
    uint32_t instanceCount = 0;
    // インスタンシングデータを書き込むためのポインタ
    ParticleForGPU *instancingData = nullptr;
    // グループ名
    std::string groupName;
    // ブレンドモード
    BlendMode blendMode = BlendMode::kAdd;
};

/// =========================

ParticleMaterial ForParticleMaterial(MaterialData material);

std::vector<ParticleMaterial> ForParticleMaterials(std::vector<MaterialData> materials);
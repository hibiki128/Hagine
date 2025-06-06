#pragma once
#include "ParticleCommon.h"
#include "Srv/SrvManager.h"
#include "type/Vector2.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include "ViewProjection/ViewProjection.h"
#include <type/Matrix4x4.h>
#include <ModelStructs.h>
#include <ParticleGroup.h>
#include <WorldTransform.h>
#include <random>
#include <unordered_map> // 追加

struct ParticleSetting {
    Vector3 translate;
    Vector3 rotation;
    Vector3 scale;
    Vector4 startColor = {1.0f, 1.0f, 1.0f, 1.0f}; 
    Vector4 endColor = {1.0f, 1.0f, 1.0f, 1.0f}; 
    uint32_t count;
    Vector3 velocityMin;
    Vector3 velocityMax;
    float lifeTimeMin;
    float lifeTimeMax;
    float gravity;
    Vector3 particleStartScale;
    Vector3 particleEndScale;
    Vector3 startAcce;
    Vector3 endAcce;
    Vector3 startRote;
    Vector3 endRote;
    bool isRandomColor;
    float alphaMin;
    float alphaMax;
    Vector3 rotateVelocityMin;
    Vector3 rotateVelocityMax;
    Vector3 allScaleMax;
    Vector3 allScaleMin;
    float scaleMin;
    float scaleMax;
    Vector3 rotateStartMax;
    Vector3 rotateStartMin;
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
    float gatherStartRatio = 0.5f;
    float gatherStrength = 2.0f;
    bool enableTrail;             // 軌跡機能を有効にするか
    float trailSpawnInterval;     // 軌跡パーティクル生成間隔
    int maxTrailParticles;        // 最大軌跡パーティクル数
    float trailLifeScale;         // 軌跡パーティクルの寿命スケール
    Vector3 trailScaleMultiplier; // 軌跡パーティクルのサイズ倍率
    Vector4 trailColorMultiplier; // 軌跡パーティクルの色倍率
    bool trailInheritVelocity;    // 軌跡が親の速度を継承するか
    float trailVelocityScale;     // 軌跡の速度スケール

    ParticleSetting() : enableTrail(false), trailSpawnInterval(0.05f),
                        maxTrailParticles(1), trailLifeScale(0.5f),
                        trailScaleMultiplier({0.8f, 0.8f, 0.8f}),
                        trailColorMultiplier({1.0f, 1.0f, 1.0f, 0.7f}),
                        trailInheritVelocity(true), trailVelocityScale(0.3f) {}
};

class ParticleManager {
  public:
    void Initialize(SrvManager *srvManager);
    void Update(const ViewProjection &viewProjeciton);
    void Draw();
    void AddParticleGroup(ParticleGroup *particleGroup);
    void RemoveParticleGroup(const std::string &name);

    // グループごとのParticleSetting
    void SetParticleSetting(const std::string &groupName, const ParticleSetting &setting);
    ParticleSetting &GetParticleSetting(const std::string &groupName);
    std::vector<std::string> GetParticleGroupsName();
    void SetTrailEnabled(const std::string &groupName, bool enabled);
    void SetTrailSettings(const std::string &groupName, float interval, int maxTrails);
    void SetEmitterCenter(Vector3 center) { emitterCenter_ = center; }

     // 全てのパーティクルが消えたかチェック
    bool IsAllParticlesComplete() const;
    // 特定のグループのパーティクルが全て消えたかチェック
    bool IsParticleGroupComplete(const std::string &groupName) const;
    // アクティブなパーティクルの総数を取得
    size_t GetActiveParticleCount() const;
    // 特定のグループのアクティブなパーティクル数を取得
    size_t GetActiveParticleCount(const std::string &groupName) const;

  private:
    ParticleCommon *particleCommon = nullptr;
    SrvManager *srvManager_;
    std::unordered_map<std::string, ParticleGroup *> particleGroups_;
    std::unordered_map<std::string, ParticleSetting> particleSettings_; // ここがポイント

    std::vector<std::string> particleGroupNames_;
    std::random_device seedGenerator;
    std::mt19937 randomEngine;
    Vector3 emitterCenter_{};

  public:
    std::list<Particle> Emit();
  private:
    void CreateTrailParticle(const Particle &parent, const ParticleSetting &setting);

    Particle MakeNewParticle(std::mt19937 &randomEngine, const ParticleSetting &setting);
};

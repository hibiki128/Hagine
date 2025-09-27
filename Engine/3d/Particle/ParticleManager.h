#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Graphics/Srv/SrvManager.h"
#include "ParticleCommon.h"
#include "ParticleGroup.h"
#include "type/Vector2.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include <Model/ModelStructs.h>
#include <Transform/WorldTransform.h>
#include <random>
#include <type/Matrix4x4.h>
#include <unordered_map>
#include"ParticleStruct.h"

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

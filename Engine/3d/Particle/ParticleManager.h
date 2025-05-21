#pragma once
#include "ParticleCommon.h"
#include "Srv/SrvManager.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "ViewProjection/ViewProjection.h"
#include "random"
#include <Matrix4x4.h>
#include <ModelStructs.h>
#include <ParticleGroup.h>
#include <WorldTransform.h>

struct ParticleSetting {
    Vector3 translate;
    Vector3 rotation;
    Vector3 scale; // スケールを引数として受け取る
    uint32_t count;
    Vector3 velocityMin;
    Vector3 velocityMax; // 速度の範囲
    float lifeTimeMin;
    float lifeTimeMax;
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
};

class ParticleManager {

  public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(SrvManager *srvManager);

    /// <summary>
    /// 更新
    /// </summary>
    void Update(const ViewProjection &viewProjeciton);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// パーティクルグループの生成
    /// </summary>
    /// <param name="name"></param>
    /// <param name="textureFilePath"></param>
    void AddParticleGroup(ParticleGroup *particleGroup);

    void RemoveParticleGroup(const std::string &name) {
        // マップやセットなどからグループ本体を削除
        particleGroups_.erase(name);

        // 名前リスト（vector）からも削除
        auto it = std::find(particleGroupNames_.begin(), particleGroupNames_.end(), name);
        if (it != particleGroupNames_.end()) {
            particleGroupNames_.erase(it);
        }
    }

    void SetParticleSetting(const ParticleSetting &particleSetting) {
        particleSetting_ = particleSetting;
        SetGatherStartRatio(particleSetting.gatherStartRatio);
        SetGatherStrength(particleSetting.gatherStrength);
    }

    std::vector<std::string> GetParticleGroupsName() { return particleGroupNames_; }

  private:
    ParticleCommon *particleCommon = nullptr;

    SrvManager *srvManager_;
    std::unordered_map<std::string, ParticleGroup *> particleGroups_;
    std::vector<std::string> particleGroupNames_;

    std::random_device seedGenerator;
    std::mt19937 randomEngine;

    ParticleSetting particleSetting_ = {};

  public:
    std::list<Particle> Emit();

  private:
    Particle MakeNewParticle(std::mt19937 &randomEngine);

    void SetGatherStartRatio(float ratio) { particleSetting_.gatherStartRatio = std::clamp(ratio, 0.0f, 1.0f); }

    void SetGatherStrength(float strength) { particleSetting_.gatherStrength = strength; }
};

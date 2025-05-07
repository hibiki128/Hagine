#pragma once
#include "ParticleCommon.h"
#include"Srv/SrvManager.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include"ViewProjection/ViewProjection.h"
#include "random"
#include <Matrix4x4.h>
#include <WorldTransform.h>
#include <ModelStructs.h>
#include <ParticleGroup.h>
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
    void AddParticleGroup(ParticleGroup* particleGroup);

    void RemoveParticleGroup(const std::string &name) {
        // マップやセットなどからグループ本体を削除
        particleGroups_.erase(name);

        // 名前リスト（vector）からも削除
        auto it = std::find(particleGroupNames_.begin(), particleGroupNames_.end(), name);
        if (it != particleGroupNames_.end()) {
            particleGroupNames_.erase(it);
        }
    }

    void SetBillBorad(bool isBillBoard) { isBillboard = isBillBoard; }
    void SetRandomRotate(bool isRandomRotate) { isRandomRotate_ = isRandomRotate; }
    void SetRotateVelocity(bool isRotateVelocity) { isRotateVelocity_ = isRotateVelocity; }
    void SetAcceMultipy(bool isAcceMultipy) { isAcceMultipy_ = isAcceMultipy; }
    void SetRandomSize(bool isRandomSize) { isRandomSize_ = isRandomSize; }
    void SetAllRandomSize(bool isAllRandomSize) { isRandomAllSize_ = isAllRandomSize; }
    void SetSinMove(bool isSinMove) { isSinMove_ = isSinMove; }
    void SetFaceDirection(bool flag) { isFaceDirection_ = flag; }
    void SetEndScale(bool isEndScale) { isEndScale_ = isEndScale; }
    void SetOnEdge(bool isOnEdge) { isEmitOnEdge_ = isOnEdge; }
   
    void SetGatherMode(bool isGather) { isGatherMode_ = isGather; }

    void SetGatherStartRatio(float ratio) { gatherStartRatio_ = std::clamp(ratio, 0.0f, 1.0f); }

    void SetGatherStrength(float strength) { gatherStrength_ = strength; }

    std::vector<std::string> GetParticleGroupsName() { return particleGroupNames_; }

  private:
    ParticleCommon *particleCommon = nullptr;

    SrvManager *srvManager_;
    std::unordered_map<std::string, ParticleGroup*> particleGroups_;
    std::vector<std::string> particleGroupNames_;

    std::random_device seedGenerator;
    std::mt19937 randomEngine;

    bool isBillboard = false;
    bool isRandomRotate_ = false;
    bool isRotateVelocity_ = false;
    bool isAcceMultipy_ = false;
    bool isRandomSize_ = false;
    bool isRandomAllSize_ = false;
    bool isSinMove_ = false;
    bool isFaceDirection_ = false;
    bool isEndScale_ = false;
    bool isEmitOnEdge_ = false;
    bool isGatherMode_ = false;     // 中心に集まるフラグ

    float gatherStartRatio_ = 0.5f; // ライフタイム何割から中心に集まり始めるか (0.0～1.0)
    float gatherStrength_ = 2.0f;   // 集まる力の強さ

  public:
    std::list<Particle> Emit(const Vector3 &position, uint32_t count, const Vector3 &scale,
                             const Vector3 &velocityMin, const Vector3 &velocityMax, float lifeTimeMin, float lifeTimeMax,
                             const Vector3 &particleStartScale, const Vector3 &particleEndScale, const Vector3 &startAcce, const Vector3 &endAcce,
                             const Vector3 &startRote, const Vector3 &endRote, bool isRandomColor, float alphaMin, float alphaMax,
                             const Vector3 &rotateVelocityMin, const Vector3 &rotateVelocityMax,
                             const Vector3 &allScaleMax, const Vector3 &allScaleMin,
                             const float &scaleMin, const float &scaleMax, const Vector3 &rotation,
                             const Vector3 &rotateStartMax, const Vector3 &rotateStartMin);

  private:

    Particle MakeNewParticle(std::mt19937 &randomEngine,
                             const Vector3 &translate,
                             const Vector3 &rotation,
                             const Vector3 &scale,
                             const Vector3 &velocityMin, const Vector3 &velocityMax,
                             float lifeTimeMin, float lifeTimeMax, const Vector3 &particleStartScale, const Vector3 &particleEndScale,
                             const Vector3 &startAcce, const Vector3 &endAcce, const Vector3 &startRote, const Vector3 &endRote, bool isRamdomColor, float alphaMin, float alphaMax, const Vector3 &rotateVelocityMin, const Vector3 &rotateVelocityMax,
                             const Vector3 &allScaleMax, const Vector3 &allScaleMin,
                             const float &scaleMin, const float &scaleMax,
                             const Vector3 &rotateStartMax, const Vector3 &rotateStartMin);
};

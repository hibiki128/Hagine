#include "ParticleCommon.h"

ParticleCommon *ParticleCommon::instance = nullptr;

ParticleCommon *ParticleCommon::GetInstance() {
    if (instance == nullptr) {
        instance = new ParticleCommon();
    }
    return instance;
}

void ParticleCommon::Finalize() {
    delete instance;
    instance = nullptr;
}

void ParticleCommon::Initialize(DirectXCommon *dxCommon) {
    assert(dxCommon);
    dxCommon_ = dxCommon;
    psoManager_ = PipeLineManager::GetInstance();
    computePsoManager_ = ComputePipeLineManager::GetInstance();
}

void ParticleCommon::DrawCommonSetting(BlendMode blendMode) {
    psoManager_->DrawCommonSetting(PipelineType::kParticle, blendMode);
}

void ParticleCommon::GPUDrawCommonSetting(BlendMode blendMode) {
    psoManager_->DrawCommonSetting(PipelineType::kGPUParticle, blendMode);
}

void ParticleCommon::ComputeInitDrawCommonSetting() {
    computePsoManager_->DrawCommonSetting(ComputePipelineType::kInitParticle);
}

void ParticleCommon::ComputeEmitterDrawCommonSetting() {
    computePsoManager_->DrawCommonSetting(ComputePipelineType::kEmitter);
}

void ParticleCommon::ComputeUpdateEmitterDrawCommonSetting() {
    computePsoManager_->DrawCommonSetting(ComputePipelineType::kUpdateEmitter);
}

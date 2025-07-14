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
}

void ParticleCommon::DrawCommonSetting(BlendMode blendMode) {
    psoManager_->DrawCommonSetting(PipelineType::kParticle, blendMode);
}
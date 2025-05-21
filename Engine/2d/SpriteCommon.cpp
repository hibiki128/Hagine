#include "SpriteCommon.h"

SpriteCommon *SpriteCommon::instance = nullptr;

SpriteCommon *SpriteCommon::GetInstance() {
    if (instance == nullptr) {
        instance = new SpriteCommon();
    }
    return instance;
}

void SpriteCommon::Finalize() {
    delete instance;
    instance = nullptr;
}

void SpriteCommon::Initialize() {
    // 引数で受け取ってメンバ変数に記録する
    dxCommon_ = DirectXCommon::GetInstance();
    psoManager_ = PipeLineManager::GetInstance();
}

void SpriteCommon::DrawCommonSetting() {
    psoManager_->DrawCommonSetting(PipelineType::kSprite);
}

void SpriteCommon::SetBlendMode(BlendMode blendMode) {
    psoManager_->DrawCommonSetting(PipelineType::kSprite, blendMode);
}

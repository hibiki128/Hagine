#include "SpriteCommon.h"

void SpriteCommon::Finalize() {
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

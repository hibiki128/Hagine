#include "Object3dCommon.h"
void Object3dCommon::Initialize() {
   psoManager_ = PipeLineManager::GetInstance();
}

void Object3dCommon::DrawCommonSetting() {
    psoManager_->DrawCommonSetting(PipelineType::kStandard);
}

void Object3dCommon::skinningDrawCommonSetting() {
    psoManager_->DrawCommonSetting(PipelineType::kSkinning);
}

void Object3dCommon::SetBlendMode(BlendMode blendMode) {
    psoManager_->DrawCommonSetting(PipelineType::kStandard, blendMode);
}

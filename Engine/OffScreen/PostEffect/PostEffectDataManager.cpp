#include "PostEffectDataManager.h"

void PostEffectDataManager::Initialize() {
    dataHandler_ = std::make_unique<DataHandler>("OffScreen", "OffScreenData");
}

void PostEffectDataManager::SaveData(const PostEffectChain &chain) const {
    const auto &slots = chain.GetSlots();

    for (int i = 0; i < PostEffectChain::kMaxSlots; ++i) {
        const std::string prefix = "slot" + std::to_string(i) + "_";
        const auto &slot         = slots[i];

        dataHandler_->Save<bool>(prefix + "occupied", slot.occupied);

        if (!slot.occupied) { continue; }

        dataHandler_->Save<bool>  (prefix + "enabled",    slot.enabled);
        dataHandler_->Save<std::string>(prefix + "name",  slot.name);
        dataHandler_->Save<int>   (prefix + "shaderMode", static_cast<int>(slot.params->GetMode()));

        // エフェクト固有パラメータをプレフィックス付きで保存
        slot.params->Save(dataHandler_.get(), prefix + "param_");
    }
}

void PostEffectDataManager::LoadData(PostEffectChain &chain, DirectXCommon *dxCommon) {
    chain.Clear();

    for (int i = 0; i < PostEffectChain::kMaxSlots; ++i) {
        const std::string prefix = "slot" + std::to_string(i) + "_";

        const bool occupied = dataHandler_->Load<bool>(prefix + "occupied", false);
        if (!occupied) { continue; }

        const bool enabled       = dataHandler_->Load<bool>(prefix + "enabled", true);
        const std::string name   = dataHandler_->Load<std::string>(prefix + "name", "");
        const ShaderMode mode    = static_cast<ShaderMode>(
            dataHandler_->Load<int>(prefix + "shaderMode", 0));

        // 指定スロットに追加
        const int resultSlot = chain.AddEffect(mode, name, dxCommon, i);

        if (resultSlot == -1) { continue; }

        chain.SetEnabled(resultSlot, enabled);

        // エフェクト固有パラメータのロード
        IPostEffectParams *params = chain.GetParams(resultSlot);
        if (params) {
            params->Load(dataHandler_.get(), prefix + "param_");
        }
    }
}

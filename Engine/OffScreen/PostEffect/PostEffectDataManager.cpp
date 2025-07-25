#include "PostEffectDataManager.h"

void PostEffectDataManager::Initialize() {
    dataHandler_ = std::make_unique<DataHandler>("OffScreen", "OffScreenData");
}

void PostEffectDataManager::SaveData(const PostEffectChain &chain, const PostEffectParameters &params) {
    // エフェクトチェーンの保存
    const auto &effects = chain.GetEffects();
    int effectCount = static_cast<int>(effects.size());
    dataHandler_->Save<int>("effectCount", effectCount);

    // 各エフェクトの情報を保存
    for (int i = 0; i < effectCount; ++i) {
        std::string shaderModeKey = "effect" + std::to_string(i) + "_shaderMode";
        std::string enabledKey = "effect" + std::to_string(i) + "_enabled";

        dataHandler_->Save<int>(shaderModeKey, static_cast<int>(effects[i].shaderMode));
        dataHandler_->Save<bool>(enabledKey, effects[i].enabled);
    }

    // エフェクトパラメータの保存
    params.SaveParameters(dataHandler_.get());
}

void PostEffectDataManager::LoadData(PostEffectChain &chain, PostEffectParameters &params) {
    // エフェクトチェーンの読み込み
    int effectCount = dataHandler_->Load<int>("effectCount", 0);

    // エフェクトチェーンをクリア
    chain.Clear();

    // 各エフェクトを読み込み
    for (int i = 0; i < effectCount; ++i) {
        std::string shaderModeKey = "effect" + std::to_string(i) + "_shaderMode";
        std::string enabledKey = "effect" + std::to_string(i) + "_enabled";

        ShaderMode shaderMode = static_cast<ShaderMode>(dataHandler_->Load<int>(shaderModeKey, 0));
        bool enabled = dataHandler_->Load<bool>(enabledKey, false);

        // エフェクトを追加
        chain.AddEffect(shaderMode);

        // 有効/無効状態を設定（最後に追加されたエフェクトのインデックスを使用）
        const auto &effects = chain.GetEffects();
        if (!effects.empty()) {
            chain.SetEffectEnabled(static_cast<int>(effects.size() - 1), enabled);
        }
    }

    // エフェクトパラメータの読み込み
    params.LoadParameters(dataHandler_.get());
}
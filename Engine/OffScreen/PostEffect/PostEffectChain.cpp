#include "PostEffectChain.h"


// エフェクト管理メソッド
void PostEffectChain::AddEffect(ShaderMode mode) {
    PostEffectSettings settings;
    settings.shaderMode = mode;
    settings.enabled = true;
    effects_.push_back(settings);
}

void PostEffectChain::RemoveEffect(int index) {
    if (index >= 0 && index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void PostEffectChain::SetEffectEnabled(int index, bool enabled) {
    if (index >= 0 && index < effects_.size()) {
        effects_[index].enabled = enabled;
    }
}

void PostEffectChain::MoveEffectUp(int index) {
    if (index > 0 && index < effects_.size()) {
        std::swap(effects_[index], effects_[index - 1]);
    }
}

void PostEffectChain::MoveEffectDown(int index) {
    if (index >= 0 && index < effects_.size() - 1) {
        std::swap(effects_[index], effects_[index + 1]);
    }
}

void PostEffectChain::Clear() {
    effects_.clear();
}

std::vector<int> PostEffectChain::GetEnabledEffectIndices() const {
    std::vector<int> enabledEffects;

    for (size_t i = 0; i < effects_.size(); ++i) {
        if (effects_[i].enabled) {
            enabledEffects.push_back(static_cast<int>(i));
        }
    }

    return enabledEffects;
}

bool PostEffectChain::HasEnabledEffects() const {
    for (const auto &effect : effects_) {
        if (effect.enabled) {
            return true;
        }
    }
    return false;
}
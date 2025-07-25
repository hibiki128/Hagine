#pragma once
#include <vector>
#include <Graphics/PipeLine/PipeLineManager.h>

struct PostEffectSettings {
    ShaderMode shaderMode = ShaderMode::kNone;
    bool enabled = false;
};

class PostEffectChain {
  public:
    void AddEffect(ShaderMode mode);
    void RemoveEffect(int index);
    void SetEffectEnabled(int index, bool enabled);
    void MoveEffectUp(int index);
    void MoveEffectDown(int index);
    void Clear();

    const std::vector<PostEffectSettings> &GetEffects() const { return effects_; }
    std::vector<int> GetEnabledEffectIndices() const;
    bool IsEmpty() const { return effects_.empty(); }
    bool HasEnabledEffects() const;

  private:
    std::vector<PostEffectSettings> effects_;
};
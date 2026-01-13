#pragma once
#include "Data/DataHandler.h"
#include "PostEffectChain.h"
#include "PostEffectParameters.h"
namespace Hagine::Graphics {
class PostEffectDataManager {
  public:
    void Initialize();
    void SaveData(const PostEffectChain &chain, const PostEffectParameters &params);
    void LoadData(PostEffectChain &chain, PostEffectParameters &params);

  private:
    std::unique_ptr<DataHandler> dataHandler_;
};
} // namespace Hagine::Graphics
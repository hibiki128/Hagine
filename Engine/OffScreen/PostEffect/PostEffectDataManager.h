#pragma once
#include "Data/DataHandler.h"
#include "PostEffectChain.h"
#include <memory>

/// @brief スロットベースのエフェクトチェーンのセーブ/ロードを担当
class PostEffectDataManager {
  public:
    void Initialize();

    void SaveData(const PostEffectChain &chain) const;
    void LoadData(PostEffectChain &chain, DirectXCommon *dxCommon);

  private:
    std::unique_ptr<DataHandler> dataHandler_;
};

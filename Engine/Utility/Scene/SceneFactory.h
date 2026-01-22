#pragma once
#include "AbstractSceneFactory.h"

namespace Hagine::Scene {

class SceneFactory : public AbstractSceneFactory {
  public:
    /// <summary>
    /// シーン生成
    /// </summary>
    /// <param name="sceneName"></param>
    /// <returns></returns>
    BaseScene *CreateScene(const std::string &sceneName) override;
};
} // namespace Hagine::Scene
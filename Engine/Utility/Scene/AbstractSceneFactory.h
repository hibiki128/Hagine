#pragma once
#include "BaseScene.h"
#include <memory>
#include <string>

class AbstractSceneFactory {
  public:
    virtual ~AbstractSceneFactory() = default;

    /// <summary>
    /// シーン生成
    /// </summary>
    /// <param name="sceneName"></param>
    /// <returns></returns>
    virtual std::unique_ptr<BaseScene> CreateScene(const std::string &sceneName) = 0;
};
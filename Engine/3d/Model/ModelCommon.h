#pragma once
#include "DirectXCommon.h"
class ModelCommon {
  private:
    DirectXCommon *dxCommon_;
    static ModelCommon *instance;

    ModelCommon() = default;
    ~ModelCommon() = default;
    ModelCommon(ModelCommon &) = delete;
    ModelCommon &operator=(ModelCommon &) = delete;
  public:
    static ModelCommon *GetInstance();

    void Finalize();

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="dxCommon"></param>
    void Initialize();

    /// <summary>
    /// getter
    /// </summary>
    /// <returns></returns>
    DirectXCommon *GetDxCommon() const { return dxCommon_; }
};

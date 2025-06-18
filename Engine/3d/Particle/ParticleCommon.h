#pragma once
#include "DirectXCommon.h"
#include "PipeLine/PipeLineManager.h"
class ParticleCommon {
  private:
    static ParticleCommon *instance;

    ParticleCommon() = default;
    ~ParticleCommon() = default;
    ParticleCommon(ParticleCommon &) = delete;
    ParticleCommon &operator=(ParticleCommon &) = delete;

  public:
    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    /// <returns></returns>
    static ParticleCommon *GetInstance();

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(DirectXCommon *dxCommon);

    /// <summary>
    /// 共通描画処理
    /// </summary>
    void DrawCommonSetting(BlendMode blendMode);

    DirectXCommon *GetDxCommon() const { return dxCommon_; }

  private:
    DirectXCommon *dxCommon_ = nullptr;
    PipeLineManager *psoManager_ = nullptr;
};

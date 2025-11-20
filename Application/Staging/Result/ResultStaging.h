#pragma once
#include <Object/Base/BaseObject.h>
class ResultStaging {

  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

  private:
    BaseObject *RightHand_ = nullptr;
    BaseObject *LeftHand_ = nullptr;

    bool secondMove_ = false;
    bool motionStarted_ = false;
};

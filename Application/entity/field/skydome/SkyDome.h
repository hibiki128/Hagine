#pragma once
#include "object/base/BaseObject.h"

/// <summary>
/// 空のドームを表現するゲームオブジェクトクラス
/// </summary>
class SkyDome : public Hagine::BaseObject
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="className">クラス名</param>
    void Init(const std::string className) override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    /// <param name="offSet">描画オフセット</param>
    void Draw(const Hagine::ViewProjection &viewProjection) override;
};
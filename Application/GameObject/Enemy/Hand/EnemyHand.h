#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"

/// <summary>
/// 敵の手（見た目）のゲームオブジェクトクラス
/// </summary>
class EnemyHand : public Hagine::BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="objectName">オブジェクト名</param>
    void Init(const std::string objectName) override;

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
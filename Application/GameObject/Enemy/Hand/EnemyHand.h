#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"

/// <summary>
/// 敵の手（攻撃判定）のゲームオブジェクトクラス
/// 攻撃ごとにダメージ量・ノックバックを外部から設定可能
/// </summary>
class EnemyHand : public BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Init(const std::string objectName) override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw(const ViewProjection &viewProjection,
              Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;
};
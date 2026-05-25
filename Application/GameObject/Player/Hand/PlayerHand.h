#pragma once
#include "Object/Base/BaseObject.h"

/// <summary>
/// プレイヤーの手のゲームオブジェクトクラス（ビジュアル専用）
/// 攻撃判定は PlayerAttackCollider が担当するため、
/// このクラスはモーションアニメーションの見た目のみを管理する
/// </summary>
class PlayerHand : public BaseObject {
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
    void Draw(const ViewProjection &viewProjection) override;
};
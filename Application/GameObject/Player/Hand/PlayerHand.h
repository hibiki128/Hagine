#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"
class Enemy;

/// <summary>
/// プレイヤーの手（攻撃判定）のゲームオブジェクトクラス
/// </summary>
class PlayerHand : public BaseObject {
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
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const ViewProjection &viewProjection);

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    /// <param name="offSet">描画オフセット</param>
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    /// <summary>
    /// 敵参照を設定
    /// </summary>
    /// <param name="enemy">設定する敵のポインタ</param>
    void SetEnemy(Enemy *enemy) { enemy_ = enemy; }

    /// <summary>
    /// 衝突判定時の処理
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollisionEnter([[maybe_unused]] Collider *other) override;
  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    Enemy *enemy_ = nullptr;

    std::unique_ptr<ParticleEmitter> hitEmitter_;
    std::unique_ptr<Shake> shake_;
};
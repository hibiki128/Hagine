#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"
class Enemy;
class Player;

/// <summary>
/// プレイヤーの手（攻撃判定）のゲームオブジェクトクラス
/// </summary>
class PlayerHand : public Hagine::Graphics::BaseObject {
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
    void DrawParticle(const Camera::ViewProjection &viewProjection);

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    /// <param name="offSet">描画オフセット</param>
    void Draw(const Camera::ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    /// <summary>
    /// 敵参照を設定
    /// </summary>
    /// <param name="enemy">設定する敵のポインタ</param>
    void SetEnemy(Enemy *enemy) { enemy_ = enemy; }

    /// <summary>
    /// 衝突判定時の処理
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollisionEnter(ColliderBase* other);

    /// <summary>
    /// エネルギー回復量を設定
    /// </summary>
    /// <param name="amount">回復量</param>
    void SetEnergyRecoveryAmount(float amount) { energyRecoveryAmount_ = amount; }

    /// <summary>
    /// プレイヤー参照を設定
    /// </summary>
    /// <param name="player">設定するプレイヤーのポインタ</param>
    void SetPlayer(class Player *player) { player_ = player; }

    void SetColliderEnabled(bool enabled) {
        if (collider_) {
            collider_->SetEnabled(enabled);
        }
    }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    Enemy *enemy_ = nullptr;
    Player *player_ = nullptr;

    float energyRecoveryAmount_ = 5.0f;

    std::unique_ptr<ParticleEmitter> hitEmitter_;
    std::unique_ptr<Shake> shake_;

    SphereCollider *collider_ = nullptr;
};
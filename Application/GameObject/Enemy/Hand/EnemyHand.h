#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"

class Enemy;
class Player;

/// <summary>
/// 敵の手（攻撃判定）のゲームオブジェクトクラス
/// 攻撃ごとにダメージ量・ノックバックを外部から設定可能
/// </summary>
class EnemyHand : public BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    void Init(const std::string objectName) override;
    void Update() override;
    void Draw(const ViewProjection &viewProjection,
              Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;
    void DrawParticle(const ViewProjection &viewProjection);

    /// <summary>
    /// 衝突判定時の処理
    /// </summary>
    void OnCollisionEnter(ColliderBase *other);

    /// <summary>
    /// コライダーの有効/無効を切り替える
    /// ComboSystem側から攻撃タイミングに合わせて呼ばれる
    /// </summary>
    void SetColliderEnabled(bool enabled) {
        if (collider_) {
            collider_->SetEnabled(enabled);
        }
    }

    /// ===================================================
    /// Setter
    /// ===================================================

    void SetPlayer(Player *player) { player_ = player; }
    void SetEnemy(Enemy *enemy) { enemy_ = enemy; }

    /// <summary>
    /// ヒット時に与えるダメージ量を設定
    /// Enemy::ConboUpdateからコンボの段ごとに設定される
    /// </summary>
    void SetDamageAmount(float amount) { damageAmount_ = amount; }

    /// <summary>
    /// ヒット時のノックバック強度を設定
    /// </summary>
    void SetKnockbackPower(float power) { knockbackPower_ = power; }

    /// <summary>
    /// エネルギー回復量を設定
    /// </summary>
    void SetEnergyRecoveryAmount(float amount) { energyRecoveryAmount_ = amount; }

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    Enemy *enemy_ = nullptr;
    Player *player_ = nullptr;

    float damageAmount_ = 4.0f;   // ヒット時ダメージ（ComboSystemから上書き可能）
    float knockbackPower_ = 3.0f; // ノックバック強度（ComboSystemから上書き可能）
    float energyRecoveryAmount_ = 5.0f;

    std::unique_ptr<ParticleEmitter> hitEmitter_;
    std::unique_ptr<Shake> shake_;
    SphereCollider *collider_ = nullptr;
};
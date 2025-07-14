#pragma once
#include "Object/Base/BaseObject.h"
#include"Particle/ParticleEmitter.h"

class Player;
class Enemy;

class PlayerBullet : public BaseObject {
  public:
    /// ==================================
    /// public method
    /// ==================================

    void Init(const std::string objectName) override;
    void Update() override;
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;
    void DrawParticle(const ViewProjection &viewProjection);
    void InitTransform(Player *player);

    void OnCollisionEnter([[maybe_unused]] Collider *other) override;

    // 弾が生きているかどうかを取得
    bool IsAlive() const { return isAlive_; }

    void SetSpeed(float speed) { speed_ = speed; }
    void SetAcce(float acce) { acce_ = acce; }

    // 加速度の取得
    float GetAcceleration() const { return acce_; }

    // 現在の速度の大きさを取得
    float GetCurrentSpeed() const {
        return std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    }

  private:
    /// ==================================
    /// private method
    ///==================================

  private:
    /// ==================================
    /// private variants
    /// ==================================

    Vector3 velocity_;
    float speed_ = 60.0f; // 弾の初期速度
    float acce_ = 10.0f;  // 弾の加速度（単位：速度/秒）

    // 生存時間関連
    float lifeTime_ = 5.0f;        // 弾の生存時間（秒）
    float currentLifeTime_ = 0.0f; // 現在の生存時間
    bool isAlive_ = true;          // 弾が生きているかどうか
    bool isHit_ = false;

    // ロックオン関連
    bool isLockOnBullet_ = false;  // ロックオン弾かどうか
    Enemy *targetEnemy_ = nullptr; // ターゲットの敵
    std::unique_ptr<ParticleEmitter> emitter_;
};
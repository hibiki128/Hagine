#pragma once
#include "application/Base/BaseObject.h"

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
    void InitTransform(Player *player);

    void OnCollisionEnter([[maybe_unused]] Collider *other)override;

    // 弾が生きているかどうかを取得
    bool IsAlive() const { return isAlive_; }

    void SetScale(float scale) {
        this->transform_.scale_ = {scale, scale, scale};
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
    float speed_ = 30.0f; // 弾の速度を上げる
    float acce_;

    // 生存時間関連
    float lifeTime_ = 5.0f;        // 弾の生存時間（秒）
    float currentLifeTime_ = 0.0f; // 現在の生存時間
    bool isAlive_ = true;          // 弾が生きているかどうか

    // ロックオン関連
    bool isLockOnBullet_ = false;  // ロックオン弾かどうか
    Enemy *targetEnemy_ = nullptr; // ターゲットの敵
};
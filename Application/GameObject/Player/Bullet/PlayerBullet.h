#pragma once
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"
class Player;
class Enemy;

/// <summary>
/// プレイヤーが発射する弾のゲームオブジェクトクラス
/// </summary>
class PlayerBullet : public BaseObject {
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
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    /// <summary>
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const ViewProjection &viewProjection);

    /// <summary>
    /// プレイヤーの情報から弾のトランスフォームを初期化
    /// </summary>
    /// <param name="player">プレイヤーのポインタ</param>
    void InitTransform(Player *player);

    /// <summary>
    /// 衝突判定時の処理
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollisionEnter([[maybe_unused]] Collider *other) override;

    /// <summary>
    /// 弾が生きているかを取得
    /// </summary>
    /// <returns>bool: 生存フラグ</returns>
    bool IsAlive() const { return isAlive_; }

    /// <summary>
    /// 移動速度を設定
    /// </summary>
    /// <param name="speed">設定する速度</param>
    void SetSpeed(float speed) { speed_ = speed; }

    /// <summary>
    /// 加速度を設定
    /// </summary>
    /// <param name="acce">設定する加速度</param>
    void SetAcce(float acce) { acce_ = acce; }

    /// <summary>
    /// 加速度を取得
    /// </summary>
    /// <returns>float: 現在の加速度</returns>
    float GetAcceleration() const { return acce_; }

     /// <summary>
    /// 現在の速度の大きさを取得
    /// </summary>
    /// <returns>float: 速度の大きさ</returns>
    float GetCurrentSpeed() const {
        return std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    Vector3 velocity_;
    float speed_ = 60.0f; // 移動速度
    float acce_ = 10.0f;  // 加速度

    float lifeTime_ = 5.0f;        // 弾の生存時間（秒）
    float currentLifeTime_ = 0.0f; // 現在の生存時間
    bool isAlive_ = true;          // 弾が生きているかどうか
    bool isHit_ = false;           // 衝突判定フラグ

    bool isLockOnBullet_ = false;  // ロックオン弾かどうか
    Enemy *targetEnemy_ = nullptr; // ターゲットの敵
    std::unique_ptr<ParticleEmitter> emitter_;
};
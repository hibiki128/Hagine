#pragma once
#include "object/base/BaseObject.h"
#include "particle/ParticleEmitter.h"
class Player;
class Enemy;

/// <summary>
/// プレイヤーが発射する弾のゲームオブジェクトクラス
/// </summary>
class PlayerBullet : public Hagine::BaseObject
{
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
    void Draw(const Hagine::ViewProjection &viewProjection) override;

    /// <summary>
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const Hagine::ViewProjection &viewProjection);

    /// <summary>
    /// プレイヤーの情報から弾のトランスフォームを初期化
    /// </summary>
    /// <param name="player">プレイヤーのポインタ</param>
    void InitTransform(Player *player);

    /// <summary>
    /// 衝突判定時の処理
    /// </summary>
    /// <param name="pCollider">衝突したコライダー</param>
    void OnCollisionEnter(Hagine::ColliderBase *pCollider);

    /// <summary>
    /// ガードされた弾を、相手の外側斜め上へ弾き返す（ホーミングと加速は打ち切る）
    /// </summary>
    /// <param name="guardPosition">ガードした相手の位置</param>
    void DeflectFrom(const Hagine::Vector3 &guardPosition);

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
    float GetCurrentSpeed() const
    {
        return std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    }

    /// <summary>
    /// コライダーの半径を設定
    /// </summary>
    /// <param name="radius">設定する半径</param>
    void SetColliderRadius(float radius)
    {
        if (pCollider_)
        {
            pCollider_->SetRadius(radius);
        }
    }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    // 定数定義
    static constexpr float kDefaultSpeed = 80.0f;         // デフォルトの移動速度
    static constexpr float kDefaultAcceleration = 10.0f;  // デフォルトの加速度
    static constexpr float kDefaultLifeTime = 5.0f;       // デフォルトの生存時間(秒)
    static constexpr float kMinDistanceThreshold = 0.1f;  // 距離判定の最小閾値
    static constexpr float kHomingStrength = 20.0f;       // ホーミング強度
    static constexpr float kHomingHeightOffset = 1.5f;    // ホーミング終点の高さオフセット（人型モデルの胸あたりを狙う）
    static constexpr float kMinSpeedThreshold = 0.1f;     // 速度判定の最小閾値
    static constexpr float kMaxSpeed = 200.0f;            // 最大速度
    static constexpr float kForwardOffsetDistance = 2.0f; // 前方オフセット距離（ジョイント取得失敗時の代用）
    static constexpr float kVerticalOffset = 1.0f;        // 垂直方向オフセット（ジョイント取得失敗時の代用）
    static constexpr int kBulletDamage = 1;               // 弾のダメージ値
    static constexpr float kFallbackKillY = -10.0f;       // 地形外へ抜けた弾を消す高さ（保険）

    // ガードで弾き返されたときの挙動
    static constexpr float kDeflectSpeedRatio = 0.6f; // 弾き返し後に残る速度の割合
    static constexpr float kDeflectMinSpeed = 25.0f;  // 弾き返し後の最低速度
    static constexpr float kDeflectUpSpeed = 12.0f;   // 弾き返し時の上方向速度
    static constexpr float kDeflectBackRatio = 0.5f;  // 弾き返す向きに混ぜる「相手の外側」成分の比率
    static constexpr float kDeflectLifeTime = 1.0f;   // 弾き返された弾が消えるまでの時間(秒)

    static constexpr const char *kHandJointName = "mixamorig:RightHand"; // 発射起点に使う手ジョイント名
    static constexpr float kHandForwardOffset = 0.5f;                    // 手からの前方オフセット距離

    Hagine::Vector3 velocity_;
    float speed_ = kDefaultSpeed;       // 移動速度
    float acce_ = kDefaultAcceleration; // 加速度

    float lifeTime_ = kDefaultLifeTime; // 弾の生存時間(秒)
    float currentLifeTime_ = 0.0f;      // 現在の生存時間
    bool isAlive_ = true;               // 弾が生きているかどうか
    bool isHit_ = false;                // 衝突判定フラグ
    bool isDeflected_ = false;          // ガードで弾き返されたか（以降ダメージを与えない）

    bool isLockOnBullet_ = false;  // ロックオン弾かどうか
    Enemy *pTargetEnemy_ = nullptr; // ターゲットの敵
    std::unique_ptr<Hagine::ParticleEmitter> emitter_;

    Hagine::SphereCollider *pCollider_ = nullptr;
};
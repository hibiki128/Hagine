#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Collider/type/OBBCollider.h"
#include "Particle/ParticleEmitter.h"

class Player;
class Enemy;
class ColliderBase;

/// <summary>
/// プレイヤー前方に展開する攻撃判定コライダー
/// 手のモーション（見た目）とは完全に独立して機能する
/// 攻撃ごとにダメージ・ノックバック・有効時間・遅延を個別設定できる
/// </summary>
class PlayerAttackCollider {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    /// <param name="enemy">敵参照（後でSetEnemyでも設定可）</param>
    void Init(Player *player, Enemy *enemy = nullptr);

    /// <summary>
    /// 更新処理（毎フレーム呼び出し必須）
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// パーティクル描画
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const ViewProjection &viewProjection);

    /// <summary>
    /// 攻撃コライダーを有効化する
    /// コンボシステムの攻撃発火コールバックから呼ぶ
    /// </summary>
    /// <param name="damage">この攻撃で与えるダメージ量</param>
    /// <param name="knockbackPower">ノックバックの強さ</param>
    /// <param name="activeDuration">コライダーが有効な時間（秒）</param>
    /// <param name="activateDelay">攻撃開始からコライダーが有効になるまでの遅延（秒）</param>
    void Activate(float damage, float knockbackPower, float activeDuration, float activateDelay = 0.0f);

    /// <summary>
    /// 攻撃コライダーを強制無効化する
    /// コンボリセット時などに呼ぶ
    /// </summary>
    void Deactivate();

    /// <summary>
    /// 敵参照を設定
    /// </summary>
    void SetEnemy(Enemy *enemy) { enemy_ = enemy; }

    /// <summary>
    /// プレイヤー前方へのオフセット距離を設定
    /// </summary>
    void SetForwardOffset(float offset) { forwardOffset_ = offset; }

    /// <summary>
    /// 高さオフセットを設定（プレイヤー中心からの相対Y）
    /// </summary>
    void SetHeightOffset(float offset) { heightOffset_ = offset; }

    /// <summary>
    /// コライダーのサイズを設定
    /// </summary>
    void SetColliderSize(const Vector3 &size) {
        if (collider_) {
            collider_->SetSize(size);
        }
    }

    /// <summary>
    /// 現在コライダーが有効かどうか
    /// </summary>
    bool IsActive() const { return isActive_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 衝突開始時のコールバック
    /// </summary>
    void OnCollision(ColliderBase *other);

    /// ===================================================
    /// private varians
    /// ===================================================

    Player *player_ = nullptr;
    Enemy *enemy_ = nullptr;

    // OBBコライダー本体（BaseObjectを介さず直接管理）
    OBBCollider* collider_;

    // 現在の攻撃パラメータ
    float currentDamage_ = 0.0f;
    float currentKnockback_ = 0.0f;

    // タイマー管理
    float activeTimer_ = 0.0f;    // 有効経過時間
    float activeDuration_ = 0.3f; // 有効時間（秒）
    float delayTimer_ = 0.0f;     // 遅延カウントダウン
    float activateDelay_ = 0.0f;  // 有効化遅延（秒）

    // 状態フラグ
    bool isActive_ = false;             // コライダーが判定中かどうか
    bool isPending_ = false;            // 遅延待機中かどうか
    bool hasHitThisActivation_ = false; // 1アクティブにつき1ヒットのみ許可

    // 位置パラメータ
    float forwardOffset_ = 2.25f; // プレイヤー前方へのオフセット距離
    float heightOffset_ = 0.5f;  // 高さオフセット（プレイヤー中心からのY）

    std::unique_ptr<ParticleEmitter> hitEmitter_;
    std::unique_ptr<Shake> shake_;
};
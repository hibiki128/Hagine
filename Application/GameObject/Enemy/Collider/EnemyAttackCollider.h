#pragma once
#include "Application/Utility/Shake/Shake.h"
#include "Collider/type/OBBCollider.h"
#include "Particle/ParticleEmitter.h"

class Enemy;
class Player;
namespace Hagine {
class ColliderBase;
}

/// <summary>
/// 敵前方に展開する攻撃判定コライダー
/// PlayerAttackCollider と対称の設計で、手のモーション（見た目）とは完全に独立して機能する
/// 攻撃ごとにダメージ・ノックバック・有効時間・遅延を個別設定できる
/// </summary>
class EnemyAttackCollider
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="enemy">敵参照</param>
    /// <param name="player">プレイヤー参照（後でSetPlayerでも設定可）</param>
    void Init(Enemy *enemy, Player *player = nullptr);

    /// <summary>
    /// 更新処理（毎フレーム呼び出し必須）
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// パーティクル描画
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const Hagine::ViewProjection &viewProjection);

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
    /// プレイヤー参照を設定
    /// </summary>
    void SetPlayer(Player *player) { player_ = player; }

    /// <summary>
    /// 敵前方へのオフセット距離を設定
    /// </summary>
    void SetForwardOffset(float offset) { forwardOffset_ = offset; }

    /// <summary>
    /// 高さオフセットを設定（敵中心からの相対Y）
    /// </summary>
    void SetHeightOffset(float offset) { heightOffset_ = offset; }

    /// <summary>
    /// コライダーのサイズを設定
    /// </summary>
    void SetColliderSize(const Hagine::Vector3 &size)
    {
        if (collider_)
        {
            collider_->SetSize(size);
        }
    }

    /// <summary>
    /// 現在コライダーが有効かどうか
    /// </summary>
    bool IsActive() const { return isActive_; }

    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 衝突時のコールバック
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollision(Hagine::ColliderBase *other);

    /// ===================================================
    /// private variants
    /// ===================================================

    Enemy *enemy_ = nullptr;   // 敵参照
    Player *player_ = nullptr; // プレイヤー参照

    // OBBコライダー本体
    Hagine::OBBCollider *collider_ = nullptr; // コライダー

    // 現在の攻撃パラメータ
    float currentDamage_ = 0.0f;    // 現在のダメージ
    float currentKnockback_ = 0.0f; // 現在のノックバック

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
    float forwardOffset_ = 2.25f; // 敵前方へのオフセット距離
    float heightOffset_ = 0.5f;   // 高さオフセット

    float energyRecoveryAmount_ = 5.0f;

    std::unique_ptr<Hagine::ParticleEmitter> hitEmitter_; // ヒットパーティクル
    std::unique_ptr<Shake> shake_;                        // シェイク
};
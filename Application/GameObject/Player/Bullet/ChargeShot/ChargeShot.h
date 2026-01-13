#pragma once
#include "Application/GameObject/Player/Player.h"
#include "Engine/Input/Input.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"
#include <Particle/CSParticle/ParticleCSEmitter.h>

/// <summary>
/// チャージショットのゲームオブジェクトクラス
/// プレイヤーがチャージして発射する特殊な弾
/// </summary>
class ChargeShot : public BaseObject {
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
    /// ImGui表示
    /// </summary>
    void imgui();

    /// <summary>
    /// 衝突判定時の処理
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollisionEnterCallback(ColliderBase *other);

    /// <summary>
    /// 生存状態を取得
    /// </summary>
    /// <returns>bool: 生存フラグ</returns>
    bool IsAlive() const { return isAlive_; }

    /// <summary>
    /// 最大スケールに到達したかを取得
    /// </summary>
    /// <returns>bool: 最大スケール到達フラグ</returns>
    bool IsMaxScale() const { return isMaxScale_; }

    /// <summary>
    /// スケール値を取得
    /// </summary>
    /// <returns>float: 現在のスケール値</returns>
    float GetScale() const { return scale_; }

    /// <summary>
    /// 発射済みかを取得
    /// </summary>
    /// <returns>bool: 発射済みフラグ</returns>
    bool IsFired() const { return isFired_; }

    /// <summary>
    /// チャージショットの発射処理
    /// </summary>
    /// <param name="pos">発射位置</param>
    /// <param name="dir">発射方向</param>
    void Fire(const Vector3 &pos, const Vector3 &dir);

    /// <summary>
    /// チャージショットのリセット
    /// </summary>
    void Reset();

    /// <summary>
    /// プレイヤー参照を設定
    /// </summary>
    /// <param name="player">設定するプレイヤーのポインタ</param>
    void SetPlayer(Player *player) { player_ = player; }

    /// <summary>
    /// プレイヤーの半径を設定
    /// </summary>
    /// <param name="radius">設定する半径</param>
    void SetPlayerRadius(float radius) { playerRadius_ = radius; }

    /// <summary>
    /// オフセット余裕距離を設定
    /// </summary>
    /// <param name="margin">設定する余裕距離</param>
    void SetOffsetMargin(float margin) { offsetMargin_ = margin; }

    /// <summary>
    /// 垂直方向のオフセットを設定
    /// </summary>
    /// <param name="yOffset">設定するY方向オフセット</param>
    void SetVerticalOffset(float yOffset) { verticalOffset_ = yOffset; }

    /// <summary>
    /// ダメージ値を取得
    /// </summary>
    /// <returns>int: ダメージ値</returns>
    float GetDamage() const;

    /// <summary>
    /// チャージ中かを取得
    /// </summary>
    /// <returns>bool: チャージ状態フラグ</returns>
    bool GetIsCharge() const { return isCharge; }

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    // 定数定義
    static constexpr float kInitialScale = 1.0f;                  // 初期スケール値
    static constexpr float kScaleSpeed = 1.25f;                   // スケール増加速度
    static constexpr float kMaxScale = 4.0f;                      // 最大スケール値
    static constexpr float kSpeed = 60.0f;                        // 発射速度
    static constexpr float kMaxDistance = 300.0f;                 // プレイヤーからの最大距離
    static constexpr float kMinEnergyCost = 5.0f;                 // 最小エネルギー消費量
    static constexpr float kMaxEnergyCost = 20.0f;                // 最大エネルギー消費量(5 + 15)
    static constexpr float kMinEnergyToStart = 20.0f;             // チャージ開始に必要な最小エネルギー
    static constexpr float kMinDamage = 1.0f;                     // 最小ダメージ値
    static constexpr int kMaxDamage = 7;                          // 最大ダメージ値
    static constexpr float kDefaultPlayerRadius = 1.0f;           // デフォルトのプレイヤー半径
    static constexpr float kDefaultOffsetMargin = 0.5f;           // デフォルトのオフセット余裕距離
    static constexpr float kDefaultVerticalOffset = 1.0f;         // デフォルトの垂直方向オフセット
    static constexpr float kParticleScaleBase = 0.6f;             // パーティクルスケールの基準値
    static constexpr float kParticleScaleMultiplier = 1.4f;       // パーティクルスケールの倍率
    static constexpr float kParticleEndScaleOffset = 0.2f;        // パーティクル終了時のスケールオフセット
    static constexpr float kBulletParticleScaleMultiplier = 2.0f; // 弾パーティクルのスケール倍率

    Vector3 offset_{};
    bool isAlive_ = false;    // 生存フラグ
    bool isMaxScale_ = false; // 最大スケール到達フラグ
    bool isFired_ = false;    // 発射済みフラグ
    bool isCharge = false;    // チャージ中フラグ

    float scale_ = kInitialScale;    // 現在のスケール
    float scaleSpeed_ = kScaleSpeed; // スケール増加速度
    float maxScale_ = kMaxScale;     // 最大スケール値

    Vector3 velocity_{};
    float speed_ = kSpeed; // 発射速度

    Player *player_ = nullptr;

    float playerRadius_ = kDefaultPlayerRadius;     // プレイヤーの半径
    float offsetMargin_ = kDefaultOffsetMargin;     // オフセット余裕距離
    float verticalOffset_ = kDefaultVerticalOffset; // 垂直方向のオフセット

    std::unique_ptr<ParticleCSEmitter> chargeEmitter_;
    std::unique_ptr<ParticleEmitter> bulletEmitter_;

    SphereCollider *bulletCollider_ = nullptr;
    float chargeStartTimer_ = 0.0f;
};
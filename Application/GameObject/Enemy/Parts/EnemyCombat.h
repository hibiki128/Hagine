#pragma once
#include "Application/GameObject/Enemy/Bullet/EnemyBullet.h"
#include "Application/GameObject/Enemy/Collider/EnemyAttackCollider.h"
#include "Application/Utility/Shake/Shake.h"
#include "Application/Utility/SkillCutscene/SkillCutscene.h"
#include <application/Utility/ComboSystem/ComboSystem.h>
#include <memory>
#include <string>
#include <type/Quaternion.h>
#include <vector>

class Enemy;
namespace Hagine {
class ParticleCSEmitter;
class OBBCollider;
class ViewProjection;
} // namespace Hagine

/// <summary>
/// 敵の戦闘パーツクラス
/// 通常射撃・弾管理・近接コンボ・前方攻撃判定と、
/// 大技（チャージ攻撃・ビーム必殺技）の演出＆処理を担当する
/// </summary>
class EnemyCombat
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    EnemyCombat();
    ~EnemyCombat();

    /// <summary>
    /// 初期化（弾・コンボ・攻撃コライダー・大技演出エミッタ・ビーム判定を生成する）
    /// </summary>
    /// <param name="owner">所有者の敵</param>
    void Init(Enemy *owner);

    /// <summary>
    /// 近接コンボと前方攻撃コライダーの更新
    /// </summary>
    void ConboUpdate();

    /// <summary>
    /// 大技演出（シェイク・エミッタ追従・発動前演出・ビーム）の毎フレーム更新
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void UpdateEffects(float deltaTime);

    /// <summary>
    /// 弾の更新と生存チェック
    /// </summary>
    void UpdateBullets();

    /// <summary>
    /// パーティクル描画処理（弾・攻撃コライダー・大技演出のGraphicsフェーズ）
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const Hagine::ViewProjection &viewProjection);

    /// <summary>
    /// GPUパーティクルのCompute描画処理（大技演出）
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticleCompute(const Hagine::ViewProjection &viewProjection);

    /// <summary>通常弾を1発撃つ</summary>
    void Shot();

    /// <summary>指定方向へ弾を撃つ（ホーミング指定可）</summary>
    void ShotWithDirection(const Hagine::Vector3 &direction, bool forceHoming = false);

    /// <summary>攻撃（ログ出力のみ）</summary>
    void PerformAttack();

    /// ===================================================
    /// 大技（チャージ攻撃・必殺技）演出＆処理
    /// BTノード(EnemyChargeAttackNode / EnemyUltimateNode)から駆動する
    /// ===================================================

    /// <summary>チャージ溜めオーラ演出を開始する</summary>
    void StartChargeAura();
    /// <summary>チャージ溜めオーラ演出を停止する</summary>
    void StopChargeAura();
    /// <summary>チャージ発射: 閃光演出＋強化ホーミング弾を1発撃つ</summary>
    void FireChargeBlast();

    /// <summary>ビーム必殺技を起動する。溜め完了後にBTノードから呼ぶ</summary>
    void ActivateBeam();
    /// <summary>ビーム必殺技を強制停止する（BT中断時など）</summary>
    void DeactivateBeam();
    /// <summary>ビームが現在アクティブかどうか</summary>
    bool IsBeamActive() const { return beamActive_; }

    /// <summary>
    /// ビーム必殺技の発動前演出（カメラ顔アップ→通常カメラ復帰→遅延→発動）を開始する。
    /// </summary>
    void StartBeamStaging();
    /// <summary>ビーム発動前演出を中断する（BT中断時など）</summary>
    void CancelBeamStaging();
    /// <summary>ビーム発動前演出中かどうか</summary>
    bool IsBeamStaging() const { return beamCutscene_.IsActive(); }

    /// <summary>
    /// シェイク初期化（ビュープロジェクション設定）
    /// </summary>
    void SetVp(Hagine::ViewProjection *vp);

    /// <summary>
    /// 被弾ヒット時のシェイクを開始する（強化弾・必殺技被弾用）
    /// </summary>
    void StartHitShake()
    {
        if (chargeShake_)
        {
            chargeShake_->StartShake();
        }
    }

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

    /// <summary>
    /// コンボパラメータのImGui表示（デバッグタブ用）
    /// </summary>
    void DrawComboImGui();

    /// ===================================================
    /// Getter
    /// ===================================================
    EnemyAttackCollider *GetAttackCollider() { return attackCollider_.get(); }
    ComboSystem &GetPunchCombo() { return punchCombo_; }
    const std::vector<std::string> &GetComboAnimations() const { return comboAnimations_; }
    int GetPunchComboLength() const { return punchCombo_.GetComboLength(); }
    bool IsPunchComboActive() const { return punchCombo_.IsComboActive(); }
    float GetCurrentAttackDamage() const { return currentAttackDamage_; }
    float GetCurrentAttackKnockback() const { return currentAttackKnockback_; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetComboAttack(bool flag) { isComboAttack_ = flag; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>大技演出エミッタの追従＆更新</summary>
    void UpdateEmitters();

    /// <summary>ビーム必殺技の毎フレーム更新</summary>
    void UpdateBeam();

    /// ===================================================
    /// private variants
    /// ===================================================

    // 弾丸関連定数
    static constexpr float kBulletScale = 0.5f;
    static constexpr float kBulletColliderRadius = 0.5f;
    static constexpr float kNormalShotEnergyCost = 5.0f;
    static constexpr float kMinRotationDistance = 0.001f;

    // 大技関連定数
    static constexpr float kChargeAttackEnergyCost = 30.0f; // チャージ攻撃の消費エネルギー
    static constexpr float kChargeBlastScale = 1.6f;        // チャージ弾のスケール
    static constexpr float kChargeBlastDamage = 15.0f;      // チャージ弾のダメージ
    static constexpr float kChargeBlastSpeed = 60.0f;       // チャージ弾の速度
    static constexpr float kUltimateEnergyCost = 60.0f;     // ビーム必殺技の消費エネルギー
    static constexpr float kBeamDamage = 40.0f;             // ビーム必殺技のダメージ
    static constexpr float kBeamMaxLength = 55.0f;          // ビームの最大長さ
    static constexpr float kBeamExtendSpeed = 110.0f;       // ビームが伸びる速度
    static constexpr float kBeamWidth = 2.5f;               // ビーム幅
    static constexpr float kBeamDuration = 2.0f;            // ビーム持続時間(秒)
    static constexpr float kLockOffsetY = 2.5f;             // ビーム発射時のYオフセット（敵中心からの相対値）

    Enemy *owner_ = nullptr; ///< 所有者の敵

    std::vector<std::unique_ptr<EnemyBullet>> bullets_; ///< 敵の弾

    // 前方攻撃判定コライダー（PlayerAttackColliderと対称の設計）
    std::unique_ptr<EnemyAttackCollider> attackCollider_; ///< 攻撃コライダー

    ComboSystem punchCombo_;                   ///< パンチコンボシステム
    bool comboInitialized_ = false;            ///< コンボ初期化済みフラグ
    bool isComboAttack_ = false;               ///< コンボ攻撃中フラグ
    std::vector<std::string> comboAnimations_; ///< コンボ段ごとの本体アニメーションパス

    // コンボ攻撃パラメータ（ComboSystemのコールバックで更新）
    float currentAttackDamage_ = 10.0f;   ///< 現在の攻撃ダメージ量
    float currentAttackKnockback_ = 3.0f; ///< 現在の攻撃ノックバック強度
    float currentAttackDuration_ = 0.25f; ///< 現在の攻撃有効時間

    std::unique_ptr<Shake> chargeShake_; ///< シェイク

    // 大技演出用CSパーティクル
    std::unique_ptr<Hagine::ParticleCSEmitter> chargeAura_;       ///< チャージ溜めオーラ (enemyChargeAura)
    std::unique_ptr<Hagine::ParticleCSEmitter> beamMainEffect_;   ///< ビームメイン演出 (makan_main)
    std::unique_ptr<Hagine::ParticleCSEmitter> beamAroundEffect_; ///< ビームらせん演出 (makan_around)

    // ビーム必殺技状態
    Hagine::OBBCollider *beamCollider_ = nullptr; ///< ビーム判定コライダー（動的にOBBを更新）
    bool beamActive_ = false;                     ///< ビームアクティブフラグ
    bool beamDamageDealt_ = false;                ///< ビームダメージ適用済みフラグ
    float beamLength_ = 0.0f;                     ///< 現在のビーム長
    float beamActiveTime_ = 0.0f;                 ///< ビームアクティブ経過時間
    float beamSpiralTime_ = 0.0f;                 ///< らせんアニメーション経過時間
    Hagine::Quaternion beamLockedRotation_{};     ///< ビーム発射時に固定した向き（発射後ホーミング防止）
    SkillCutscene beamCutscene_;                  ///< ビーム発動前演出（カメラ顔アップ＋発動遅延）
    float beamSpiralRadius_ = 2.0f;               ///< らせんの半径
    float beamSpiralRevolution_ = 3.0f;           ///< 最大長に達した時の巻き数
    float beamSpiralForwardSpeed_ = 30.0f;        ///< らせんパーティクルの前進速度
};

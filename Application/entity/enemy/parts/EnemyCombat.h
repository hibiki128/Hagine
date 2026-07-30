#pragma once
#include "Application/entity/enemy/bullet/EnemyBullet.h"
#include "Application/entity/enemy/collider/EnemyAttackCollider.h"
#include "Application/utility/shake/Shake.h"
#include "Application/utility/cutscene/SkillCutscene.h"
#include <application/utility/combo/ComboSystem.h>
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
    /// <param name="pOwner">所有者の敵</param>
    void Init(Enemy *pOwner);

    /// <summary>
    /// 近接コンボと前方攻撃コライダーの更新
    /// </summary>
    void ComboUpdate();

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
    void SetViewProjection(Hagine::ViewProjection *pViewProjection);

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

    /// <summary>
    /// ビームの発射起点を取得する
    /// 必殺技モーションは両手を前に突き出すため、両手ジョイントの中点を使う
    /// （取得できない場合は従来のYオフセットで代用）
    /// </summary>
    /// <returns>Vector3: 発射起点のワールド座標</returns>
    Hagine::Vector3 GetBeamOrigin();

    /// ===================================================
    /// private variables
    /// ===================================================

    // コンボ定義（jsons/ComboDefinition 以下のファイル名。拡張子なし）
    static constexpr const char *kComboDefinitionName = "EnemyPunchCombo";

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
    // 必殺技モーション（MakanSkill.gltf・30fps）の発射(30フレーム目)〜撃ち終わり(80フレーム目)
    // に合わせた持続時間（50フレーム）
    static constexpr float kBeamDuration = 50.0f / 30.0f;
    static constexpr float kLockOffsetY = 2.5f;                               // ビーム発射時のYオフセット（ジョイント取得失敗時の代用）
    static constexpr const char *kRightHandJointName = "mixamorig:RightHand"; // 発射起点に使う右手ジョイント名
    static constexpr const char *kLeftHandJointName = "mixamorig:LeftHand";   // 発射起点に使う左手ジョイント名

    Enemy *pOwner_ = nullptr; ///< 所有者の敵

    std::vector<std::unique_ptr<EnemyBullet>> bullets_; ///< 敵の弾

    // 前方攻撃判定コライダー（PlayerAttackColliderと対称の設計）
    std::unique_ptr<EnemyAttackCollider> attackCollider_; ///< 攻撃コライダー

    ComboSystem punchCombo_;        ///< パンチコンボシステム
    bool comboInitialized_ = false; ///< コンボ初期化済みフラグ
    bool isComboAttack_ = false;    ///< コンボ攻撃中フラグ

    // コンボ攻撃パラメータ（ComboSystemのコールバックで更新）
    float currentAttackDamage_ = 10.0f;   ///< 現在の攻撃ダメージ量
    float currentAttackKnockback_ = 3.0f; ///< 現在の攻撃ノックバック強度
    float currentAttackDuration_ = 0.25f; ///< 現在の攻撃有効時間

    // ─── コンボの吹き飛ばし段（GameParamで調整可・0で無効）───
    // 途中の段で吹き飛ばすとコンボが途切れるため、既定では締めの段だけ吹き飛ばす
    int blowLaunchStage_ = 0;             ///< 前方へ大きく吹き飛ばす段（1始まり）
    float blowLaunchKnockback_ = 22.0f;   ///< 吹き飛ばし段のノックバック強度
    int blowFinisherStage_ = 8;           ///< 地面へ叩きつける締めの段（1始まり）
    float blowFinisherKnockback_ = 30.0f; ///< 叩きつけ段の下方向ノックバック強度

    std::unique_ptr<Shake> chargeShake_; ///< シェイク

    // 大技演出用CSパーティクル。実体は ParticleCSSpawner が所有する借用ポインタで、
    // 更新・描画はエンジンが自動で回し、シーン遷移時にまとめて破棄される。
    Hagine::ParticleCSEmitter *pChargeAura_ = nullptr;       ///< チャージ溜めオーラ (enemyChargeAura)
    Hagine::ParticleCSEmitter *pBeamMainEffect_ = nullptr;   ///< ビームメイン演出 (makan_main)
    Hagine::ParticleCSEmitter *pBeamAroundEffect_ = nullptr; ///< ビームらせん演出 (makan_around)

    // ビーム必殺技状態
    Hagine::OBBCollider *pBeamCollider_ = nullptr; ///< ビーム判定コライダー（動的にOBBを更新）
    Hagine::Vector3 beamOrigin_{};                ///< ビーム発射起点（手ジョイント位置。コライダーの中心にも使う）
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

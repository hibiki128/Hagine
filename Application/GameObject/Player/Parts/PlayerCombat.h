#pragma once
#include "Application/GameObject/Player/Bullet/PlayerBullet.h"
#include "Application/GameObject/Player/Collider/PlayerAttackCollider.h"
#include "Application/GameObject/Player/Skill/MakanAttackSkill.h"
#include "Application/Utility/SkillCutscene/SkillCutscene.h"
#include <application/Utility/ComboSystem/ComboSystem.h>
#include <memory>
#include <string>
#include <vector>

class Player;
class ChargeShot;
namespace Hagine {
class DataHandler;
}

/// <summary>
/// プレイヤーの戦闘パーツクラス
/// 射撃・チャージショット・近接コンボ・必殺技・弾管理を担当する
/// </summary>
class PlayerCombat {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    PlayerCombat();
    ~PlayerCombat();

    /// <summary>
    /// 初期化（チャージショット・必殺技・コンボ・攻撃コライダーを生成する）
    /// </summary>
    /// <param name="owner">所有者のプレイヤー</param>
    void Init(Player *owner);

    /// <summary>
    /// 近接コンボと前方攻撃コライダーの更新
    /// </summary>
    void UpdateComboAndCollider();

    /// <summary>
    /// チャージショットの更新（入力表示UI用の通知も行う）
    /// </summary>
    void UpdateChargeShot();

    /// <summary>
    /// 必殺技の発動前演出（カメラ顔アップ＋発動遅延）の更新
    /// </summary>
    void UpdateSkillCutscene();

    /// <summary>
    /// 射撃処理（弾の発射入力判定と既存弾の更新）
    /// </summary>
    void Shot();

    /// <summary>
    /// 必殺技（魔貫攻撃）の発動入力判定
    /// 入力後すぐには発動せず、カメラ演出→遅延を経て発動する
    /// </summary>
    void SkillShot();

    /// <summary>
    /// 描画処理（弾・チャージショット）
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void Draw(const Hagine::ViewProjection &viewProjection);

    /// <summary>
    /// チャージショットのパーティクル描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawChargeParticle(const Hagine::ViewProjection &viewProjection);

    /// <summary>
    /// 弾・攻撃コライダー・必殺技のパーティクル描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawAttackParticles(const Hagine::ViewProjection &viewProjection);

    /// <summary>
    /// GPUパーティクルのCompute描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticleCompute(const Hagine::ViewProjection &viewProjection);

    /// <summary>
    /// 戦闘関連パラメータを保存する
    /// </summary>
    /// <param name="data">保存先のデータハンドラ</param>
    void Save(Hagine::DataHandler *data);

    /// <summary>
    /// 戦闘関連パラメータを読み込む
    /// </summary>
    /// <param name="data">読み込み元のデータハンドラ</param>
    void Load(Hagine::DataHandler *data);

    /// <summary>
    /// 弾関連のImGui表示（プレイヤータブ内に置く分）
    /// </summary>
    void DrawBulletImGui();

    /// <summary>
    /// コンボ・必殺技関連のImGui表示（タブ外に置く分）
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

    /// <summary>
    /// このフレームに近接攻撃が発火していれば true を返し、その段名を out に格納する。
    /// 取得すると内部フラグはクリアされる（1発火につき1回だけ true）。入力表示UI用。
    /// </summary>
    bool ConsumeMeleeAttackFired(std::string &outName) {
        if (!meleeAttackFired_) {
            return false;
        }
        meleeAttackFired_ = false;
        outName = lastMeleeAttackName_;
        return true;
    }

    /// ===================================================
    /// Getter
    /// ===================================================
    std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() { return bullets_; }
    PlayerAttackCollider *GetAttackCollider() { return attackCollider_.get(); }
    ChargeShot *GetChargeShot() { return chargeShot_.get(); }
    ComboSystem &GetPunchCombo() { return punchCombo_; }
    const std::vector<std::string> &GetComboAnimations() const { return comboAnimations_; }

    /// <summary>必殺技（ビーム）が発動中か</summary>
    bool IsSkillActive() const { return makanAttack_ptr_ && makanAttack_ptr_->IsActive(); }

    /// <summary>必殺技の発動前演出（カメラ演出＋遅延）中か</summary>
    bool IsSkillStaging() const { return skillCutscene_.IsActive(); }

    /// <summary>チャージショットの溜め中か</summary>
    bool IsCharging() const;

    bool GetIsSkillMenu() const { return isSkillMenu_; }
    float GetChargeThreshold() const { return kYButtonChargeThreshold; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetSkillMenu(bool flag) { isSkillMenu_ = flag; }

    /// <summary>
    /// チャージショットの溜め操作を一時ロックする（必殺技カメラ演出中など）。
    /// ロック中でも UpdateChargeShot() は呼び続けることで溜め演出のエミッタ管理が継続し、
    /// 演出が消えなくなるのを防ぐ。
    /// </summary>
    /// <param name="locked">ロックするなら true</param>
    void SetChargeActionLocked(bool locked);

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 近接コンボの更新と入力判定
    /// </summary>
    void ComboUpdate();

    /// <summary>
    /// 通常弾を1発生成して発射する
    /// </summary>
    void FireNormalBullet();

    /// <summary>
    /// 必殺技の発動前演出を開始する（エネルギー消費済みの前提）
    /// </summary>
    void StartSkillStaging();

    /// ===================================================
    /// private variants
    /// ===================================================

    // 弾丸関連定数
    static constexpr float kBulletScale = 0.5f;
    static constexpr float kBulletColliderRadius = 0.5f;
    static constexpr float kNormalShotEnergyCost = 5.0f;
    static constexpr float kSkillShotEnergyCost = 65.0f;

    Player *owner_ = nullptr; ///< 所有者のプレイヤー

    std::vector<std::unique_ptr<PlayerBullet>> bullets_;   ///< 発射した弾
    std::unique_ptr<ChargeShot> chargeShot_;               ///< チャージショット
    std::unique_ptr<PlayerAttackCollider> attackCollider_; ///< 前方攻撃判定
    MakanAttackSkill *makanAttack_ptr_ = nullptr;          ///< 必殺技（所有権はBaseObjectManager）

    ComboSystem punchCombo_;                   ///< パンチコンボ
    bool comboInitialized_ = false;            ///< コンボ初期化済みフラグ
    std::vector<std::string> comboAnimations_; ///< コンボ段ごとのプレイヤー本体アニメーションパス

    SkillCutscene skillCutscene_; ///< 必殺技の発動前演出（カメラ顔アップ＋発動遅延）

    float B_acce_ = 0.0f;  ///< 弾の加速度
    float B_speed_ = 0.0f; ///< 弾の速度

    float yButtonHoldTime_ = 0.0f;               ///< Yボタン押下時間
    const float kYButtonChargeThreshold = 0.15f; ///< チャージ判定閾値(秒)

    bool isSkillMenu_ = false;   ///< スキルメニュー（LT押下）中フラグ
    bool prevChargeState_ = false; ///< チャージ開始のエッジ検出用

    // 入力表示UI用: 近接攻撃の発火通知（発火コールバックで設定し、Consumeでクリア）
    bool meleeAttackFired_ = false;   ///< このフレームに近接攻撃が発火したか
    std::string lastMeleeAttackName_; ///< 直近に発火した近接攻撃の段名
};

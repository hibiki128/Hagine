#pragma once
#include "Application/entity/player/bullet/PlayerBullet.h"
#include "Application/entity/player/collider/PlayerAttackCollider.h"
#include "Application/entity/player/finisher/ComboFinisher.h"
#include "Application/entity/player/skill/MakanAttackSkill.h"
#include "Application/utility/cutscene/SkillCutscene.h"
#include <application/utility/combo/ComboSystem.h>
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
class PlayerCombat
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    PlayerCombat();
    ~PlayerCombat();

    /// <summary>
    /// 初期化（チャージショット・必殺技・コンボ・攻撃コライダーを生成する）
    /// </summary>
    /// <param name="pOwner">所有者のプレイヤー</param>
    void Init(Player *pOwner);

    /// <summary>
    /// 近接コンボの進行と入力判定の更新
    /// </summary>
    void UpdateCombo();

    /// <summary>
    /// 前方攻撃コライダーの更新（判定の有効時間・遅延とヒット時のカメラシェイク）
    /// 攻撃できない状況でもシェイクを進める必要があるため、毎フレーム呼ぶ
    /// （途中で止めるとカメラのずれが戻らず、画面がずれたまま固まる）
    /// </summary>
    void UpdateAttackCollider();

    /// <summary>
    /// チャージショットの更新（入力表示UI用の通知も行う）
    /// </summary>
    void UpdateChargeShot();

    /// <summary>
    /// 必殺技の発動前演出（カメラ顔アップ＋発動遅延）の更新
    /// </summary>
    void UpdateSkillCutscene();

    /// <summary>
    /// 射撃処理（弾の発射入力判定）
    /// 近接コンボ中の入力はコンボ派生技の発動に振り分けられる
    /// </summary>
    void Shot();

    /// <summary>
    /// 発射済みの弾の更新と消滅判定
    /// 発射できない状況（被弾リアクション中・派生技の演出中など）でも飛んでいる弾は
    /// 進み続ける必要があるため、Shot() とは分けて毎フレーム呼ぶ
    /// </summary>
    void UpdateBullets();

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
    /// 戦闘関連パラメータを保存する
    /// </summary>
    /// <param name="pData">保存先のデータハンドラ</param>
    void Save(Hagine::DataHandler *pData);

    /// <summary>
    /// 戦闘関連パラメータを読み込む
    /// </summary>
    /// <param name="pData">読み込み元のデータハンドラ</param>
    void Load(Hagine::DataHandler *pData);

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

    /// ===================================================
    /// 瞬間移動コンボ（吹き飛ばし→瞬間移動追撃→叩きつけ）
    /// ===================================================

    /// <summary>瞬間移動追撃中か（Playerの行動ロック判定用）</summary>
    bool IsTeleporting() const { return teleportActive_; }

    /// <summary>
    /// 瞬間移動追撃の位置固定・向き・消える演出・カメラ制御を進める（毎フレーム呼ぶ）
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void UpdateTeleport(float deltaTime);

    /// <summary>
    /// 瞬間移動追撃を開始する（吹き飛ばし段のヒット時）。少量のエネルギーを消費する
    /// </summary>
    void StartTeleportChase();

    /// <summary>瞬間移動追撃を継続する（追撃段のヒット時。貼り付き時間を再充填）</summary>
    void RefreshTeleportChase();

    /// <summary>瞬間移動追撃を終了する（叩きつけ段のヒット時・敵消滅・時間切れ）</summary>
    void EndTeleportChase();

    /// ===================================================
    /// コンボ派生技（近接コンボ中の射撃入力で出る演出付きの技）
    /// ===================================================

    /// <summary>
    /// コンボ派生技の進行とクールダウンを進める（毎フレーム呼ぶ）
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void UpdateFinisher(float deltaTime) { finisher_.Update(deltaTime); }

    /// <summary>コンボ派生技が発動中か（Playerの行動ロック判定用）</summary>
    bool IsFinisherActive() const { return finisher_.IsActive(); }

    /// <summary>コンボ派生技を中断する（被弾時などに呼ぶ）</summary>
    void CancelFinisher() { finisher_.Cancel(); }

    /// <summary>
    /// コンボ派生技中に再生する全身アニメーションのクリップ名を取得する
    /// </summary>
    /// <returns>const std::string&amp;: クリップ名（未発動なら空文字）</returns>
    const std::string &GetFinisherAnimationClip() const { return finisher_.GetAnimationClip(); }

    /// <summary>
    /// このフレームに近接攻撃が発火していれば true を返し、その段名を out に格納する。
    /// 取得すると内部フラグはクリアされる（1発火につき1回だけ true）。入力表示UI用。
    /// </summary>
    bool ConsumeMeleeAttackFired(std::string &outName)
    {
        if (!meleeAttackFired_)
        {
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

    /// <summary>必殺技（ビーム）が発動中か</summary>
    bool IsSkillActive() const { return pMakanAttack_ && pMakanAttack_->IsActive(); }

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
    /// <param name="forceHoming">ロックオンしていなくても追尾させるなら true（派生技の連射用）</param>
    void FireNormalBullet(bool forceHoming = false);

    /// <summary>
    /// 射撃ボタンが押された瞬間かどうかを判定する
    /// （キーボードはトリガー、ゲームパッドはYボタンの短押し離しで発火）
    /// </summary>
    /// <returns>bool: 押された瞬間なら true</returns>
    bool IsShotTriggered();

    /// <summary>
    /// 射撃入力の1回分を処理する。
    /// 近接コンボ中ならコンボ派生技へ、そうでなければ通常弾の発射へ振り分ける
    /// </summary>
    void HandleShotInput();

    /// <summary>
    /// 必殺技の発動前演出を開始する（エネルギー消費済みの前提）
    /// </summary>
    void StartSkillStaging();

    /// ===================================================
    /// private variables
    /// ===================================================

    // コンボ定義（jsons/ComboDefinition 以下のファイル名。拡張子なし）
    static constexpr const char *kComboDefinitionName = "PunchCombo";

    // ImGuiでアニメーションパスを編集する入力欄のバッファサイズ
    static constexpr int kAnimationPathBufferSize = 256;

    // 弾丸関連定数
    static constexpr float kBulletScale = 0.5f;
    static constexpr float kBulletColliderRadius = 0.5f;
    static constexpr float kNormalShotEnergyCost = 5.0f;
    static constexpr float kSkillShotEnergyCost = 65.0f;

    Player *pOwner_ = nullptr; ///< 所有者のプレイヤー

    std::vector<std::unique_ptr<PlayerBullet>> bullets_;   ///< 発射した弾
    std::unique_ptr<ChargeShot> chargeShot_;               ///< チャージショット
    std::unique_ptr<PlayerAttackCollider> attackCollider_; ///< 前方攻撃判定
    MakanAttackSkill *pMakanAttack_ = nullptr;          ///< 必殺技（所有権はBaseObjectManager）

    ComboSystem punchCombo_;        ///< パンチコンボ
    bool comboInitialized_ = false; ///< コンボ初期化済みフラグ

    ComboFinisher finisher_; ///< 近接コンボからの派生技（演出付き）

    SkillCutscene skillCutscene_; ///< 必殺技の発動前演出（カメラ顔アップ＋発動遅延）

    float bulletAcceleration_ = 0.0f;  ///< 弾の加速度
    float bulletSpeed_ = 0.0f; ///< 弾の速度

    float yButtonHoldTime_ = 0.0f;               ///< Yボタン押下時間
    const float kYButtonChargeThreshold = 0.15f; ///< チャージ判定閾値(秒)

    bool isSkillMenu_ = false;     ///< スキルメニュー（LT押下）中フラグ
    bool prevChargeState_ = false; ///< チャージ開始のエッジ検出用

    // 入力表示UI用: 近接攻撃の発火通知（発火コールバックで設定し、Consumeでクリア）
    bool meleeAttackFired_ = false;   ///< このフレームに近接攻撃が発火したか
    std::string lastMeleeAttackName_; ///< 直近に発火した近接攻撃の段名

    /// <summary>
    /// 敵を横方向へ大きく吹き飛ばし、その先へプレイヤーを先回り瞬間移動させる。
    /// StartTeleportChase / RefreshTeleportChase から呼ぶ内部処理
    /// </summary>
    void TeleportAhead();

    // ─── 瞬間移動コンボの調整パラメータ（GameParamで調整可）───
    bool teleportEnabled_ = true;          ///< 瞬間移動コンボの有効フラグ
    int teleportLaunchStage_ = 5;          ///< 大吹き飛ばし＋瞬間移動を始める段（1始まり）
    int teleportSlamStage_ = 8;            ///< 地面へ叩きつける最終段（1始まり）
    float teleportEnergyCost_ = 8.0f;      ///< 瞬間移動で消費するエネルギー
    float teleportAheadDistance_ = 45.0f;  ///< 敵を吹き飛ばす距離＝プレイヤーが先回りする距離（速度=距離/到達時間なので距離を伸ばすほど速く遠くへ飛ぶ）
    float teleportArrivalTime_ = 0.6f;     ///< 吹き飛ばした敵が先回り地点へ到達するまでの時間（秒。小さいほど速く吹き飛ぶ）
    float teleportLaunchUp_ = 0.5f;        ///< 吹き飛ばし時の上方向成分（横方向主体・小さめ。大きいと追撃のたびに上へ漂う）
    float teleportSlamKnockback_ = 45.0f;  ///< 叩きつけ段の下方向ノックバック強度
    float teleportFollowDistance_ = 3.0f;  ///< 敵がこの距離まで到達したら滑走を止める（攻撃間合い）
    float teleportPinDuration_ = 0.9f;     ///< 追撃を保つ最大時間（安全弁。次段が来なければ終了）
    float teleportVanishDuration_ = 0.15f; ///< 消える演出の時間（秒）
    float teleportCameraHold_ = 0.3f;      ///< カメラを旧位置に留める時間（秒）

    // ─── 瞬間移動コンボの実行状態 ───
    bool teleportActive_ = false;       ///< 瞬間移動追撃中フラグ
    float teleportTimer_ = 0.0f;        ///< 最後のヒットからの経過時間
    float teleportVanishTimer_ = 0.0f;  ///< 消える演出の経過時間
    Hagine::Vector3 teleportDir_ = {0.0f, 0.0f, 1.0f}; ///< 吹き飛ばし＆先回りの方向（水平）
};

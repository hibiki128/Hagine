#pragma once
#include "Application/camera/follow/parts/CameraFinisher.h"
#include "Application/utility/shake/Shake.h"
#include <functional>
#include <memory>
#include <string>
#include <type/Vector3.h>

class Player;
class Enemy;
namespace Hagine {
class DataHandler;
}

/// <summary>
/// コンボ派生技の種別
/// 近接コンボを何段目まで繋いだ状態で射撃ボタンを押したかで分岐する
/// </summary>
enum class FinisherKind
{
    None,          ///< 発動していない
    BlastRush,     ///< 打ち上げ→空中連射→追い討ち
    TeleportSmash, ///< 吹き飛ばし→瞬間移動で先回り連撃→地面へ叩き落とし
    MeteorDrive,   ///< 真下へ叩きつけ→上空から急降下→ゼロ距離砲
};

/// <summary>
/// 近接コンボからの派生技（フィニッシャー）を管理するクラス
/// 近接コンボの途中で射撃ボタンを押すと、通常弾の代わりに段数に応じた
/// 演出付きの派生技が発動する。フェーズごとに敵への干渉・カメラワーク・
/// 画面演出を進行させ、終了後はクールダウンで連発を防ぐ。
/// はめ殺しにならないよう、締めの一撃で必ず敵にひるみ無効時間を与えて仕切り直す
/// </summary>
class ComboFinisher
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（カメラシェイクの生成を行う）
    /// </summary>
    /// <param name="pOwner">所有者のプレイヤー</param>
    void Init(Player *pOwner);

    /// <summary>
    /// 派生技の発動を試みる（近接コンボ中の射撃入力時に呼ぶ）
    /// </summary>
    /// <param name="executedStepCount">現在のコンボで繰り出した段数（1段目を出したら1）</param>
    /// <returns>bool: 発動できたら true（false なら通常射撃を行ってよい）</returns>
    bool TryStart(int executedStepCount);

    /// <summary>
    /// 更新処理（毎フレーム呼ぶ。非発動中はクールダウンだけを進める）
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// 発動中の派生技を中断する（被弾・敵の消滅・シーン終了時に呼ぶ）
    /// </summary>
    void Cancel();

    /// <summary>
    /// 弾を1発生成して発射するコールバックを設定する（連射フェーズで使う）
    /// </summary>
    /// <param name="callback">弾を発射する処理</param>
    void SetFireBulletCallback(std::function<void()> callback) { onFireBullet_ = std::move(callback); }

    /// <summary>
    /// 派生技関連パラメータを保存する
    /// </summary>
    /// <param name="pData">保存先のデータハンドラ</param>
    void Save(Hagine::DataHandler *pData);

    /// <summary>
    /// 派生技関連パラメータを読み込む
    /// </summary>
    /// <param name="pData">読み込み元のデータハンドラ</param>
    void Load(Hagine::DataHandler *pData);

    /// <summary>
    /// 派生技関連のImGui表示
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

    /// ===================================================
    /// Getter
    /// ===================================================

    /// <summary>派生技が発動中か（プレイヤーの行動ロック判定に使う）</summary>
    bool IsActive() const { return kind_ != FinisherKind::None; }

    /// <summary>発動中の派生技の種別</summary>
    FinisherKind GetKind() const { return kind_; }

    /// <summary>
    /// 現在のフェーズで再生する全身アニメーションのクリップ名を取得する。
    /// PlayerVisual が派生技中のモーションとして参照する
    /// </summary>
    /// <returns>const std::string&amp;: クリップ名（未設定なら空文字）</returns>
    const std::string &GetAnimationClip() const { return animationClip_; }

    /// <summary>クールダウンの残り時間（秒）。入力表示・デバッグ用</summary>
    float GetCooldownRemain() const { return cooldownTimer_; }

  private:
    /// ===================================================
    /// private struct
    /// ===================================================

    /// <summary>
    /// 派生技1フェーズ分の定義
    /// </summary>
    struct FinisherPhase
    {
        const char *animationClip; ///< そのフェーズ中に再生する全身クリップ名
        float duration;            ///< 継続時間（秒）
    };

    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 発動する派生技の種別を段数から決める
    /// </summary>
    /// <param name="executedStepCount">繰り出した段数</param>
    /// <returns>FinisherKind: 発動する種別（条件を満たさなければ None）</returns>
    FinisherKind SelectKind(int executedStepCount) const;

    /// <summary>
    /// 現在の種別のフェーズ表を取得する
    /// </summary>
    /// <param name="outCount">フェーズ数の格納先</param>
    /// <returns>const FinisherPhase*: フェーズ表の先頭（種別が None なら nullptr）</returns>
    const FinisherPhase *GetPhaseTable(int &outCount) const;

    /// <summary>次のフェーズへ進める（最後まで進んだら技を終了する）</summary>
    void AdvancePhase();

    /// <summary>
    /// フェーズ開始時の1回だけの処理（吹き飛ばし・瞬間移動・カメラ切替など）
    /// </summary>
    void OnPhaseEnter();

    /// <summary>
    /// フェーズ中の毎フレーム処理（位置の固定・連射の間隔管理など）
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void OnPhaseUpdate(float deltaTime);

    /// <summary>派生技を終了して通常状態へ戻す（クールダウンを開始する）</summary>
    void Finish();

    /// ===================================================
    /// private method（各フェーズの実処理）
    /// ===================================================

    void EnterBlastRush();     ///< 打ち上げ連射のフェーズ開始処理
    void EnterTeleportSmash(); ///< 瞬間移動連撃のフェーズ開始処理
    void EnterMeteorDrive();   ///< 急降下ゼロ距離砲のフェーズ開始処理

    void UpdateBlastRush(float deltaTime);     ///< 打ち上げ連射のフェーズ更新処理
    void UpdateTeleportSmash(float deltaTime); ///< 瞬間移動連撃のフェーズ更新処理
    void UpdateMeteorDrive(float deltaTime);   ///< 急降下ゼロ距離砲のフェーズ更新処理

    /// ===================================================
    /// private method（共通の小道具）
    /// ===================================================

    /// <summary>技の対象となる敵を取得する（生存していなければ nullptr）</summary>
    Enemy *GetTargetEnemy() const;

    /// <summary>
    /// プレイヤーを指定位置へ瞬間移動させ、敵の方を向かせる
    /// （以降その位置に固定されるので、カメラのカットは呼び出し側で行う）
    /// </summary>
    /// <param name="position">移動先のワールド座標</param>
    /// <param name="holdCamera">
    /// 演出カメラを使わない場合に、通常カメラを一瞬その場へ留めてからスナップさせるなら true。
    /// 長距離の瞬間移動でカメラがワープ追従してガクつくのを防ぐ
    /// </param>
    void TeleportPlayer(const Hagine::Vector3 &position, bool holdCamera = true);

    /// <summary>
    /// プレイヤーを固定位置に留める（速度を消して敵の方を向き続ける）。
    /// 派生技中はステート更新が止まるため、毎フレーム呼んで姿勢を維持する
    /// </summary>
    void PinPlayer();

    /// <summary>
    /// 敵へダメージを与え、指定の速度で吹き飛ばす
    /// </summary>
    /// <param name="damage">与えるダメージ量</param>
    /// <param name="velocity">吹き飛ばし速度（そのまま代入する）</param>
    /// <param name="grantsFlinchImmunity">復帰後にひるみ無効時間を与えるなら true（締めの一撃用）</param>
    void HitEnemy(float damage, const Hagine::Vector3 &velocity, bool grantsFlinchImmunity = false);

    /// <summary>敵を空中に留める（落下速度を緩めて滞空させる。ジャグル演出用）</summary>
    /// <param name="hoverFallSpeed">許容する下向き速度（負値）</param>
    void HoldEnemyAirborne(float hoverFallSpeed);

    /// <summary>
    /// 吹き飛ばしリアクションの予約を張り直す（毎フレーム呼ぶ）。
    /// 連射の弾など演出外のダメージが入っても「ひるみ」に落ちて相手が復帰しないようにする。
    /// 予約はダメージの無いフレームに敵側で落とされるため、毎フレーム掛け直す必要がある
    /// </summary>
    void RefreshEnemyBlowReaction();

    /// <summary>
    /// 敵から見た水平方向の位置を求める（叩き込む先や回り込み先の算出に使う）
    /// </summary>
    /// <param name="direction">敵からの水平方向（正規化不要）</param>
    /// <param name="distance">敵からの距離</param>
    /// <param name="heightOffset">敵の位置からの高さオフセット</param>
    /// <returns>Vector3: 算出したワールド座標</returns>
    Hagine::Vector3 GetPositionAroundEnemy(const Hagine::Vector3 &direction, float distance,
                                           float heightOffset) const;

    /// <summary>プレイヤーから敵へ向かう水平方向を取得する（求まらなければ直前の方向）</summary>
    Hagine::Vector3 GetAttackDirection();

    /// <summary>
    /// 出際の一撃を出す位置（敵のすぐ手前）へ詰め寄る。
    /// 瞬間移動追撃からの派生など、離れた位置から始動しても密着した絵になるようにする
    /// </summary>
    void CloseInOnEnemy();

    /// <summary>
    /// カメラのアングルを切り替える（演出カメラが無効なら何もしない）
    /// </summary>
    /// <param name="style">切り替え先のアングル種別</param>
    /// <param name="isCut">補間せず瞬時に切り替えるなら true</param>
    /// <param name="rollDegrees">画面の傾き（度）</param>
    void SetCameraStyle(FinisherCameraStyle style, bool isCut, float rollDegrees);

    /// <summary>
    /// 画面フラッシュ（ポストエフェクト）を鳴らす（無効なら何もしない）
    /// </summary>
    void TriggerScreenFlash();

    /// ===================================================
    /// private variables
    /// ===================================================

    // ─── 定数 ───
    static constexpr float kEpsilon = 0.001f;                ///< ゼロ除算を避けるための微小値
    static constexpr const char *kImpactShakeName = "finisherImpact"; ///< 中程度のシェイク設定名
    static constexpr const char *kFinishShakeName = "finisherFinish"; ///< 締めの強いシェイク設定名

    Player *pOwner_ = nullptr; ///< 所有者のプレイヤー

    std::function<void()> onFireBullet_; ///< 弾を1発発射するコールバック（連射フェーズ用）

    std::unique_ptr<Shake> impactShake_; ///< 各段のヒット時に使うシェイク
    std::unique_ptr<Shake> finishShake_; ///< 締めの一撃で使う強いシェイク

    // ─── 実行状態 ───
    FinisherKind kind_ = FinisherKind::None; ///< 発動中の種別
    int phaseIndex_ = 0;                     ///< 現在のフェーズ番号
    float phaseTimer_ = 0.0f;                ///< 現在のフェーズの経過時間
    float phaseDuration_ = 0.0f;             ///< 現在のフェーズの継続時間
    std::string animationClip_;              ///< 現在のフェーズで再生するクリップ名
    float cooldownTimer_ = 0.0f;             ///< クールダウンの残り時間

    Hagine::Vector3 attackDirection_ = {0.0f, 0.0f, 1.0f}; ///< 技の基準となる水平方向
    Hagine::Vector3 pinnedPosition_{};                     ///< プレイヤーを固定する位置
    Hagine::Vector3 moveStartPosition_{};                  ///< フェーズ内で移動する場合の開始位置
    float pinBaseY_ = 0.0f;                                ///< 浮上演出の基準高さ

    int barrageFiredCount_ = 0;     ///< 連射フェーズで撃った弾数
    float barrageFireTimer_ = 0.0f; ///< 次の弾までの残り時間

    // 締めの一撃を出したか。以降は吹き飛ばし予約の更新でも「ひるみ無効を与える」を保つ
    bool finalHitDelivered_ = false;

    // ─── 調整パラメータ（GameParamで調整可）───
    bool enabled_ = true; ///< 派生技の有効フラグ

    // 専用の演出カメラ（CameraFinisher）を使うか。
    // 既定は false ＝ 通常の追従カメラのまま技だけ出す。
    // true にするとアングルのカット割り・回り込みが入る（見た目は派手だが動きは追いにくい）
    bool useCinematicCamera_ = false;

    // 締めの一撃で画面フラッシュ（白黒＋ブルーム＋ブラーのポストエフェクト）を鳴らすか
    bool useScreenFlash_ = false;

    // 演出カメラを使わないとき、瞬間移動でカメラを現在位置に留める時間（秒）。
    // 0 だと通常カメラが瞬間移動先へワープ追従してガクつく
    float cameraHoldOnTeleport_ = 0.2f;
    int blastRushMinStage_ = 3;   ///< 打ち上げ連射に派生できる最小段数
    int teleportMinStage_ = 5;    ///< 瞬間移動連撃に派生できる最小段数
    int meteorMinStage_ = 7;      ///< 急降下ゼロ距離砲に派生できる最小段数
    float triggerRange_ = 50.0f;    ///< 敵がこの距離より遠いと派生できない（瞬間移動追撃中の始動も許容する広さ）
    float contactDistance_ = 2.6f;  ///< 出際の一撃で敵の手前に詰め寄る距離
    float cooldownDuration_ = 8.0f; ///< 技を終えてから次に派生できるまでの時間（秒）

    float blastRushEnergyCost_ = 25.0f; ///< 打ち上げ連射の消費エネルギー
    float teleportEnergyCost_ = 35.0f;  ///< 瞬間移動連撃の消費エネルギー
    float meteorEnergyCost_ = 45.0f;    ///< 急降下ゼロ距離砲の消費エネルギー

    // ─── 打ち上げ連射（BlastRush）───
    float blastImpactDamage_ = 1.0f;   ///< 出際の一撃のダメージ
    float blastLaunchDamage_ = 1.0f;   ///< 打ち上げのダメージ
    float blastFinishDamage_ = 1.5f;   ///< 追い討ちのダメージ
    float blastLaunchUpSpeed_ = 24.0f; ///< 打ち上げの上方向速度
    float blastHoverFall_ = -2.5f;     ///< 連射中に敵が落ちる速度（負値・小さいほど滞空する）
    float blastPlayerRise_ = 3.5f;     ///< 連射中にプレイヤーが浮く高さ
    int barrageBulletCount_ = 5;       ///< 連射する弾数
    float barrageInterval_ = 0.16f;    ///< 連射の間隔（秒）
    float blastFinishBlowSpeed_ = 30.0f; ///< 追い討ちで吹き飛ばす水平速度

    // ─── 瞬間移動連撃（TeleportSmash）───
    float teleportImpactDamage_ = 1.0f; ///< 出際の一撃のダメージ
    float teleportBlowDamage_ = 0.8f;   ///< 吹き飛ばしのダメージ
    float teleportStrikeDamage_ = 1.3f; ///< 先回り連撃1発あたりのダメージ
    float teleportSlamDamage_ = 2.5f;   ///< 叩き落としのダメージ
    float teleportBlowDistance_ = 26.0f; ///< 敵を吹き飛ばす距離＝先回りする距離
    float teleportArrivalTime_ = 0.28f; ///< 敵が先回り地点へ到達するまでの時間（秒）
    float teleportStrikeUpSpeed_ = 9.0f; ///< 先回り連撃で浮かせる上方向速度
    float teleportCatchDistance_ = 3.2f; ///< 迎え撃つ位置の敵からの距離
    float teleportSlamSpeed_ = 45.0f;    ///< 叩き落としの下方向速度
    float teleportAboveHeight_ = 9.0f;   ///< 叩き落とし前に回り込む高さ

    // ─── 急降下ゼロ距離砲（MeteorDrive）───
    float meteorImpactDamage_ = 1.0f;   ///< 出際の一撃のダメージ
    float meteorSlamDamage_ = 1.5f;     ///< 真下への叩きつけのダメージ
    float meteorBlastDamage_ = 4.0f;    ///< ゼロ距離砲のダメージ
    float meteorSlamSpeed_ = 40.0f;     ///< 真下へ叩きつける速度
    float meteorRiseHeight_ = 16.0f;    ///< 上空へ回り込む高さ
    float meteorDiveHeight_ = 3.0f;     ///< 急降下の到達高さ（敵からの高さ）
    float meteorDiveDistance_ = 2.5f;   ///< 急降下の到達距離（敵からの水平距離）
};

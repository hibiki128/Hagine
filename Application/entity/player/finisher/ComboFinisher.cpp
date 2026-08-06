#define NOMINMAX
#include "ComboFinisher.h"
#include "Application/camera/follow/FollowCamera.h"
#include "Application/entity/enemy/Enemy.h"
#include "Application/entity/player/Player.h"
#include <data/DataHandler.h>
#include <utility/debug/param/GameParamHub.h>
#include <algorithm>
#include <cmath>
#include <iterator>

using namespace Hagine;

namespace {
// ゲームパラメータHub上の出所ラベル
constexpr const char *kParamOwner = "コンボ派生技(Player)";

// 重力加速度が未設定のときに使うフォールバック値
constexpr float kFallbackGravity = 30.0f;

// 打ち上げ時に前方へ流す速度（真上だけだと真下に落ちてきて絵が単調になる）
constexpr float kLaunchForwardSpeed = 5.0f;

// 相手の真上へ回り込むときの水平ずらし量（完全な真上だと向きの計算が退化する）
constexpr float kAboveOffsetDistance = 1.0f;

// 叩き落とした相手を追って降りるときに保つ高さ
constexpr float kSlamFollowHeight = 3.0f;

// 締めの一撃だけに入れる画面の傾き（度）。
// 常時傾けると「カメラが回っている」ようにしか見えないので、最後の一撃のアクセントに限る
constexpr float kFinishRollDegrees = 4.0f;

/// <summary>
/// 0〜1の進行度を滑らかに補間する（両端の変化が緩やかになる）
/// </summary>
/// <param name="t">進行度</param>
/// <returns>float: 補間後の進行度</returns>
float SmoothStep(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
} // namespace

void ComboFinisher::Init(Player *pOwner)
{
    pOwner_ = pOwner;

    impactShake_ = std::make_unique<Shake>();
    impactShake_->Initialize(kImpactShakeName);

    finishShake_ = std::make_unique<Shake>();
    finishShake_->Initialize(kFinishShakeName);
}

/// ===================================================
/// 発動・進行・終了
/// ===================================================

FinisherKind ComboFinisher::SelectKind(int executedStepCount) const
{
    // 段数が深いほど派手な技になる。同じ段数域なら常に同じ技が出るので覚えて狙える
    if (meteorMinStage_ > 0 && executedStepCount >= meteorMinStage_)
    {
        return FinisherKind::MeteorDrive;
    }
    if (teleportMinStage_ > 0 && executedStepCount >= teleportMinStage_)
    {
        return FinisherKind::TeleportSmash;
    }
    if (blastRushMinStage_ > 0 && executedStepCount >= blastRushMinStage_)
    {
        return FinisherKind::BlastRush;
    }
    return FinisherKind::None;
}

bool ComboFinisher::TryStart(int executedStepCount)
{
    if (!enabled_ || IsActive() || cooldownTimer_ > 0.0f)
    {
        return false;
    }

    Enemy *pEnemy = GetTargetEnemy();
    if (!pEnemy)
    {
        return false;
    }

    // 必殺技被弾スタン中の相手には重ねない（浮かせっぱなしで詰ませないための保険）
    if (pEnemy->Status().IsSkillBlow())
    {
        return false;
    }

    // 間合いの外からは派生できない（遠距離から一方的に始動させない）
    Vector3 toEnemy = pEnemy->GetWorldPosition() - pOwner_->GetWorldPosition();
    if (toEnemy.Length() > triggerRange_)
    {
        return false;
    }

    const FinisherKind kind = SelectKind(executedStepCount);
    if (kind == FinisherKind::None)
    {
        return false;
    }

    float energyCost = blastRushEnergyCost_;
    if (kind == FinisherKind::TeleportSmash)
    {
        energyCost = teleportEnergyCost_;
    }
    else if (kind == FinisherKind::MeteorDrive)
    {
        energyCost = meteorEnergyCost_;
    }
    if (!pOwner_->ConsumeEnergy(energyCost))
    {
        return false;
    }

    kind_ = kind;
    phaseIndex_ = -1; // AdvancePhase() で0段目から始める
    barrageFiredCount_ = 0;
    barrageFireTimer_ = 0.0f;
    finalHitDelivered_ = false;
    pinnedPosition_ = pOwner_->GetWorldPosition();
    pinBaseY_ = pinnedPosition_.y;
    attackDirection_ = GetAttackDirection();

    // コンボを繋いだ上での派生なので、出際の一撃はガードを崩す扱いにする。
    // （ガードが残っていると演出だけ流れてほぼ無傷、という噛み合わない絵になる）
    pEnemy->SetGuarding(false);

    // 演出カメラを使う設定のときだけカメラを差し替える。
    // 既定は通常の追従カメラのままで、技の動きそのものを素直に見せる
    if (useCinematicCamera_ && pOwner_->GetCamera())
    {
        pOwner_->GetCamera()->StartFinisherCamera(pOwner_, pEnemy, FinisherCameraStyle::SideProfile);
    }

    AdvancePhase();
    return true;
}

const ComboFinisher::FinisherPhase *ComboFinisher::GetPhaseTable(int &outCount) const
{
    // ─── 打ち上げ連射：出際の一撃→打ち上げ→空中連射→追い討ち ───
    static const FinisherPhase kBlastRush[] = {
        {"Smash", 0.20f},
        {"Kick_2", 0.45f},
        {"FlyIdle", 1.10f},
        {"MakanSkill", 0.60f},
    };

    // ─── 瞬間移動連撃：吹き飛ばし→先回り3連撃→真上へ回り込み→叩き落とし ───
    static const FinisherPhase kTeleportSmash[] = {
        {"Smash", 0.18f},
        {"Kick_1", 0.26f},
        {"Kick_2", 0.34f},
        {"Kick_3", 0.34f},
        {"Punch_4", 0.34f},
        {"FlyMove", 0.28f},
        {"Smash", 0.55f},
    };

    // ─── 急降下ゼロ距離砲：叩きつけ→上空へ→急降下→ゼロ距離砲 ───
    static const FinisherPhase kMeteorDrive[] = {
        {"Smash", 0.20f},
        {"Kick_3", 0.38f},
        {"FlyIdle", 0.42f},
        {"FlyMove", 0.40f},
        {"MakanSkill", 0.75f},
    };

    switch (kind_)
    {
    case FinisherKind::BlastRush:
        outCount = static_cast<int>(std::size(kBlastRush));
        return kBlastRush;
    case FinisherKind::TeleportSmash:
        outCount = static_cast<int>(std::size(kTeleportSmash));
        return kTeleportSmash;
    case FinisherKind::MeteorDrive:
        outCount = static_cast<int>(std::size(kMeteorDrive));
        return kMeteorDrive;
    case FinisherKind::None:
    default:
        outCount = 0;
        return nullptr;
    }
}

void ComboFinisher::AdvancePhase()
{
    int phaseCount = 0;
    const FinisherPhase *pTable = GetPhaseTable(phaseCount);
    if (!pTable)
    {
        Finish();
        return;
    }

    ++phaseIndex_;
    if (phaseIndex_ >= phaseCount)
    {
        Finish();
        return;
    }

    phaseTimer_ = 0.0f;
    phaseDuration_ = pTable[phaseIndex_].duration;
    animationClip_ = pTable[phaseIndex_].animationClip;
    moveStartPosition_ = pOwner_->GetWorldPosition();

    OnPhaseEnter();
}

void ComboFinisher::Update(float deltaTime)
{
    // 揺れは技が終わったあとも余韻として残るため、発動状態に関わらず毎フレーム進める
    impactShake_->Update();
    finishShake_->Update();

    if (!IsActive())
    {
        // 非発動中はクールダウンだけを進める
        if (cooldownTimer_ > 0.0f)
        {
            cooldownTimer_ = (std::max)(cooldownTimer_ - deltaTime, 0.0f);
        }
        return;
    }

    // 相手が倒れた・消えた場合は演出を打ち切る
    if (!GetTargetEnemy())
    {
        Cancel();
        return;
    }

    // 演出中の被弾はすべて吹き飛ばし扱いにする。
    // フェーズ開始処理より前に張り直すことで、締めの一撃が上書きされないようにする
    RefreshEnemyBlowReaction();

    OnPhaseUpdate(deltaTime);

    phaseTimer_ += deltaTime;
    if (phaseTimer_ >= phaseDuration_)
    {
        AdvancePhase();
    }
}

void ComboFinisher::Finish()
{
    kind_ = FinisherKind::None;
    animationClip_.clear();
    phaseIndex_ = 0;
    phaseTimer_ = 0.0f;
    phaseDuration_ = 0.0f;
    cooldownTimer_ = cooldownDuration_;

    pOwner_->SetAlpha(1.0f); // 瞬間移動の透過演出を解除する

    if (pOwner_->GetCamera())
    {
        pOwner_->GetCamera()->SetFinisherCameraRoll(0.0f);
        pOwner_->GetCamera()->EndFinisherCamera();
    }

    // 空中に取り残されたら落下ステートへ移し、棒立ちで浮かないようにする。
    // 被弾で中断した場合はリアクション側がステートを管理するので触らない
    if (!pOwner_->GetIsGrounded() && !pOwner_->Status().IsReacting())
    {
        pOwner_->ChangeState("Air");
    }
}

void ComboFinisher::Cancel()
{
    if (!IsActive())
    {
        return;
    }
    Finish();
}

/// ===================================================
/// フェーズ処理のディスパッチ
/// ===================================================

void ComboFinisher::OnPhaseEnter()
{
    switch (kind_)
    {
    case FinisherKind::BlastRush:
        EnterBlastRush();
        break;
    case FinisherKind::TeleportSmash:
        EnterTeleportSmash();
        break;
    case FinisherKind::MeteorDrive:
        EnterMeteorDrive();
        break;
    case FinisherKind::None:
    default:
        break;
    }
}

void ComboFinisher::OnPhaseUpdate(float deltaTime)
{
    switch (kind_)
    {
    case FinisherKind::BlastRush:
        UpdateBlastRush(deltaTime);
        break;
    case FinisherKind::TeleportSmash:
        UpdateTeleportSmash(deltaTime);
        break;
    case FinisherKind::MeteorDrive:
        UpdateMeteorDrive(deltaTime);
        break;
    case FinisherKind::None:
    default:
        break;
    }
}

/// ===================================================
/// 打ち上げ連射（BlastRush）
/// ===================================================

void ComboFinisher::EnterBlastRush()
{
    switch (phaseIndex_)
    {
    case 0:
        // 出際の一撃：相手の手前へ詰め寄り、動きを止めて溜めを作る。
        // 開始時のカメラは Start() が合わせ済みなのでカットしない
        CloseInOnEnemy();
        HitEnemy(blastImpactDamage_, {0.0f, 0.0f, 0.0f});
        impactShake_->StartShake();
        SetCameraStyle(FinisherCameraStyle::SideProfile, false, 0.0f);
        break;

    case 1:
        // 打ち上げ：斜め上へ大きく浮かせ、プレイヤーもふわりと追って浮上する。
        // カメラは同じ側のまま低い位置へ下がって煽るので、切らずに繋げる
        HitEnemy(blastLaunchDamage_,
                 {attackDirection_.x * kLaunchForwardSpeed,
                  blastLaunchUpSpeed_,
                  attackDirection_.z * kLaunchForwardSpeed});
        impactShake_->StartShake();
        pinBaseY_ = pOwner_->GetWorldPosition().y;
        pOwner_->GetIsGrounded() = false;
        SetCameraStyle(FinisherCameraStyle::LowAngleUp, false, 0.0f);
        break;

    case 2:
        // 連射：肩越しへ切り替える。位置が大きく変わるので、ここだけカットで切り替える
        barrageFiredCount_ = 0;
        barrageFireTimer_ = 0.0f;
        SetCameraStyle(FinisherCameraStyle::OverShoulder, true, 0.0f);
        break;

    case 3:
        // 追い討ち：締めの一撃で大きく吹き飛ばし、画面を明滅させる。
        // 横位置へ戻して、飛んでいく相手を見送れるようにする
        attackDirection_ = GetAttackDirection();
        HitEnemy(blastFinishDamage_,
                 {attackDirection_.x * blastFinishBlowSpeed_,
                  blastFinishBlowSpeed_ * 0.35f,
                  attackDirection_.z * blastFinishBlowSpeed_},
                 true);
        finishShake_->StartShake();
        TriggerScreenFlash();
        SetCameraStyle(FinisherCameraStyle::SideProfile, true, kFinishRollDegrees);
        break;

    default:
        break;
    }
}

void ComboFinisher::UpdateBlastRush(float deltaTime)
{
    switch (phaseIndex_)
    {
    case 0:
    {
        // 溜めの間は相手も止めて、ヒットストップのような間を作る
        Enemy *pEnemy = GetTargetEnemy();
        if (pEnemy)
        {
            pEnemy->SetVelocity({0.0f, 0.0f, 0.0f});
        }
        PinPlayer();
        break;
    }

    case 1:
        // 打ち上げに合わせてプレイヤーも浮上する
        pinnedPosition_.y = pinBaseY_ + blastPlayerRise_ * SmoothStep(phaseTimer_ / phaseDuration_);
        PinPlayer();
        break;

    case 2:
    {
        // 相手を滞空させたまま、間隔をあけて弾を撃ち込む
        HoldEnemyAirborne(blastHoverFall_);
        PinPlayer();

        barrageFireTimer_ -= deltaTime;
        if (barrageFiredCount_ < barrageBulletCount_ && barrageFireTimer_ <= 0.0f)
        {
            if (onFireBullet_)
            {
                onFireBullet_();
            }
            ++barrageFiredCount_;
            barrageFireTimer_ = barrageInterval_;
        }
        break;
    }

    case 3:
        PinPlayer();
        break;

    default:
        break;
    }
}

/// ===================================================
/// 瞬間移動連撃（TeleportSmash）
/// ===================================================

void ComboFinisher::EnterTeleportSmash()
{
    Enemy *pEnemy = GetTargetEnemy();
    if (!pEnemy)
    {
        return;
    }

    // 吹き飛ばし・先回りの速度。到達時間で割ることで「距離を伸ばすほど速く飛ぶ」ようにする
    const float arrivalTime = (std::max)(teleportArrivalTime_, kEpsilon);
    const float blowSpeed = teleportBlowDistance_ / arrivalTime;

    switch (phaseIndex_)
    {
    case 0:
        // 出際の一撃：相手の手前へ詰め寄り、溜めを作ってから吹き飛ばしへ繋ぐ。
        // 開始時のカメラは Start() が合わせ済みなのでカットしない
        CloseInOnEnemy();
        HitEnemy(teleportImpactDamage_, {0.0f, 0.0f, 0.0f});
        impactShake_->StartShake();
        SetCameraStyle(FinisherCameraStyle::SideProfile, false, 0.0f);
        break;

    case 1:
        // 横へ大きく吹き飛ばし、プレイヤーは姿を消す。
        // カメラは同じ横位置のまま。離れるほど自動で引くので飛んでいく相手を追える
        HitEnemy(teleportBlowDamage_,
                 {attackDirection_.x * blowSpeed, 2.0f, attackDirection_.z * blowSpeed});
        impactShake_->StartShake();
        pOwner_->SetAlpha(0.15f);
        SetCameraStyle(FinisherCameraStyle::SideProfile, false, 0.0f);
        break;

    case 2:
    case 3:
    case 4:
    {
        // 飛んでいる相手の進路へ先回りし、迎え撃って逆方向へ蹴り返す。
        // 進行方向は相手の速度から取り、止まっていれば直前の吹き飛ばし方向を使う
        Vector3 travel = pEnemy->GetVelocity();
        travel.y = 0.0f;
        travel = (travel.Length() > kEpsilon) ? travel.Normalize() : attackDirection_;

        // 相手の進路の先で待ち構える
        TeleportPlayer(GetPositionAroundEnemy(travel, teleportCatchDistance_, 0.0f));

        // 来た方向と逆へ蹴り返す。以降はこの方向が新しい基準になる
        const Vector3 counter = {-travel.x, 0.0f, -travel.z};
        HitEnemy(teleportStrikeDamage_,
                 {counter.x * blowSpeed, teleportStrikeUpSpeed_, counter.z * blowSpeed});
        attackDirection_ = counter;

        impactShake_->StartShake();

        // 1発目だけカットで一気に寄せ、以降は同じ横位置のまま繋ぐ。
        // 蹴り返すたびにカメラまで反対側へ回すと、前後関係が入れ替わって何も読めなくなる
        const bool isFirstStrike = (phaseIndex_ == 2);
        SetCameraStyle(FinisherCameraStyle::SideProfile, isFirstStrike, 0.0f);
        break;
    }

    case 5:
        // 真上へ回り込む。相手はその場で止めて叩き落としの的にする。
        // 縦に並ぶので見下ろしではなく横位置のまま見せる（見下ろすと2人が重なる）
        pEnemy->SetVelocity({0.0f, 0.0f, 0.0f});
        TeleportPlayer(GetPositionAroundEnemy(attackDirection_, kAboveOffsetDistance, teleportAboveHeight_));
        pOwner_->SetAlpha(1.0f);
        SetCameraStyle(FinisherCameraStyle::SideProfile, false, 0.0f);
        break;

    case 6:
        // 叩き落とし：真下へ叩きつけ、締めとしてひるみ無効時間を与えて仕切り直す
        HitEnemy(teleportSlamDamage_, {0.0f, -teleportSlamSpeed_, 0.0f}, true);
        pOwner_->RequestGroundCrack(pEnemy); // 相手が地面に到達したら地割れを出す
        finishShake_->StartShake();
        TriggerScreenFlash();
        SetCameraStyle(FinisherCameraStyle::SideProfile, false, kFinishRollDegrees);
        break;

    default:
        break;
    }
}

void ComboFinisher::UpdateTeleportSmash(float deltaTime)
{
    (void)deltaTime;

    Enemy *pEnemy = GetTargetEnemy();

    switch (phaseIndex_)
    {
    case 0:
        if (pEnemy)
        {
            pEnemy->SetVelocity({0.0f, 0.0f, 0.0f});
        }
        PinPlayer();
        break;

    case 1:
        PinPlayer();
        break;

    case 2:
    case 3:
    case 4:
        // 瞬間移動直後は薄く、迎え撃つ間に実体化していくように見せる
        pOwner_->SetAlpha(0.15f + 0.85f * SmoothStep(phaseTimer_ / phaseDuration_));
        PinPlayer();
        break;

    case 5:
        // 相手を真下に留めたまま、上から狙いを定める
        if (pEnemy)
        {
            pEnemy->SetVelocity({0.0f, 0.0f, 0.0f});
        }
        PinPlayer();
        break;

    case 6:
        // 叩き落とした相手を追って自分も降下する
        if (pEnemy)
        {
            const float t = SmoothStep(phaseTimer_ / phaseDuration_);
            const Vector3 goal = pEnemy->GetWorldPosition() + Vector3(0.0f, kSlamFollowHeight, 0.0f);
            pinnedPosition_ = moveStartPosition_ + (goal - moveStartPosition_) * t;
        }
        PinPlayer();
        break;

    default:
        break;
    }
}

/// ===================================================
/// 急降下ゼロ距離砲（MeteorDrive）
/// ===================================================

void ComboFinisher::EnterMeteorDrive()
{
    Enemy *pEnemy = GetTargetEnemy();
    if (!pEnemy)
    {
        return;
    }

    switch (phaseIndex_)
    {
    case 0:
        // 出際の一撃：相手の手前へ詰め寄る。
        // 開始時のカメラは Start() が合わせ済みなのでカットしない
        CloseInOnEnemy();
        HitEnemy(meteorImpactDamage_, {0.0f, 0.0f, 0.0f});
        impactShake_->StartShake();
        SetCameraStyle(FinisherCameraStyle::SideProfile, false, 0.0f);
        break;

    case 1:
        // 真下へ叩きつける。縦の動きは横位置から見るのが一番はっきり見える
        HitEnemy(meteorSlamDamage_, {0.0f, -meteorSlamSpeed_, 0.0f});
        pOwner_->RequestGroundCrack(pEnemy); // 相手が地面に到達したら地割れを出す
        impactShake_->StartShake();
        SetCameraStyle(FinisherCameraStyle::SideProfile, false, 0.0f);
        break;

    case 2:
        // 上空へ瞬間移動。低い位置から見上げて高さを強調する
        // （2人が縦に離れるぶん、カメラは自動で引く）
        TeleportPlayer(GetPositionAroundEnemy(attackDirection_, meteorDiveDistance_, meteorRiseHeight_));
        SetCameraStyle(FinisherCameraStyle::LowAngleUp, true, 0.0f);
        break;

    case 3:
        // 急降下。ゆっくり回り込みながら落下を追いかける
        SetCameraStyle(FinisherCameraStyle::Orbit, false, 0.0f);
        break;

    case 4:
        // ゼロ距離砲：必殺技級の吹き飛ばしで大きく引き離し、仕切り直させる。
        // 急降下で相手の反対側へ回り込んでいるので、吹き飛ばす向きは取り直す
        attackDirection_ = GetAttackDirection();
        pEnemy->Status().RequestSkillBlowReaction(attackDirection_);
        pEnemy->SetDamage(meteorBlastDamage_, false, true);
        finishShake_->StartShake();
        TriggerScreenFlash();
        SetCameraStyle(FinisherCameraStyle::SideProfile, true, -kFinishRollDegrees);
        break;

    default:
        break;
    }
}

void ComboFinisher::UpdateMeteorDrive(float deltaTime)
{
    (void)deltaTime;

    Enemy *pEnemy = GetTargetEnemy();

    switch (phaseIndex_)
    {
    case 0:
        if (pEnemy)
        {
            pEnemy->SetVelocity({0.0f, 0.0f, 0.0f});
        }
        PinPlayer();
        break;

    case 1:
    case 2:
        PinPlayer();
        break;

    case 3:
        // 上空から相手のすぐ手前まで一気に降りる
        if (pEnemy)
        {
            const float t = SmoothStep(phaseTimer_ / phaseDuration_);
            const Vector3 goal =
                GetPositionAroundEnemy(attackDirection_, meteorDiveDistance_, meteorDiveHeight_);
            pinnedPosition_ = moveStartPosition_ + (goal - moveStartPosition_) * t;
        }
        PinPlayer();
        break;

    case 4:
        PinPlayer();
        break;

    default:
        break;
    }
}

/// ===================================================
/// 共通の小道具
/// ===================================================

Enemy *ComboFinisher::GetTargetEnemy() const
{
    Enemy *pEnemy = pOwner_ ? pOwner_->GetEnemy() : nullptr;
    if (!pEnemy || !pEnemy->GetAlive())
    {
        return nullptr;
    }
    return pEnemy;
}

Vector3 ComboFinisher::GetAttackDirection()
{
    Enemy *pEnemy = GetTargetEnemy();
    if (!pEnemy)
    {
        return attackDirection_;
    }

    Vector3 direction = pEnemy->GetWorldPosition() - pOwner_->GetWorldPosition();
    direction.y = 0.0f;
    if (direction.Length() < kEpsilon)
    {
        return attackDirection_;
    }
    return direction.Normalize();
}

Vector3 ComboFinisher::GetPositionAroundEnemy(const Vector3 &direction, float distance,
                                              float heightOffset) const
{
    Enemy *pEnemy = GetTargetEnemy();
    if (!pEnemy)
    {
        return pOwner_->GetWorldPosition();
    }

    Vector3 horizontal = {direction.x, 0.0f, direction.z};
    horizontal = (horizontal.Length() > kEpsilon) ? horizontal.Normalize() : attackDirection_;

    const Vector3 enemyPos = pEnemy->GetWorldPosition();
    Vector3 position = enemyPos + horizontal * distance;
    position.y = enemyPos.y + heightOffset;
    return position;
}

void ComboFinisher::CloseInOnEnemy()
{
    attackDirection_ = GetAttackDirection();

    // 敵から見て「自分がいる側」の手前に立つ。もともと近ければほとんど動かないので、
    // ここではカメラを止めない（技の入りでいきなり画面が固まると間延びして見える）
    const Vector3 backward = {-attackDirection_.x, 0.0f, -attackDirection_.z};
    TeleportPlayer(GetPositionAroundEnemy(backward, contactDistance_, 0.0f), false);
}

void ComboFinisher::TeleportPlayer(const Vector3 &position, bool holdCamera)
{
    pOwner_->GetLocalPosition() = position;
    pinnedPosition_ = position;
    pOwner_->GetVelocity() = {0.0f, 0.0f, 0.0f};
    pOwner_->GetIsGrounded() = false;

    Enemy *pEnemy = GetTargetEnemy();
    if (pEnemy)
    {
        pOwner_->Movement().FaceTargetInstant(pEnemy->GetWorldPosition());
    }

    // 演出カメラを使わない場合、通常カメラをそのままにすると瞬間移動先へワープ追従して
    // 画面がガクつく。既存の瞬間移動コンボと同じく、一瞬留めてからスナップさせる
    if (!useCinematicCamera_ && holdCamera && pOwner_->GetCamera())
    {
        pOwner_->GetCamera()->HoldThenSnap(cameraHoldOnTeleport_);
    }
}

void ComboFinisher::PinPlayer()
{
    pOwner_->GetLocalPosition() = pinnedPosition_;
    pOwner_->GetVelocity() = {0.0f, 0.0f, 0.0f};

    Enemy *pEnemy = GetTargetEnemy();
    if (pEnemy)
    {
        pOwner_->Movement().FaceTargetInstant(pEnemy->GetWorldPosition());
    }
}

void ComboFinisher::HitEnemy(float damage, const Vector3 &velocity, bool grantsFlinchImmunity)
{
    Enemy *pEnemy = GetTargetEnemy();
    if (!pEnemy)
    {
        return;
    }

    if (grantsFlinchImmunity)
    {
        finalHitDelivered_ = true;
    }

    // 吹き飛ばしリアクションとして扱わせる（BTを止め、BlowBack→着地でBlowAfterへ）。
    // 予約はダメージと同じフレームに処理されるため、順序はどちらが先でもよい
    pEnemy->RequestBlowReaction(grantsFlinchImmunity);
    pEnemy->SetDamage(damage);

    // 速度は加算ではなく上書きする。演出の軌道を毎回同じにするため
    pEnemy->SetVelocity(velocity);
    pEnemy->Movement().CancelVelocityEase();
    pEnemy->SetIsFlying(false);

    if (velocity.Length() > kEpsilon)
    {
        // 空中へ飛ばすので接地を外し、重力で落ちる状態にする
        pEnemy->SetIsGrounded(false);
        float gravity = std::abs(pEnemy->GetFallSpeed());
        if (gravity < kEpsilon)
        {
            gravity = kFallbackGravity;
        }
        pEnemy->GetAcceleration().y = -gravity;
    }
}

void ComboFinisher::RefreshEnemyBlowReaction()
{
    Enemy *pEnemy = GetTargetEnemy();
    if (!pEnemy)
    {
        return;
    }

    // 必殺技被弾スタン（ゼロ距離砲の締め）はより強いリアクションなので上書きしない
    if (pEnemy->Status().IsSkillBlow())
    {
        return;
    }

    // 締めの一撃を出したあとは、ひるみ無効の付与指定も維持する
    pEnemy->RequestBlowReaction(finalHitDelivered_);
}

void ComboFinisher::HoldEnemyAirborne(float hoverFallSpeed)
{
    Enemy *pEnemy = GetTargetEnemy();
    if (!pEnemy)
    {
        return;
    }

    // 落下速度に下限を設けて、撃ち込んでいる間はゆっくり落ちるだけにする
    Vector3 velocity = pEnemy->GetVelocity();
    if (velocity.y < hoverFallSpeed)
    {
        velocity.y = hoverFallSpeed;
        pEnemy->SetVelocity(velocity);
    }
}

void ComboFinisher::SetCameraStyle(FinisherCameraStyle style, bool isCut, float rollDegrees)
{
    if (!useCinematicCamera_)
    {
        return; // 通常の追従カメラのまま見せる
    }

    FollowCamera *pCamera = pOwner_->GetCamera();
    if (!pCamera)
    {
        return;
    }
    pCamera->SetFinisherCameraStyle(style, isCut);
    pCamera->SetFinisherCameraRoll(rollDegrees);
}

void ComboFinisher::TriggerScreenFlash()
{
    if (!useScreenFlash_)
    {
        return;
    }
    pOwner_->TriggerScreenFlash();
}

/// ===================================================
/// セーブ・ロード・デバッグ表示
/// ===================================================

void ComboFinisher::Save(DataHandler *pData)
{
    pData->Save("fnEnabled", enabled_);
    pData->Save("fnUseCinematicCamera", useCinematicCamera_);
    pData->Save("fnUseScreenFlash", useScreenFlash_);
    pData->Save("fnCameraHoldOnTeleport", cameraHoldOnTeleport_);
    pData->Save("fnBlastStage", blastRushMinStage_);
    pData->Save("fnTeleportStage", teleportMinStage_);
    pData->Save("fnMeteorStage", meteorMinStage_);
    pData->Save("fnTriggerRange", triggerRange_);
    pData->Save("fnContactDistance", contactDistance_);
    pData->Save("fnCooldown", cooldownDuration_);

    pData->Save("fnBlastCost", blastRushEnergyCost_);
    pData->Save("fnTeleportCost", teleportEnergyCost_);
    pData->Save("fnMeteorCost", meteorEnergyCost_);

    pData->Save("fnBlastImpactDmg", blastImpactDamage_);
    pData->Save("fnBlastLaunchDmg", blastLaunchDamage_);
    pData->Save("fnBlastFinishDmg", blastFinishDamage_);
    pData->Save("fnBlastLaunchUp", blastLaunchUpSpeed_);
    pData->Save("fnBlastHoverFall", blastHoverFall_);
    pData->Save("fnBlastPlayerRise", blastPlayerRise_);
    pData->Save("fnBarrageCount", barrageBulletCount_);
    pData->Save("fnBarrageInterval", barrageInterval_);
    pData->Save("fnBlastFinishBlow", blastFinishBlowSpeed_);

    pData->Save("fnTpImpactDmg", teleportImpactDamage_);
    pData->Save("fnTpBlowDmg", teleportBlowDamage_);
    pData->Save("fnTpStrikeDmg", teleportStrikeDamage_);
    pData->Save("fnTpSlamDmg", teleportSlamDamage_);
    pData->Save("fnTpBlowDistance", teleportBlowDistance_);
    pData->Save("fnTpArrivalTime", teleportArrivalTime_);
    pData->Save("fnTpStrikeUp", teleportStrikeUpSpeed_);
    pData->Save("fnTpCatchDistance", teleportCatchDistance_);
    pData->Save("fnTpSlamSpeed", teleportSlamSpeed_);
    pData->Save("fnTpAboveHeight", teleportAboveHeight_);

    pData->Save("fnMtImpactDmg", meteorImpactDamage_);
    pData->Save("fnMtSlamDmg", meteorSlamDamage_);
    pData->Save("fnMtBlastDmg", meteorBlastDamage_);
    pData->Save("fnMtSlamSpeed", meteorSlamSpeed_);
    pData->Save("fnMtRiseHeight", meteorRiseHeight_);
    pData->Save("fnMtDiveHeight", meteorDiveHeight_);
    pData->Save("fnMtDiveDistance", meteorDiveDistance_);
}

void ComboFinisher::Load(DataHandler *pData)
{
    enabled_ = pData->Load<bool>("fnEnabled", enabled_);
    useCinematicCamera_ = pData->Load<bool>("fnUseCinematicCamera", useCinematicCamera_);
    useScreenFlash_ = pData->Load<bool>("fnUseScreenFlash", useScreenFlash_);
    cameraHoldOnTeleport_ = pData->Load<float>("fnCameraHoldOnTeleport", cameraHoldOnTeleport_);
    blastRushMinStage_ = pData->Load<int>("fnBlastStage", blastRushMinStage_);
    teleportMinStage_ = pData->Load<int>("fnTeleportStage", teleportMinStage_);
    meteorMinStage_ = pData->Load<int>("fnMeteorStage", meteorMinStage_);
    triggerRange_ = pData->Load<float>("fnTriggerRange", triggerRange_);
    contactDistance_ = pData->Load<float>("fnContactDistance", contactDistance_);
    cooldownDuration_ = pData->Load<float>("fnCooldown", cooldownDuration_);

    blastRushEnergyCost_ = pData->Load<float>("fnBlastCost", blastRushEnergyCost_);
    teleportEnergyCost_ = pData->Load<float>("fnTeleportCost", teleportEnergyCost_);
    meteorEnergyCost_ = pData->Load<float>("fnMeteorCost", meteorEnergyCost_);

    blastImpactDamage_ = pData->Load<float>("fnBlastImpactDmg", blastImpactDamage_);
    blastLaunchDamage_ = pData->Load<float>("fnBlastLaunchDmg", blastLaunchDamage_);
    blastFinishDamage_ = pData->Load<float>("fnBlastFinishDmg", blastFinishDamage_);
    blastLaunchUpSpeed_ = pData->Load<float>("fnBlastLaunchUp", blastLaunchUpSpeed_);
    blastHoverFall_ = pData->Load<float>("fnBlastHoverFall", blastHoverFall_);
    blastPlayerRise_ = pData->Load<float>("fnBlastPlayerRise", blastPlayerRise_);
    barrageBulletCount_ = pData->Load<int>("fnBarrageCount", barrageBulletCount_);
    barrageInterval_ = pData->Load<float>("fnBarrageInterval", barrageInterval_);
    blastFinishBlowSpeed_ = pData->Load<float>("fnBlastFinishBlow", blastFinishBlowSpeed_);

    teleportImpactDamage_ = pData->Load<float>("fnTpImpactDmg", teleportImpactDamage_);
    teleportBlowDamage_ = pData->Load<float>("fnTpBlowDmg", teleportBlowDamage_);
    teleportStrikeDamage_ = pData->Load<float>("fnTpStrikeDmg", teleportStrikeDamage_);
    teleportSlamDamage_ = pData->Load<float>("fnTpSlamDmg", teleportSlamDamage_);
    teleportBlowDistance_ = pData->Load<float>("fnTpBlowDistance", teleportBlowDistance_);
    teleportArrivalTime_ = pData->Load<float>("fnTpArrivalTime", teleportArrivalTime_);
    teleportStrikeUpSpeed_ = pData->Load<float>("fnTpStrikeUp", teleportStrikeUpSpeed_);
    teleportCatchDistance_ = pData->Load<float>("fnTpCatchDistance", teleportCatchDistance_);
    teleportSlamSpeed_ = pData->Load<float>("fnTpSlamSpeed", teleportSlamSpeed_);
    teleportAboveHeight_ = pData->Load<float>("fnTpAboveHeight", teleportAboveHeight_);

    meteorImpactDamage_ = pData->Load<float>("fnMtImpactDmg", meteorImpactDamage_);
    meteorSlamDamage_ = pData->Load<float>("fnMtSlamDmg", meteorSlamDamage_);
    meteorBlastDamage_ = pData->Load<float>("fnMtBlastDmg", meteorBlastDamage_);
    meteorSlamSpeed_ = pData->Load<float>("fnMtSlamSpeed", meteorSlamSpeed_);
    meteorRiseHeight_ = pData->Load<float>("fnMtRiseHeight", meteorRiseHeight_);
    meteorDiveHeight_ = pData->Load<float>("fnMtDiveHeight", meteorDiveHeight_);
    meteorDiveDistance_ = pData->Load<float>("fnMtDiveDistance", meteorDiveDistance_);
}

void ComboFinisher::DrawImGui()
{
#ifdef USE_IMGUI
    if (!ImGui::CollapsingHeader("コンボ派生技"))
    {
        return;
    }

    ImGui::TextDisabled("近接コンボ中に射撃ボタンを押すと段数に応じた派生技が出ます");

    const char *kindName = "なし";
    switch (kind_)
    {
    case FinisherKind::BlastRush:
        kindName = "打ち上げ連射";
        break;
    case FinisherKind::TeleportSmash:
        kindName = "瞬間移動連撃";
        break;
    case FinisherKind::MeteorDrive:
        kindName = "急降下ゼロ距離砲";
        break;
    case FinisherKind::None:
    default:
        break;
    }
    ImGui::Text("発動中: %s (フェーズ %d)", kindName, phaseIndex_);
    ImGui::Text("クールダウン残り: %.2f 秒", cooldownTimer_);
    ImGui::Separator();

    ImGui::Checkbox("有効##finisher", &enabled_);
    ImGui::Checkbox("演出カメラを使う##finisher", &useCinematicCamera_);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("OFFなら通常の追従カメラのまま技だけ出します");
    }
    ImGui::Checkbox("画面フラッシュを使う##finisher", &useScreenFlash_);
    ImGui::DragFloat("瞬間移動時のカメラ待機(秒)", &cameraHoldOnTeleport_, 0.01f, 0.0f, 1.0f);
    ImGui::DragInt("打ち上げ連射の最小段数", &blastRushMinStage_, 1, 1, 12);
    ImGui::DragInt("瞬間移動連撃の最小段数", &teleportMinStage_, 1, 1, 12);
    ImGui::DragInt("ゼロ距離砲の最小段数", &meteorMinStage_, 1, 1, 12);
    ImGui::DragFloat("発動できる間合い", &triggerRange_, 0.5f, 1.0f, 100.0f);
    ImGui::DragFloat("出際に詰め寄る距離", &contactDistance_, 0.1f, 0.5f, 15.0f);
    ImGui::DragFloat("クールダウン(秒)", &cooldownDuration_, 0.1f, 0.0f, 30.0f);
#endif // USE_IMGUI
}

void ComboFinisher::RegisterParams()
{
    auto *pHub = GameParamHub::GetInstance();

    pHub->Register(kParamOwner, "有効", &enabled_);
    pHub->Register(kParamOwner, "演出カメラを使う", &useCinematicCamera_);
    pHub->Register(kParamOwner, "画面フラッシュを使う", &useScreenFlash_);
    pHub->Register(kParamOwner, "瞬間移動時のカメラ待機(秒)", &cameraHoldOnTeleport_, {0.01f, 0.0f, 1.0f});
    pHub->Register(kParamOwner, "打ち上げ連射の最小段数", &blastRushMinStage_, {1.0f, 1.0f, 12.0f});
    pHub->Register(kParamOwner, "瞬間移動連撃の最小段数", &teleportMinStage_, {1.0f, 1.0f, 12.0f});
    pHub->Register(kParamOwner, "ゼロ距離砲の最小段数", &meteorMinStage_, {1.0f, 1.0f, 12.0f});
    pHub->Register(kParamOwner, "発動できる間合い", &triggerRange_, {0.5f, 1.0f, 100.0f});
    pHub->Register(kParamOwner, "出際に詰め寄る距離", &contactDistance_, {0.1f, 0.5f, 15.0f});
    pHub->Register(kParamOwner, "クールダウン(秒)", &cooldownDuration_, {0.1f, 0.0f, 30.0f});

    pHub->Register(kParamOwner, "消費:打ち上げ連射", &blastRushEnergyCost_, {0.5f, 0.0f, 100.0f});
    pHub->Register(kParamOwner, "消費:瞬間移動連撃", &teleportEnergyCost_, {0.5f, 0.0f, 100.0f});
    pHub->Register(kParamOwner, "消費:ゼロ距離砲", &meteorEnergyCost_, {0.5f, 0.0f, 100.0f});

    pHub->Register(kParamOwner, "連射:出際ダメージ", &blastImpactDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "連射:打ち上げダメージ", &blastLaunchDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "連射:追い討ちダメージ", &blastFinishDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "連射:打ち上げ速度", &blastLaunchUpSpeed_, {0.5f, 0.0f, 80.0f});
    pHub->Register(kParamOwner, "連射:滞空落下速度", &blastHoverFall_, {0.1f, -30.0f, 0.0f});
    pHub->Register(kParamOwner, "連射:自機の浮上量", &blastPlayerRise_, {0.1f, 0.0f, 30.0f});
    pHub->Register(kParamOwner, "連射:弾数", &barrageBulletCount_, {1.0f, 1.0f, 20.0f});
    pHub->Register(kParamOwner, "連射:間隔(秒)", &barrageInterval_, {0.01f, 0.02f, 1.0f});
    pHub->Register(kParamOwner, "連射:追い討ち吹き飛ばし", &blastFinishBlowSpeed_, {0.5f, 0.0f, 120.0f});

    pHub->Register(kParamOwner, "瞬移:出際ダメージ", &teleportImpactDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "瞬移:吹き飛ばしダメージ", &teleportBlowDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "瞬移:連撃ダメージ", &teleportStrikeDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "瞬移:叩き落としダメージ", &teleportSlamDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "瞬移:吹き飛ばし距離", &teleportBlowDistance_, {0.5f, 1.0f, 80.0f});
    pHub->Register(kParamOwner, "瞬移:到達時間(秒)", &teleportArrivalTime_, {0.01f, 0.05f, 1.5f});
    pHub->Register(kParamOwner, "瞬移:連撃の浮かせ速度", &teleportStrikeUpSpeed_, {0.5f, 0.0f, 50.0f});
    pHub->Register(kParamOwner, "瞬移:迎え撃つ距離", &teleportCatchDistance_, {0.1f, 0.5f, 15.0f});
    pHub->Register(kParamOwner, "瞬移:叩き落とし速度", &teleportSlamSpeed_, {0.5f, 0.0f, 150.0f});
    pHub->Register(kParamOwner, "瞬移:回り込む高さ", &teleportAboveHeight_, {0.1f, 1.0f, 40.0f});

    pHub->Register(kParamOwner, "急降下:出際ダメージ", &meteorImpactDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "急降下:叩きつけダメージ", &meteorSlamDamage_, {0.05f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "急降下:ゼロ距離砲ダメージ", &meteorBlastDamage_, {0.05f, 0.0f, 30.0f});
    pHub->Register(kParamOwner, "急降下:叩きつけ速度", &meteorSlamSpeed_, {0.5f, 0.0f, 150.0f});
    pHub->Register(kParamOwner, "急降下:上昇高さ", &meteorRiseHeight_, {0.5f, 1.0f, 60.0f});
    pHub->Register(kParamOwner, "急降下:到達高さ", &meteorDiveHeight_, {0.1f, 0.0f, 20.0f});
    pHub->Register(kParamOwner, "急降下:到達距離", &meteorDiveDistance_, {0.1f, 0.0f, 20.0f});
}

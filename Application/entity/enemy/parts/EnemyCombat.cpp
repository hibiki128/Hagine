#define NOMINMAX
#include "EnemyCombat.h"
#include <Application/entity/enemy/Enemy.h>
#include <Application/entity/player/Player.h>
#include <particle/gpu/ParticleCSSpawner.h>
#include <Application/camera/follow/FollowCamera.h>
#include <debug/log/Logger.h>
#include <Frame.h>
#include <utility/debug/param/GameParamHub.h>
#include <cmath>
#include <numbers>

using namespace Hagine;

EnemyCombat::EnemyCombat() {}

EnemyCombat::~EnemyCombat()
{
    // ポインタ失効前にゲームパラメータHubから登録を解除する
    GameParamHub::GetInstance()->Unregister("必殺演出(Enemy)");
}

void EnemyCombat::Init(Enemy *pOwner)
{
    pOwner_ = pOwner;

    chargeShake_ = std::make_unique<Shake>();

    // 大技演出の GPU パーティクル。Spawn した実体の更新・描画はエンジンが自動で回すので、
    // ここでは各所で発生（SetAuto）と姿勢だけ設定する。
    // チャージ攻撃演出（enemyChargeAura: 既存の気弾チャージオーラ）
    pChargeAura_ = ParticleCSSpawner::GetInstance()->Spawn("enemyChargeAura");
    // ビームメイン演出（プレイヤーの MakanAttackSkill と同じテンプレートを流用）
    pBeamMainEffect_ = ParticleCSSpawner::GetInstance()->Spawn("makan_main");
    // ビームらせん演出
    pBeamAroundEffect_ = ParticleCSSpawner::GetInstance()->Spawn("makan_around");

    // ビーム判定コライダー（初期は無効化）
    pBeamCollider_ = pOwner_->AddOBBCollider("enemy_BeamCollider");
    pBeamCollider_->SetTag("EnemyBeam");
    pBeamCollider_->AddCollisionMask("Player");
    pBeamCollider_->SetEnabled(false);

    // ビームは手（右手ジョイント）から発射するため、コライダーの中心も
    // 敵本体ではなく発射起点（beamOrigin_）に追従させる
    pBeamCollider_->SetPositionGetter([this] { return beamOrigin_; });

    // 発動前演出の長さを必殺技モーション（MakanSkill.gltf・30fps）に合わせる。
    // 顔アップ＋発動遅延の合計 = 発射キーフレーム（30フレーム目 ≒ 1.0秒）
    beamCutscene_.GetCloseUpDuration() = 0.6f;
    beamCutscene_.GetActivationDelay() = 0.4f;
    pBeamCollider_->SetOnCollisionEnter([this](ColliderBase *pOther) {
        // ビームアクティブ中かつ未ダメージ処理のとき一度だけダメージを与える
        if (pOther->GetTag() == "Player" && beamActive_ && !beamDamageDealt_ && pOwner_->GetTarget())
        {
            Player *pTarget = pOwner_->GetTarget();

            // 必殺技被弾：ビームの進行方向（敵→プレイヤー）へ大きく吹き飛ばし、
            // そのまま地面まで落下する大スタンにする。予約はダメージ適用より先に行う
            Vector3 blowDir = pTarget->GetWorldPosition() - pOwner_->GetWorldPosition();
            blowDir.y = 0.0f;
            pTarget->Status().RequestSkillBlowReaction(blowDir);

            // 必殺技扱いにして、ガードされた場合はエネルギーを大きく削る
            pTarget->SetDamage(kBeamDamage, false, true);
            beamDamageDealt_ = true;

            // 命中したらビーム演出を終了し、パーティクルの新規発生を止める
            // （既存の粒子は寿命どおり自然に消える）。判定コライダーも即無効化される。
            DeactivateBeam();
        }
    });

    // 前方攻撃判定コライダー（PlayerAttackColliderと対称の設計）
    attackCollider_ = std::make_unique<EnemyAttackCollider>();
    attackCollider_->Init(pOwner_);

    // コンボ登録（段の並び・ダメージ・ノックバック・アニメーションは
    // jsons/ComboDefinition/EnemyPunchCombo.json が持つ。ImGuiで調整して同ファイルへ保存できる）
    if (!comboInitialized_)
    {
        // 攻撃の見た目は本体アニメーション（定義の animationPath）で再生するため、
        // モーション再生用ターゲットは指定しない（nullptr）
        punchCombo_.LoadDefinition(kComboDefinitionName);

        // 攻撃発火時にダメージ・ノックバックを保存し、前方コライダーを有効化する
        punchCombo_.SetOnAttackFired(
            [this](float damage, float knockback, float duration, float /*delay*/) {
                currentAttackDamage_ = damage;
                currentAttackKnockback_ = knockback;
                currentAttackDuration_ = duration;
                if (!attackCollider_)
                {
                    return;
                }

                // 段番号（GetCurrentComboIndex() は実行中の段の0始まりインデックス）。
                // 締めの段だけプレイヤーを地面へ叩きつけて吹き飛ばす
                const int stage = punchCombo_.GetCurrentComboIndex() + 1;
                EnemyMeleeHitReaction reaction = EnemyMeleeHitReaction::Normal;
                float kb = knockback;
                if (blowFinisherStage_ > 0 && stage == blowFinisherStage_)
                {
                    reaction = EnemyMeleeHitReaction::Slam;
                    kb = blowFinisherKnockback_;
                }
                else if (blowLaunchStage_ > 0 && stage == blowLaunchStage_)
                {
                    reaction = EnemyMeleeHitReaction::Launch;
                    kb = blowLaunchKnockback_;
                }

                attackCollider_->Activate(damage, kb, duration, 0.0f, reaction);
            });

        comboInitialized_ = true;
    }
}

void EnemyCombat::ComboUpdate()
{
    punchCombo_.Update(Frame::DeltaTime());

    if (isComboAttack_)
    {
        punchCombo_.TryExecuteCombo();
        isComboAttack_ = false;
    }

    // 前方攻撃判定コライダーの遅延・有効時間タイマーを進める
    if (attackCollider_)
    {
        attackCollider_->Update(Frame::DeltaTime());
    }

    // コンボが非アクティブになったらコライダーも強制無効化
    if (!punchCombo_.IsComboActive())
    {
        if (attackCollider_)
        {
            attackCollider_->Deactivate();
        }
    }
}

void EnemyCombat::UpdateEffects(float deltaTime)
{
    if (chargeShake_)
    {
        chargeShake_->Update();
    }

    // 大技演出エミッタの追従＆更新
    UpdateEmitters();

    // ビーム発動前演出（カメラ顔アップ→遅延→発動）の進行
    beamCutscene_.Update(deltaTime);

    // ビーム必殺技のフレーム更新
    UpdateBeam();
}

void EnemyCombat::UpdateEmitters()
{
    Vector3 selfPos = pOwner_->GetWorldPosition();
    Quaternion selfRot = pOwner_->GetLocalRotation();
    if (pChargeAura_)
    {
        pChargeAura_->SetTranslate(selfPos);
        pChargeAura_->SetRotation(-selfRot);
    }
    // pBeamMainEffect_ / pBeamAroundEffect_ の更新・描画はエンジンが自動で回すため、
    // ここでは各所で発生・姿勢を設定するだけでよい。
}

void EnemyCombat::UpdateBullets()
{
    for (auto it = bullets_.begin(); it != bullets_.end();)
    {
        (*it)->Update();
        (*it)->UpdateWorldTransformHierarchy();
        if (!(*it)->IsAlive())
        {
            it = bullets_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void EnemyCombat::DrawParticle(const ViewProjection &viewProjection)
{
    // 大技演出（pChargeAura_ / pBeamMainEffect_ / beamAroundEffect_）は GPU パーティクルで、
    // 描画はエンジンが自動で行う。ここでは別システムの CPU パーティクルだけ描画する。
    // 前方攻撃判定コライダーのヒットエフェクト
    if (attackCollider_)
    {
        attackCollider_->DrawParticle(viewProjection);
    }
    for (auto &bullet : bullets_)
    {
        bullet->DrawParticle(viewProjection);
    }
}

void EnemyCombat::Shot()
{
    if (!pOwner_->GetTarget() || !pOwner_->Status().ConsumeEnergy(kNormalShotEnergyCost))
        return;
    std::string bulletName = "EnemyBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(pOwner_);
    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);
    bullets_.push_back(std::move(bullet));

    pOwner_->Visual().PlayShotAnimation(); // 発射モーション（連射時は毎回先頭から再生し直す）
}

void EnemyCombat::ShotWithDirection(const Vector3 &direction, bool forceHoming)
{
    if (!pOwner_->GetTarget() || !pOwner_->Status().ConsumeEnergy(kNormalShotEnergyCost))
        return;

    std::string bulletName = "EnemyBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);

    bool prevLockOn = pOwner_->GetIsLockOn();
    pOwner_->SetIsLockOn(false);
    bullet->InitTransform(pOwner_);
    pOwner_->SetIsLockOn(prevLockOn);

    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);

    if (forceHoming)
    {
        bullet->SetIsLockOnBullet(true);
        if (pOwner_->GetTarget())
        {
            Vector3 toTarget = pOwner_->GetTarget()->GetLocalPosition() - pOwner_->GetLocalPosition();
            float len = toTarget.Length();
            if (len > kMinRotationDistance)
                toTarget = toTarget / len;
            else
                toTarget = pOwner_->GetForward();
            bullet->SetVelocity(toTarget * bullet->GetCurrentSpeed());
        }
    }
    else
    {
        bullet->SetIsLockOnBullet(false);
        bullet->SetVelocity(-direction * bullet->GetCurrentSpeed());
    }

    bullets_.push_back(std::move(bullet));

    pOwner_->Visual().PlayShotAnimation(); // 発射モーション（連射時は毎回先頭から再生し直す）
}

void EnemyCombat::PerformAttack()
{
    Logger::Log("Attack\n");
}

void EnemyCombat::StartChargeAura()
{
    if (pChargeAura_)
    {
        pChargeAura_->SetTranslate(pOwner_->GetWorldPosition());
        pChargeAura_->SetAuto(true);
    }
    if (chargeShake_)
    {
        chargeShake_->StartShake();
    }
}

void EnemyCombat::StopChargeAura()
{
    if (pChargeAura_)
        pChargeAura_->SetAuto(false);
}

void EnemyCombat::FireChargeBlast()
{
    StopChargeAura();

    if (!pOwner_->GetTarget())
        return;

    // エネルギーが足りなければ通常弾にフォールバック
    if (!pOwner_->Status().ConsumeEnergy(kChargeAttackEnergyCost))
    {
        Shot();
        return;
    }

    // 強化ホーミング弾を1発撃つ
    std::string bulletName = "EnemyChargeBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(pOwner_);
    bullet->GetLocalScale() = {kChargeBlastScale, kChargeBlastScale, kChargeBlastScale};
    bullet->SetColliderRadius(kBulletColliderRadius * kChargeBlastScale);
    bullet->SetIsLockOnBullet(true);
    bullet->SetSpeed(kChargeBlastSpeed);
    bullet->SetDamage(kChargeBlastDamage);

    Vector3 toTarget = pOwner_->GetTarget()->GetLocalPosition() - pOwner_->GetLocalPosition();
    float len = toTarget.Length();
    toTarget = (len > kMinRotationDistance) ? toTarget / len : pOwner_->GetForward();
    bullet->SetVelocity(toTarget * kChargeBlastSpeed);

    bullets_.push_back(std::move(bullet));
}

void EnemyCombat::ActivateBeam()
{
    if (beamActive_)
        return;

    pOwner_->Status().ConsumeEnergy(kUltimateEnergyCost);
    beamActive_ = true;
    beamDamageDealt_ = false;
    beamLength_ = 0.0f;
    beamActiveTime_ = 0.0f;
    beamSpiralTime_ = 0.0f;
    // 発射時点の向きを固定する。ビーム持続中はこの向きを使い続けることで
    // ターゲット追従（ホーミング）を防ぐ。
    beamLockedRotation_ = pOwner_->GetLocalRotation();

    if (pBeamMainEffect_)
        pBeamMainEffect_->SetAuto(true);
    if (pBeamAroundEffect_)
        pBeamAroundEffect_->SetAuto(true);
    if (chargeShake_)
        chargeShake_->StartShake();
}

void EnemyCombat::StartBeamStaging()
{
    if (beamActive_ || beamCutscene_.IsActive())
        return;

    // カメラはプレイヤーのフォローカメラを借りて敵の顔に寄せる
    FollowCamera *pCamera = pOwner_->GetTarget() ? pOwner_->GetTarget()->GetCamera() : nullptr;

    // 演出・遅延中は照準追従を維持し、発動の瞬間の向きで固定する（以降は回避が可能になる）
    beamCutscene_.Start(pOwner_, pCamera, [this] {
        pOwner_->SetIsLockOn(false);
        StopChargeAura();
        ActivateBeam();
    });
}

void EnemyCombat::CancelBeamStaging()
{
    beamCutscene_.Cancel();
}

void EnemyCombat::DeactivateBeam()
{
    beamActive_ = false;
    beamLength_ = 0.0f;
    beamActiveTime_ = 0.0f;
    beamSpiralTime_ = 0.0f;

    if (pBeamMainEffect_)
        pBeamMainEffect_->SetAuto(false);
    if (pBeamAroundEffect_)
        pBeamAroundEffect_->SetAuto(false); // 新規発生を止める（既存パーティクルは自然消滅）
    if (pBeamCollider_)
        pBeamCollider_->SetEnabled(false); // 判定を即無効化（ダメージは止める）
}

void EnemyCombat::UpdateBeam()
{
    if (!beamActive_)
        return;

    float dt = Frame::DeltaTime();
    beamActiveTime_ += dt;

    // 発射時の固定向きで上書きする。しないとOBBコライダーがプレイヤーを追従し続ける
    pOwner_->GetWorldTransform()->quaternionRotation_ = beamLockedRotation_;

    // ビーム長を前方に伸ばす
    beamLength_ += kBeamExtendSpeed * dt;
    if (beamLength_ > kBeamMaxLength)
        beamLength_ = kBeamMaxLength;

    // 発射起点は手（右手ジョイント）に追従させる。コライダー中心も同じ起点を使う
    beamOrigin_ = GetBeamOrigin();
    Vector3 selfPos = beamOrigin_;
    Quaternion selfRot = beamLockedRotation_;

    // メインビームエミッタの設定（MakanAttackSkill と同じアプローチ）
    // SetScale の Z 成分でビームの長さを制御し、SetAnchorPoint でビームの起点を調整する
    if (pBeamMainEffect_)
    {
        pBeamMainEffect_->SetAuto(true);
        pBeamMainEffect_->SetScale(Vector3(0.0f, 0.0f, beamLength_));
        pBeamMainEffect_->SetAnchorPoint(Vector3(0.5f, 0.5f, 0.75f));
        pBeamMainEffect_->SetTranslate(selfPos);
        pBeamMainEffect_->SetRotation(selfRot);
    }

    // らせん状エミッタの設定（MakanAttackSkill と同じアプローチ）
    if (pBeamAroundEffect_)
    {
        pBeamAroundEffect_->SetAuto(true);
        beamSpiralTime_ += dt;

        // クォータニオンからローカル基底を計算する（+Z がプレイヤー方向）
        Quaternion q = selfRot;
        Vector3 localRight(
            1.0f - 2.0f * (q.y * q.y + q.z * q.z),
            2.0f * (q.x * q.y - q.w * q.z),
            2.0f * (q.x * q.z + q.w * q.y));
        Vector3 localUp(
            2.0f * (q.x * q.y + q.w * q.z),
            1.0f - 2.0f * (q.x * q.x + q.z * q.z),
            2.0f * (q.y * q.z - q.w * q.x));
        Vector3 localForward(
            2.0f * (q.x * q.z - q.w * q.y),
            2.0f * (q.y * q.z + q.w * q.x),
            1.0f - 2.0f * (q.x * q.x + q.y * q.y));

        // らせんパーティクルの前進距離
        float forwardDistance = (beamSpiralTime_ * beamSpiralForwardSpeed_) * 2.0f;
        if (forwardDistance > beamLength_ * 2.0f)
            forwardDistance = beamLength_ * 2.0f;

        // ビーム長に対する進捗に応じて巻き角度を決定する
        float t = forwardDistance / kBeamMaxLength;
        float angle = t * beamSpiralRevolution_ * (2.0f * std::numbers::pi_v<float>);

        // らせんオフセット（ローカル座標系で計算）
        Vector3 spiralOffset = localRight * (std::cos(angle) * beamSpiralRadius_) +
                               localUp * (std::sin(angle) * beamSpiralRadius_);

        Vector3 emitterPos = selfPos + localForward * forwardDistance + spiralOffset;
        pBeamAroundEffect_->SetTranslate(emitterPos);
        pBeamAroundEffect_->SetScale(Vector3(0.3f, 0.3f, 0.3f));
    }

    // OBBコライダーをビーム形状に更新する
    if (pBeamCollider_)
    {
        pBeamCollider_->SetEnabled(true);
        pBeamCollider_->SetSize(Vector3(kBeamWidth, kBeamWidth, beamLength_));
        pBeamCollider_->SetAnchorPoint(Vector3(0.5f, 0.5f, 1.0f));
    }

    // 持続時間が過ぎたら自動停止
    if (beamActiveTime_ >= kBeamDuration)
    {
        DeactivateBeam();
    }
}

Vector3 EnemyCombat::GetBeamOrigin()
{
    // 両手を前に突き出すモーションなので、両手ジョイントの中点から発射する。
    // アニメーションに合わせて毎フレーム追従する
    if (pOwner_)
    {
        auto rightHand = pOwner_->GetJointWorldPosition(kRightHandJointName);
        auto leftHand = pOwner_->GetJointWorldPosition(kLeftHandJointName);
        if (rightHand && leftHand)
        {
            return (*rightHand + *leftHand) * 0.5f;
        }
        if (rightHand)
        {
            return *rightHand;
        }
    }

    // ジョイントが取得できない場合は従来どおり固定オフセットで代用する
    Vector3 origin = pOwner_->GetWorldPosition();
    origin.y += kLockOffsetY;
    return origin;
}

void EnemyCombat::SetViewProjection(ViewProjection *pViewProjection)
{
    chargeShake_->Initialize(pViewProjection, "chargehit");
}

void EnemyCombat::RegisterParams()
{
    beamCutscene_.RegisterParams("必殺演出(Enemy)");

    auto *pHub = GameParamHub::GetInstance();
    pHub->Register("Enemy", "コンボ吹き飛ばし段(0で無効)", &blowLaunchStage_, {0.1f, 0.0f, 8.0f});
    pHub->Register("Enemy", "コンボ吹き飛ばし強度", &blowLaunchKnockback_, {0.5f, 0.0f, 100.0f});
    pHub->Register("Enemy", "コンボ叩きつけ段(0で無効)", &blowFinisherStage_, {0.1f, 0.0f, 8.0f});
    pHub->Register("Enemy", "コンボ叩きつけ強度", &blowFinisherKnockback_, {0.5f, 0.0f, 100.0f});
}

void EnemyCombat::DrawComboImGui()
{
#ifdef USE_IMGUI
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                       "現在の攻撃: ダメージ %.1f / ノックバック %.1f",
                       currentAttackDamage_, currentAttackKnockback_);
    ImGui::Separator();
    punchCombo_.DrawImGui();
#endif
}

#define NOMINMAX
#include "EnemyCombat.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "Particle/CSParticle/ParticleCSEditor.h"
#include "Particle/CSParticle/ParticleCSEmitter.h"
#include "application/Camera/FollowCamera.h"
#include <Debug/Log/Logger.h>
#include <Frame.h>
#include <Utility/Debug/GameParam/GameParamHub.h>
#include <cmath>
#include <numbers>

using namespace Hagine;

EnemyCombat::EnemyCombat() {}

EnemyCombat::~EnemyCombat() {
    // ポインタ失効前にゲームパラメータHubから登録を解除する
    GameParamHub::GetInstance()->Unregister("必殺演出(Enemy)");
}

void EnemyCombat::Init(Enemy *owner) {
    owner_ = owner;

    chargeShake_ = std::make_unique<Shake>();

    // チャージ攻撃演出（enemyChargeAura: 既存の気弾チャージオーラ）
    chargeAura_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("enemyChargeAura");
    // ビームメイン演出（プレイヤーの MakanAttackSkill と同じテンプレートを流用）
    beamMainEffect_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("makan_main");
    // ビームらせん演出
    beamAroundEffect_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("makan_around");

    // ビーム判定コライダー（初期は無効化）
    beamCollider_ = owner_->AddOBBCollider("enemy_BeamCollider");
    beamCollider_->SetTag("EnemyBeam");
    beamCollider_->AddCollisionMask("Player");
    beamCollider_->SetEnabled(false);
    beamCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        // ビームアクティブ中かつ未ダメージ処理のとき一度だけダメージを与える
        if (other->GetTag() == "Player" && beamActive_ && !beamDamageDealt_ && owner_->GetTarget()) {
            owner_->GetTarget()->SetDamage(kBeamDamage);
            beamDamageDealt_ = true;
        }
    });

    // 前方攻撃判定コライダー（PlayerAttackColliderと対称の設計）
    attackCollider_ = std::make_unique<EnemyAttackCollider>();
    attackCollider_->Init(owner_);

    // コンボ登録（ダメージ・ノックバックはImGuiで調整・セーブ可能）
    if (!comboInitialized_) {
        punchCombo_.SetName("EnemyPunchCombo"); // DataHandlerのファイル名

        // 攻撃の見た目は本体アニメーション（comboAnimations_）で再生するため、
        // モーション再生用ターゲットは不要（nullptr）。ダメージ等のパラメータのみ指定する
        punchCombo_
            .Add(nullptr, "Jab", 8.0f, 2.0f, 0.25f, 0.08f)
            .Add(nullptr, "Hook", 10.0f, 3.0f, 0.25f, 0.08f)
            .Add(nullptr, "Cross", 10.0f, 3.0f, 0.25f, 0.08f)
            .Add(nullptr, "Uppercut", 12.0f, 5.0f, 0.30f, 0.10f)
            .Add(nullptr, "Overhand", 12.0f, 5.0f, 0.30f, 0.10f)
            .Add(nullptr, "Swing", 14.0f, 6.0f, 0.30f, 0.10f)
            .Add(nullptr, "Elbow", 16.0f, 7.0f, 0.25f, 0.06f)
            .Add(nullptr, "Slam", 20.0f, 10.0f, 0.35f, 0.12f);

        // JSONがあれば保存済みの値で上書き
        punchCombo_.LoadAttackParams();

        // 攻撃発火時にダメージ・ノックバックを保存し、前方コライダーを有効化する
        punchCombo_.SetOnAttackFired(
            [this](float damage, float knockback, float duration, float /*delay*/) {
                currentAttackDamage_ = damage;
                currentAttackKnockback_ = knockback;
                currentAttackDuration_ = duration;
                if (attackCollider_) {
                    attackCollider_->Activate(damage, knockback, duration);
                }
            });

        comboInitialized_ = true;
    }

    // コンボ段ごとの本体アニメーション（プレイヤーと同じ割り当て：パンチ4段＋キック3段＋叩きつけ）
    comboAnimations_ = {
        "animation/Player/Punch_1.gltf", // 1段目: Jab
        "animation/Player/Punch_2.gltf", // 2段目: Hook
        "animation/Player/Punch_3.gltf", // 3段目: Cross
        "animation/Player/Punch_4.gltf", // 4段目: Uppercut
        "animation/Player/Kick_1.gltf",  // 5段目: Overhand
        "animation/Player/Kick_2.gltf",  // 6段目: Swing
        "animation/Player/Kick_3.gltf",  // 7段目: Elbow
        "animation/Player/Smash.gltf",   // 8段目: Slam
    };
}

void EnemyCombat::ConboUpdate() {
    punchCombo_.Update(Frame::DeltaTime());

    if (isComboAttack_) {
        punchCombo_.TryExecuteCombo();
        isComboAttack_ = false;
    }

    // 前方攻撃判定コライダーの遅延・有効時間タイマーを進める
    if (attackCollider_) {
        attackCollider_->Update(Frame::DeltaTime());
    }

    // コンボが非アクティブになったらコライダーも強制無効化
    if (!punchCombo_.IsComboActive()) {
        if (attackCollider_) {
            attackCollider_->Deactivate();
        }
    }
}

void EnemyCombat::UpdateEffects(float deltaTime) {
    if (chargeShake_) {
        chargeShake_->Update();
    }

    // 大技演出エミッタの追従＆更新
    UpdateEmitters();

    // ビーム発動前演出（カメラ顔アップ→遅延→発動）の進行
    beamCutscene_.Update(deltaTime);

    // ビーム必殺技のフレーム更新
    UpdateBeam();
}

void EnemyCombat::UpdateEmitters() {
    Vector3 selfPos = owner_->GetWorldPosition();
    Quaternion selfRot = owner_->GetLocalRotation();
    if (chargeAura_) {
        chargeAura_->SetTranslate(selfPos);
        chargeAura_->SetRotation(-selfRot);
        chargeAura_->Update();
    }
    if (beamMainEffect_) {
        beamMainEffect_->Update();
    }
    if (beamAroundEffect_) {
        beamAroundEffect_->Update();
    }
}

void EnemyCombat::UpdateBullets() {
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        (*it)->Update();
        (*it)->UpdateWorldTransformHierarchy();
        if (!(*it)->IsAlive()) {
            it = bullets_.erase(it);
        } else {
            ++it;
        }
    }
}

void EnemyCombat::DrawParticle(const ViewProjection &viewProjection) {
    // 大技演出（Graphics フェーズ）
    if (chargeAura_)
        chargeAura_->DrawGraphics(viewProjection);
    if (beamMainEffect_)
        beamMainEffect_->DrawGraphics(viewProjection);
    if (beamAroundEffect_)
        beamAroundEffect_->DrawGraphics(viewProjection);
    // 前方攻撃判定コライダーのヒットエフェクト
    if (attackCollider_) {
        attackCollider_->DrawParticle(viewProjection);
    }
    for (auto &bullet : bullets_) {
        bullet->DrawParticle(viewProjection);
    }
}

void EnemyCombat::DrawParticleCompute(const ViewProjection &viewProjection) {
    if (chargeAura_)
        chargeAura_->DrawCompute(viewProjection);
    if (beamMainEffect_)
        beamMainEffect_->DrawCompute(viewProjection);
    if (beamAroundEffect_)
        beamAroundEffect_->DrawCompute(viewProjection);
}

void EnemyCombat::Shot() {
    if (!owner_->GetTarget() || !owner_->Status().ConsumeEnergy(kNormalShotEnergyCost))
        return;
    std::string bulletName = "EnemyBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(owner_);
    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);
    bullets_.push_back(std::move(bullet));
}

void EnemyCombat::ShotWithDirection(const Vector3 &direction, bool forceHoming) {
    if (!owner_->GetTarget() || !owner_->Status().ConsumeEnergy(kNormalShotEnergyCost))
        return;

    std::string bulletName = "EnemyBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);

    bool prevLockOn = owner_->GetIsLockOn();
    owner_->SetIsLockOn(false);
    bullet->InitTransform(owner_);
    owner_->SetIsLockOn(prevLockOn);

    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);

    if (forceHoming) {
        bullet->SetIsLockOnBullet(true);
        if (owner_->GetTarget()) {
            Vector3 toTarget = owner_->GetTarget()->GetLocalPosition() - owner_->GetLocalPosition();
            float len = toTarget.Length();
            if (len > kMinRotationDistance)
                toTarget = toTarget / len;
            else
                toTarget = owner_->GetForward();
            bullet->SetVelocity(toTarget * bullet->GetCurrentSpeed());
        }
    } else {
        bullet->SetIsLockOnBullet(false);
        bullet->SetVelocity(-direction * bullet->GetCurrentSpeed());
    }

    bullets_.push_back(std::move(bullet));
}

void EnemyCombat::PerformAttack() {
    Logger::Log("Attack\n");
}

void EnemyCombat::StartChargeAura() {
    if (chargeAura_) {
        chargeAura_->SetTranslate(owner_->GetWorldPosition());
        chargeAura_->SetAuto(true);
    }
    if (chargeShake_) {
        chargeShake_->StartShake();
    }
}

void EnemyCombat::StopChargeAura() {
    if (chargeAura_)
        chargeAura_->SetAuto(false);
}

void EnemyCombat::FireChargeBlast() {
    StopChargeAura();

    if (!owner_->GetTarget())
        return;

    // エネルギーが足りなければ通常弾にフォールバック
    if (!owner_->Status().ConsumeEnergy(kChargeAttackEnergyCost)) {
        Shot();
        return;
    }

    // 強化ホーミング弾を1発撃つ
    std::string bulletName = "EnemyChargeBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(owner_);
    bullet->GetLocalScale() = {kChargeBlastScale, kChargeBlastScale, kChargeBlastScale};
    bullet->SetColliderRadius(kBulletColliderRadius * kChargeBlastScale);
    bullet->SetIsLockOnBullet(true);
    bullet->SetSpeed(kChargeBlastSpeed);
    bullet->SetDamage(kChargeBlastDamage);

    Vector3 toTarget = owner_->GetTarget()->GetLocalPosition() - owner_->GetLocalPosition();
    float len = toTarget.Length();
    toTarget = (len > kMinRotationDistance) ? toTarget / len : owner_->GetForward();
    bullet->SetVelocity(toTarget * kChargeBlastSpeed);

    bullets_.push_back(std::move(bullet));
}

void EnemyCombat::ActivateBeam() {
    if (beamActive_)
        return;

    owner_->Status().ConsumeEnergy(kUltimateEnergyCost);
    beamActive_ = true;
    beamDamageDealt_ = false;
    beamLength_ = 0.0f;
    beamActiveTime_ = 0.0f;
    beamSpiralTime_ = 0.0f;
    // 発射時点の向きを固定する。ビーム持続中はこの向きを使い続けることで
    // ターゲット追従（ホーミング）を防ぐ。
    beamLockedRotation_ = owner_->GetLocalRotation();

    if (beamMainEffect_)
        beamMainEffect_->SetAuto(true);
    if (beamAroundEffect_)
        beamAroundEffect_->SetAuto(true);
    if (chargeShake_)
        chargeShake_->StartShake();
}

void EnemyCombat::StartBeamStaging() {
    if (beamActive_ || beamCutscene_.IsActive())
        return;

    // カメラはプレイヤーのフォローカメラを借りて敵の顔に寄せる
    FollowCamera *camera = owner_->GetTarget() ? owner_->GetTarget()->GetCamera() : nullptr;

    // 演出・遅延中はロックオン（照準追従）を維持し、発動の瞬間に固定する。
    // ActivateBeam() 内で発射時の quateRotation_ が beamLockedRotation_ に
    // スナップショットされるため、以降は向きが固定され回避が可能になる
    beamCutscene_.Start(owner_, camera, [this] {
        owner_->SetIsLockOn(false);
        StopChargeAura();
        ActivateBeam();
    });
}

void EnemyCombat::CancelBeamStaging() {
    beamCutscene_.Cancel();
}

void EnemyCombat::DeactivateBeam() {
    beamActive_ = false;
    beamLength_ = 0.0f;
    beamActiveTime_ = 0.0f;
    beamSpiralTime_ = 0.0f;

    if (beamMainEffect_)
        beamMainEffect_->SetAuto(false);
    if (beamAroundEffect_)
        beamAroundEffect_->SetAuto(false); // 新規発生を止める（既存パーティクルは自然消滅）
    if (beamCollider_)
        beamCollider_->SetEnabled(false); // 判定を即無効化（ダメージは止める）
}

void EnemyCombat::UpdateBeam() {
    if (!beamActive_)
        return;

    float dt = Frame::DeltaTime();
    beamActiveTime_ += dt;

    // ビーム発射中は transform_->quateRotation_ を発射時の固定向きで上書きする。
    // OBBコライダーは transform_->quateRotation_ から向きを取るため、
    // ここで上書きしないと RotateUpdate() によりコライダーがプレイヤーを追従し続ける。
    owner_->GetWorldTransform()->quateRotation_ = beamLockedRotation_;

    // ビーム長を前方に伸ばす
    beamLength_ += kBeamExtendSpeed * dt;
    if (beamLength_ > kBeamMaxLength)
        beamLength_ = kBeamMaxLength;

    Vector3 selfPos = owner_->GetWorldPosition();
    selfPos.y = owner_->GetWorldPosition().y + kLockOffsetY;
    Quaternion selfRot = beamLockedRotation_;

    // メインビームエミッタの設定（MakanAttackSkill と同じアプローチ）
    // SetScale の Z 成分でビームの長さを制御し、SetAnchorPoint でビームの起点を調整する
    if (beamMainEffect_) {
        beamMainEffect_->SetAuto(true);
        beamMainEffect_->SetScale(Vector3(0.0f, 0.0f, beamLength_));
        beamMainEffect_->SetAnchorPoint(Vector3(0.5f, 0.5f, 0.75f));
        beamMainEffect_->SetTranslate(selfPos);
        beamMainEffect_->SetRotation(selfRot);
    }

    // らせん状エミッタの設定（MakanAttackSkill と同じアプローチ）
    if (beamAroundEffect_) {
        beamAroundEffect_->SetAuto(true);
        beamSpiralTime_ += dt;

        // クォータニオンからローカル座標系の基底ベクトルを計算する
        // RotateUpdate が +Z をプレイヤー方向に向けるため、
        // localForward（+Z 基底）はそのままプレイヤー方向を向く
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
        beamAroundEffect_->SetTranslate(emitterPos);
        beamAroundEffect_->SetScale(Vector3(0.3f, 0.3f, 0.3f));
    }

    // OBBコライダーをビーム形状に更新する
    if (beamCollider_) {
        beamCollider_->SetEnabled(true);
        beamCollider_->SetSize(Vector3(kBeamWidth, kBeamWidth, beamLength_));
        beamCollider_->SetAnchorPoint(Vector3(0.5f, 0.5f, 1.0f));
    }

    // 持続時間が過ぎたら自動停止
    if (beamActiveTime_ >= kBeamDuration) {
        DeactivateBeam();
    }
}

void EnemyCombat::SetVp(ViewProjection *vp) {
    chargeShake_->Initialize(vp, "chargehit");
}

void EnemyCombat::RegisterParams() {
    beamCutscene_.RegisterParams("必殺演出(Enemy)");
}

void EnemyCombat::DrawComboImGui() {
#ifdef USE_IMGUI
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                       "現在の攻撃: ダメージ %.1f / ノックバック %.1f",
                       currentAttackDamage_, currentAttackKnockback_);
    ImGui::Separator();
    punchCombo_.DrawImGui();
#endif
}
